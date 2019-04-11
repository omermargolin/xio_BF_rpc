/* example.c -- usage example of the zlib compression library
 * Copyright (C) 1995-2006, 2011, 2016 Jean-loup Gailly
 * For conditions of distribution and use, see copyright notice in zlib.h
 */

/* @(#) $Id$ */

#include "zlib.h"
#include <stdio.h>

#ifdef STDC
#  include <string.h>
#  include <stdlib.h>
#endif

#if defined(VMS) || defined(RISCOS)
#  define TESTFILE "foo-gz"
#else
#  define TESTFILE "foo.gz"
#endif

#define CHECK_ERR(err, msg) { \
    if (err != Z_OK) { \
        fprintf(stderr, "%s error: %d\n", msg, err); \
        exit(1); \
    } \
}


int test_large_deflate OF((Byte *compr, uLong comprLen,
                            Byte *uncompr, uLong uncomprLen));
void test_large_inflate OF((Byte *compr, uLong comprLen,
                            Byte *uncompr, uLong uncomprLen));
int  main               OF((int argc, char *argv[]));

static alloc_func zalloc = (alloc_func)0;
static free_func zfree = (free_func)0;


/* ===========================================================================
 * Test deflate() with large buffers and dynamic change of compression level
 */
int test_large_deflate(compr, comprLen, uncompr, uncomprLen)
    Byte *compr, *uncompr;
    uLong comprLen, uncomprLen;
{
    z_stream c_stream; /* compression stream */
    int err;

    c_stream.zalloc = zalloc;
    c_stream.zfree = zfree;
    c_stream.opaque = (voidpf)0;

    err = deflateInit(&c_stream, Z_BEST_SPEED);
    CHECK_ERR(err, "deflateInit");

    c_stream.next_out = compr;
    c_stream.avail_out = (uInt)comprLen;

    /* At this point, uncompr is still mostly zeroes, so it should compress
     * very well:
     */
    c_stream.next_in = uncompr;
    c_stream.avail_in = (uInt)uncomprLen;
    deflateParams(&c_stream, Z_BEST_COMPRESSION, Z_FILTERED);

    err = deflate(&c_stream, Z_NO_FLUSH);
    CHECK_ERR(err, "deflate");
    if (c_stream.avail_in != 0) {
        fprintf(stderr, "deflate not greedy\n");
        exit(1);
    }

    printf("After 1st deflate, c_stream.next_out=%lx, c_stream.avail_out=%d\n", c_stream.next_out, c_stream.avail_out);

    if (0) {
    /* Feed in already compressed data and switch to no compression: */
    deflateParams(&c_stream, Z_NO_COMPRESSION, Z_DEFAULT_STRATEGY);
    c_stream.next_in = compr;
    c_stream.avail_in = (uInt)comprLen/2;
    err = deflate(&c_stream, Z_NO_FLUSH);
    CHECK_ERR(err, "deflate");

    printf("After 2nd deflate, c_stream.next_out=%lx, c_stream.avail_out=%d\n", c_stream.next_out, c_stream.avail_out);


    /* Switch back to compressing mode: */
    deflateParams(&c_stream, Z_BEST_COMPRESSION, Z_FILTERED);
    c_stream.next_in = uncompr;
    c_stream.avail_in = (uInt)uncomprLen;
    err = deflate(&c_stream, Z_NO_FLUSH);
    CHECK_ERR(err, "deflate");

    printf("After 3rd deflate, c_stream.next_out=%lx, c_stream.avail_out=%d\n", c_stream.next_out, c_stream.avail_out);
}

    err = deflate(&c_stream, Z_FINISH);
    if (err != Z_STREAM_END) {
        fprintf(stderr, "deflate should report Z_STREAM_END\n");
        exit(1);
    }


    printf("After 4rd deflate, c_stream.next_out=%lx, c_stream.avail_out=%d\n", c_stream.next_out, c_stream.avail_out);

    err = deflateEnd(&c_stream);
    CHECK_ERR(err, "deflateEnd");
    return comprLen - c_stream.avail_out;
}

