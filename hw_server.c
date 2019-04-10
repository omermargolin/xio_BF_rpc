#include "hw.h"
#include "xcommon.h"

#define MAX_RESOURCE_HANDLES 20
#define INITIAL_PORT 19875

static uint8_t SHA1_INIT[] =
{
    0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF,
    0xFE, 0xDC, 0xBA, 0x98, 0x76, 0x54, 0x32, 0x10,
    0xF0, 0xE1, 0xD2, 0xC3
};

static uint8_t SHA2_256_INIT[] =
{
    0x67, 0xE6, 0x09, 0x6A, 0x85, 0xAE, 0x67, 0xBB,
    0x72, 0xF3, 0x6E, 0x3C, 0x3A, 0xF5, 0x4F, 0xA5,
    0x7F, 0x52, 0x0E, 0x51, 0x8C, 0x68, 0x05, 0x9B,
    0xAB, 0xD9, 0x83, 0x1F, 0x19, 0xCD, 0xE0, 0x5B
};

#define STORE_BE8(ptr, data) \
    ({ * ((uint64_t *) ptr) = __builtin_bswap64(data); })

//./rdma_server -g 0 -d mlx5_0
struct config_t in_config = {
    "mlx5_0",                     /* dev_name */
    NULL,                         /* server_name */
    INITIAL_PORT,                        /* tcp_port */
    1,                            /* ib_port */
    0                             /* gid_idx */
};

int test_param=5;
struct resources resource_handles[MAX_RESOURCE_HANDLES];
int last_resource_handle = -1;


