#include "hw.h"
#include "xcommon.h"

#define MAX_RESOURCE_HANDLES 20
#define INITIAL_PORT 19875

//./rdma_server -g 0 -d mlx5_0
struct config_t in_config = {
    "mlx5_0",                     /* dev_name */
    NULL,                         /* server_name */
    INITIAL_PORT,                 /* tcp_port */
    1,                            /* ib_port */
    0                             /* gid_idx */
};

int test_param=5;
struct resources resource_handles[MAX_RESOURCE_HANDLES];
int last_resource_handle = -1;

char **hw_1_svc(rpc_args_t *remote_args, struct svc_req *req) {
    static char msg[256];
    static char *result_p;
    static char *buffer, *de_buffer;
    int rc;
    // printf ("Entering hw_1_svc\n");
    // printf("getting ready to return value\n");
    // printf("given queue number=%d\n", remote_args->qp_num);
    // printf("src value:%p\n", remote_args->src_add);
    // printf("dest value:%p\n", remote_args->dest_add);
    // printf("dest key: %x\n", remote_args->dest_key);
    // printf("address:%p\n", &remote_args->src_add);
    // printf("device name is:%s\n", remote_args->device_name);
    // printf("read from lba:%d\n", remote_args->lba);
    // printf("read %d lbas.\n", remote_args->num_lbas);
    // printf("write to lba:%d\n", remote_args->dest_lba);

    buffer = (char *) malloc (remote_args->num_lbas * 512);
    if (!buffer)
    {
       fprintf (stderr, "failed to malloc %Zu bytes to memory buffer\n", remote_args->num_lbas * 512);
       strcpy(msg, "Finish Server");
       result_p = msg;
       printf("Returning...\n");
       return (&result_p);
    }

    rc = read_blkdev(remote_args->device_name, remote_args->lba, remote_args->num_lbas, buffer);
    if (rc)
    {
       fprintf (stderr, "failed to read from device\n");
       strcpy(msg, "Finish Server");
       result_p = msg;
       printf("Returning...\n");
       free(buffer);
       return (&result_p);
    }

    de_buffer = (char *) malloc (4096);
	if (!de_buffer)
	{
	   fprintf (stderr, "failed to malloc 4096 bytes to memory buffer\n");
	   strcpy(msg, "Finish Server");
	   result_p = msg;
	   printf("Returning...\n");
       free(buffer);
	   return (&result_p);
	}

    struct vlb_d vlb[8] = {
      {164, true},
      {26, true},
      {4096, false},
      {26, true},
      {4096, false},
      {4096, false},
      {26, true},
      {26, true},
    };

    /*  Sha1 expectations:
    [root@indus inflate_alone]# sha1sum zero.txt
    1ceaf73df40e531df3bfb26b4fb7cd95fb7bff1d  zero.txt
    [root@indus inflate_alone]# sha1sum rand.txt
    2378cea99e6e366efbea2ef805735b86bfeca6ca  rand.txt
    [root@indus inflate_alone]# sha1sum private_data.txt
    51698b37afbec6fcc48d317ac8523c27736da6b5  private_data.txt
    */
    int start_position = 0;
    char hashes[VLB_SIZE][20];
    int i;
    for (i=0; i < VLB_SIZE; i++)
    {
      //TODO: Compute hash on each 512 byte block of 4k
      if (vlb[i].compressed)
      {
    	//   printf("Calling decompress with %d\n", vlb[i].len );
          //decompress from current pointer to len
    	  rc = decompress_data(buffer + start_position, vlb[i].len, de_buffer, 4096);
//    	  printf("returned from decompress...\n");
    	  if (rc)
    	  {
    		  fprintf (stderr, "failed to decompress data\n");
    		  strcpy(msg, "Finish Server");
    		  result_p = msg;
    		  printf("Returning...\n");
              free(buffer);
              free(de_buffer);
              free(hashes);
    		  return (&result_p);
    	  }
          // compute hash of returned decompressed buffer
          // add hash to hash result array
    	  Sha1(de_buffer, 4096, hashes[i]);
      }
      else
      {
    	  Sha1(buffer + start_position, 4096, hashes[i]);
      }
//      print_sha(hashes[i],20);
      // increase current pointer by len
      start_position += vlb[i].len;
    }

    // printf("> > > > > Start post_send");
    resource_handles[remote_args->qp_num].remote_props.addr =  remote_args->src_add;
    resource_handles[remote_args->qp_num].remote_buf_len = remote_args->len;

    /*
    * Allocate buffer by size of remote len
    */
    if (!resource_handles[remote_args->qp_num].buf)
    {
        fprintf (stderr, "failed to malloc %Zu bytes to memory buffer\n", remote_args->len);
        strcpy(msg, "Finish Server");
        result_p = msg;
        printf("Returning...\n");
        free(buffer);
        free(de_buffer);
        free(hashes);
        return (&result_p);
    }

    memset(resource_handles[remote_args->qp_num].buf, 0, remote_args->len);
    strcpy(msg, "Finish Server");
    result_p = msg;
    /*return the SHA result*/

    resource_handles[remote_args->qp_num].remote_props.addr = remote_args->dest_add;
    resource_handles[remote_args->qp_num].remote_buf_len = remote_args->len;

    resource_handles[remote_args->qp_num].remote_props.rkey = remote_args->dest_key;
    // printf("rkey: %x\n", resource_handles[remote_args->qp_num].remote_props.rkey);

    // TODO: Make sure this code works once decompress works
    memcpy(resource_handles[remote_args->qp_num].buf, hashes, VLB_SIZE * 20);
    if (post_send (&resource_handles[remote_args->qp_num], IBV_WR_RDMA_WRITE))
    {
        fprintf (stderr, "failed to post SR 2\n");
        if (resources_destroy (&resource_handles[remote_args->qp_num]))
        {
            fprintf (stderr, "failed to destroy resources\n");
        }
    }
    // TODO: Check rc of command completion
    rc = poll_completion(&resource_handles[remote_args->qp_num]);

    // printf("Returning...\n");
    // TODO: make sure to free memory before returning
    free(buffer);
    free(de_buffer);
    return (&result_p);
}

char **rdmac_1_svc(rpc_args_t *remote_args, struct svc_req *req) {
    static char msg[256];
    static char *result_p;
    // printf("Entering: rmdac_1_svc\n");
    // printf("getting ready to set rdma connection for id: %d\n", remote_args->qp_num);
    in_config.tcp_port = resource_handles[remote_args->qp_num].tcp_port;
    create_res_handle(&resource_handles[remote_args->qp_num], in_config);
    //    printf("getting ready to set global rdma value\n");
    strcpy(msg, "global changed");
    /* sprintf(msg, "%d", last_resource_handle); */
    result_p = msg;
    // printf("rdmac_1_svc: Returning\n", msg);
    // TODO: create the QP handle like in rdma_queue and store in a global location for hw_1_svc to use when triggered

    return(&result_p);
}

char **aquirep_1_svc(void *remote_args, struct svc_req *req) {
    static char msg[256];
    static char *result_p;
    // printf("Entering: acquirep_svc\n");
    last_resource_handle++;  // TODO: Add check that we aren't blowing past the end of the array
    resource_handles[last_resource_handle].tcp_port = INITIAL_PORT + last_resource_handle;
    sprintf(msg, "%d, %d", last_resource_handle, resource_handles[last_resource_handle].tcp_port);
    result_p = msg;
    // printf("acquirep returning %s\n", msg);
    return(&result_p);
}