/* ==========================================i=================================
 * Test inflate() with large buffers
 */
void test_large_inflate(compr, comprLen, uncompr, uncomprLen)
    Byte *compr, *uncompr;
    uLong comprLen, uncomprLen;
{
    int err;
    Byte *tmp_uncompr;
    int i = 0;
    z_stream d_stream; /* decompression stream */

    strcpy((char*)uncompr, "garbage");

    d_stream.zalloc = zalloc;
    d_stream.zfree = zfree;
    d_stream.opaque = (voidpf)0;

    d_stream.next_in  = compr;
    d_stream.avail_in = (uInt)comprLen;

    err = inflateInit(&d_stream);
    CHECK_ERR(err, "inflateInit");

    d_stream.next_out = uncompr;            /* discard the output */
    d_stream.avail_out = (uInt)uncomprLen;


    for (;;) {
        printf("inside test_large_inflate, err=%d, next_in=%lx avail_in=%lu, next_out=%lu, avail_out=%lu uncompress content: %s\n", err, d_stream.next_in, d_stream.avail_in, d_stream.next_out, d_stream.avail_out, (char*)(uncompr));


        err = inflate(&d_stream, Z_NO_FLUSH);
        if (err == Z_STREAM_END) break;
        CHECK_ERR(err, "large inflate");
    }
    printf("inside test_large_inflate - after for loop, err=%d, next_in=%lx avail_in=%lu, next_out=%lu, avail_out=%lu uncompress content: %s\n", err, d_stream.next_in, d_stream.avail_in, d_stream.next_out, d_stream.avail_out, (char*)(uncompr));

    printf("uncompress content the end: %s\n", (char*)uncompr);
    err = inflateEnd(&d_stream);
    CHECK_ERR(err, "inflateEnd");

    if (d_stream.total_out != 2*uncomprLen + comprLen/2) {
        fprintf(stderr, "bad large inflate: %ld\n", d_stream.total_out);
        exit(1);
    } else {
        printf("large_inflate(): OK\n");
    }
}

int decompress_data(char *buffer, int compress_len, char *de_buffer, int decompress_len)
{
    //Byte *compr, *uncompr;
    //uLong comprLen = 10000*sizeof(int); /* don't overflow on MSDOS */
    //uLong uncomprLen = comprLen;
    //static const char* myVersion = ZLIB_VERSION;

    //if (zlibVersion()[0] != myVersion[0]) {
    //    fprintf(stderr, "incompatible zlib version\n");
    //    exit(1);

    //} else if (strcmp(zlibVersion(), ZLIB_VERSION) != 0) {
    //    fprintf(stderr, "warning: different zlib version\n");
    //}

    //printf("zlib version %s = 0x%04x, compile flags = 0x%lx\n",
    //        ZLIB_VERSION, ZLIB_VERNUM, zlibCompileFlags());

    //compr    = (Byte*)calloc((uInt)comprLen, 1);
    //uncompr  = (Byte*)calloc((uInt)uncomprLen, 1);
    /* compr and uncompr are cleared to avoid reading uninitialized
     * data and to ensure that uncompr compresses well.
     */
    //if (compr == Z_NULL || uncompr == Z_NULL) {
    //    printf("out of memory\n");
    //    exit(1);
    //}

    //strcpy((char*)uncompr, "hello hello!");
    //printf("Before deflate (compress) uncompr=%s\n", (char*) uncompr);

    //int compr_size;

    //compr_size = test_large_deflate(compr, comprLen, uncompr, uncomprLen);
    //    printf("After deflate (compress) compr_size=%d, uncompr=%s, compr=%s\n", compress_len, (char*) buffer);

	printf("calling test_large_inflate\n");
	test_large_inflate(buffer, compress_len, de_buffer, decompress_len);
    printf("After inflate (decompress) uncompr=%s, compr=%s\n", (char*) de_buffer, (char*) buffer);


    //free(buffer);
    //free(uncompr);

    return 0;
}