char **hw_1_svc(rpc_args_t *a, struct svc_req *req) {
	static char msg[256];
	static char *p;
	static char *buffer;
	rpc_args_t* remote_args = (rpc_args_t*)a;
	int rc;
	printf ("Entering hw_1_svc\n");
	printf("getting ready to return value\n");
    printf("given queue number=%d\n", remote_args->qp_num);
    printf("src value:%p\n", remote_args->src_add);
    printf("dest value:%p\n", remote_args->dest_add);
    printf("dest key: %x\n", remote_args->dest_key);

    printf("address:%p\n", &remote_args->src_add);

    printf("device name is:%s\n", remote_args->device_name);
    printf("read from lba:%d\n", remote_args->lba);
    printf("read %d lbas.\n", remote_args->num_lbas);
    printf("write to lba:%d\n", remote_args->dest_lba);


    buffer = (char *) malloc (remote_args->num_lbas * 512);
    if (!buffer)
    {
       fprintf (stderr, "failed to malloc %Zu bytes to memory buffer\n", remote_args->num_lbas * 512);
       strcpy(msg, "Finish Server");
 	   p = msg;
 	   printf("Returning...\n");
       return (&p);
    }

    rc = read_blkdev(remote_args->device_name, remote_args->lba, remote_args->num_lbas, buffer);
    if (rc)
    {
       fprintf (stderr, "failed to read from device\n");
       strcpy(msg, "Finish Server");
 	   p = msg;
 	   printf("Returning...\n");
       return (&p);
    }

    //printf("*address:%s\n", *remote_args->src_add);

    //	 printf("len:%d\n", remote_args->len);

    //	 printf("dest val:%s\n", remote_args->dest_add);
    //	 printf("dest add:%p\n", &remote_args->dest_add);
	 //	 printf("dest ref val:%s\n", *remote_args->dest_add);


	printf("> > > > > Start post_send");
    resource_handles[remote_args->qp_num].remote_props.addr =  remote_args->src_add;
	resource_handles[remote_args->qp_num].remote_buf_len = remote_args->len;

    	/*
    	 * Allocate buffer by size of remote len
    	 */
    if (!resource_handles[remote_args->qp_num].buf)
    {
        fprintf (stderr, "failed to malloc %Zu bytes to memory buffer\n", remote_args->len);
        strcpy(msg, "Finish Server");
  	    p = msg;
    	printf("Returning...\n");
        return (&p);
    }
    memset (resource_handles[remote_args->qp_num].buf, 0, remote_args->len);
    if (post_send (&resource_handles[remote_args->qp_num], IBV_WR_RDMA_READ))
    {
        fprintf (stderr, "failed to post SR 2\n");
        if (resources_destroy (&resource_handles[remote_args->qp_num]))
        {
            fprintf (stderr, "failed to destroy resources\n");
        }
    }
    rc = poll_completion(&resource_handles[remote_args->qp_num]);

    printf("Buffer: %s\n", resource_handles[remote_args->qp_num].buf);

    uint8_t result[128];

    printf("Calc SHA1\n");
    //    Sha1(resource_handles[remote_args->qp_num].buf, remote_args->len, result );
    Sha2_256(resource_handles[remote_args->qp_num].buf, remote_args->len, &result );
  printf("SHA Calc done\n");
    fflush(stdout);
    printf("Result after SHA: ");
    int x;
    for(x = 0; x < 128; x++)
      printf("%02x", result[x]);
    putchar( '\n' );
    printf("Result after SHA: 0x%x\n",result);
    printf("Result after SHA: %s\n",result);
    strcpy(msg, "Finish Server");
    p = msg;
/*And now return the SHA result*/

    resource_handles[remote_args->qp_num].remote_props.addr =  remote_args->dest_add;
    uint64_t temp_len =  resource_handles[remote_args->qp_num].remote_buf_len;
    resource_handles[remote_args->qp_num].remote_buf_len = 64;
    uint32_t rkey_temp = resource_handles[remote_args->qp_num].remote_props.rkey;

    resource_handles[remote_args->qp_num].remote_props.rkey = remote_args->dest_key;
    printf("rkey: %x, saved rkey\n", resource_handles[remote_args->qp_num].remote_props.rkey ,rkey_temp);


    sprintf(resource_handles[remote_args->qp_num].buf, result);
    if (post_send (&resource_handles[remote_args->qp_num], IBV_WR_RDMA_WRITE))
    {
        fprintf (stderr, "failed to post SR 2\n");
        if (resources_destroy (&resource_handles[remote_args->qp_num]))
        {
            fprintf (stderr, "failed to destroy resources\n");
        }
    }
    resource_handles[remote_args->qp_num].remote_props.rkey = rkey_temp;
    resource_handles[remote_args->qp_num].remote_buf_len = temp_len;
    printf("restored rkey: %x\n", resource_handles[remote_args->qp_num].remote_props.rkey);
    sleep(2);

    printf("Returning...\n");

    return (&p);
}

char **rdmac_1_svc(rpc_args_t *a, struct svc_req *req) {
	static char msg[256];
	static char *p;
//	resources_init (&res);
	printf("Entering: rmdac_1_svc\n");
	rpc_args_t* remote_args = (rpc_args_t*)a;

	printf("getting ready to set rdma connection for id: %d\n", remote_args->qp_num);

	in_config.tcp_port = resource_handles[remote_args->qp_num].tcp_port;
	create_res_handle(&resource_handles[remote_args->qp_num], in_config);
	//	printf("getting ready to set global rdma value\n");
	strcpy(msg, "global changed");
	/* sprintf(msg, "%d", last_resource_handle); */
	p = msg;
	printf("rdmac_1_svc: Returning\n", msg);
	// TODO: create the QP handle like in rdma_queue and store in a global location for hw_1_svc to use when triggered

	return(&p);
}

char **aquirep_1_svc(void *a, struct svc_req *req) {
	static char msg[256];
	static char *p;
//	resources_init (&res);
	printf("Entering: aquirep_svc\n");
	last_resource_handle++;  // TODO: Add check that we aren't blowing past the end of the array
	resource_handles[last_resource_handle].tcp_port = INITIAL_PORT + last_resource_handle;
	sprintf(msg, "%d, %d", last_resource_handle, resource_handles[last_resource_handle].tcp_port);
//	strcpy(msg, "global changed");
	p = msg;
	printf("aquirep eturning %s\n", msg);
	return(&p);
}


void Sha1(uint8_t *data_ptr, uint32_t data_len, uint8_t *hash_result)
{
    uint64_t total_bit_len;
    uint32_t num_whole_blks, whole_blks_len;
    uint8_t  in_buf[64], state[20];
    printf("Start SHA1 after init\n");
    printf("data_ptr:%p\n",data_ptr);
    printf("len data_ptr:%d\n",sizeof(data_ptr));
    printf("data_ptr:%s\n",data_ptr);

    total_bit_len = 8 * data_len;
    memcpy(&state[0], SHA1_INIT, sizeof(state));
    printf("111Start SHA1 after init\n");

    // Do an even number of 64-byte SHA1 input data blocks:
    num_whole_blks = data_len / 64;
    printf("num_whole_blks:%d\n",num_whole_blks);

    if (num_whole_blks != 0)
    {
        printf("Inside blk\n");
        whole_blks_len = 64 * num_whole_blks;
        printf("before fastSHA\n");

        FastSha1Data(state, data_ptr, whole_blks_len);
        data_ptr += whole_blks_len;
        data_len -= whole_blks_len;
    }

    // Now do the final blk or 2.  Note that data_len MUST be <= 63 here.
    printf("After Fast SHA\n");
    fflush(stdout);
    memset(in_buf, 0, sizeof(in_buf));
    if (data_len != 0)
        memcpy(in_buf, data_ptr, data_len);


    printf("After Memset\n");
    fflush(stdout);
    in_buf[data_len] = 0x80;
    if (56 <= data_len)
    {
        FastSha1Data(state, in_buf, 64);
        memset(in_buf, 0, 64);
    }
    printf("After Second FastSha1Data\n");
    fflush(stdout);
    STORE_BE8(&in_buf[56], total_bit_len);
    FastSha1Data(state, in_buf, 64);
    BigEndian4Copy(state, hash_result, 5);
    printf("After Second FastSha1Data\n");
    fflush(stdout);
}

void Sha2_256(uint8_t *data_ptr, uint32_t data_len, uint8_t *hash_result)
{
    uint64_t total_bit_len;
    uint32_t num_whole_blks, whole_blks_len;
    uint8_t  in_buf[64], state[32];

    total_bit_len = 8 * data_len;
    memcpy(&state[0], SHA2_256_INIT, sizeof(state));

    // Do an even number of 64-byte SHA2_256 input data blocks:
    num_whole_blks = data_len / 64;
    if (num_whole_blks != 0)
    {
        whole_blks_len = 64 * num_whole_blks;
        FastSha2_256Data(state, data_ptr, whole_blks_len);
        data_ptr += whole_blks_len;
        data_len -= whole_blks_len;
    }

    // Now do the final blk or 2.  Note that data_len MUST be <= 63 here.
    memset(in_buf, 0, sizeof(in_buf));
    if (data_len != 0)
        memcpy(in_buf, data_ptr, data_len);

    in_buf[data_len] = 0x80;
    if (56 <= data_len)
    {
        FastSha2_256Data(state, in_buf, 64);
        memset(in_buf, 0, 64);
    }

    STORE_BE8(&in_buf[56], total_bit_len);
    FastSha2_256Data(state, in_buf, 64);
    BigEndian4Copy(state, hash_result, 8);
}

void BigEndian4Copy(void *in_ptr, void *out_ptr, uint32_t num_words)
{
    uint32_t *in32_ptr, *out32_ptr, idx;

    in32_ptr  = (uint32_t *) in_ptr;
    out32_ptr = (uint32_t *) out_ptr;
    for (idx = 0; idx < num_words; idx++)
        *out32_ptr++ = __builtin_bswap32(*in32_ptr++);
}
