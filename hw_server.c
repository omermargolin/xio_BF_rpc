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
    static char *buffer, de_buffer;
    // rpc_args_t* remote_args = (rpc_args_t*)a;
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
       return (&result_p);
    }

    de_buffer = (char *) malloc (4096);
	if (!de_buffer)
	{
	   fprintf (stderr, "failed to malloc 4096 bytes to memory buffer\n");
	   strcpy(msg, "Finish Server");
	   result_p = msg;
	   printf("Returning...\n");
	   return (&result_p);
	}
	printf("Done reading from file...\n");
    struct vlb_d vlb[VLB_SIZE] = {
      {259, true},
      {120, true},
      {4096, false},
      {120, true},
      {4096, false},
      {4096, false},
      {120, true},
      {120, true},
    };
    printf("Done mapping file...\n");

    int start_position = 0;
    char hashes[VLB_SIZE][20];
    int i;
    for (i=0; i < VLB_SIZE; i++)
    {

      //      //decompress from current pointer to len
      //      // increase current pointer by len
      //      // comput hash of returned decompressed buffer
      //      // add hash to hash result array
      //TODO: Compute hash on each 512 byte block of 4k

      if (vlb[i].compressed)
      {
    	  printf("Calling decompress with %d\n", vlb[i].len );
    	  rc = decompress_data(buffer + start_position, vlb[i].len, de_buffer, 4096);
    	  printf("returned from decompress...\n");
    	  if (rc)
    	  {
    		  fprintf (stderr, "failed to decompress data\n");
    		  strcpy(msg, "Finish Server");
    		  result_p = msg;
    		  printf("Returning...\n");
    		  return (&result_p);
    	  }
      }
      start_position += vlb[i].len;
      Sha1(de_buffer, 4096, hashes[i]);
    }

    //printf("*address:%s\n", *remote_args->src_add);
    //     printf("len:%d\n", remote_args->len);
    //     printf("dest val:%s\n", remote_args->dest_add);
    //     printf("dest add:%p\n", &remote_args->dest_add);
    //     printf("dest ref val:%s\n", *remote_args->dest_add);

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
        result_p = msg;
        printf("Returning...\n");
        return (&result_p);
    }

    memset (resource_handles[remote_args->qp_num].buf, 0, remote_args->len);
//    if (post_send (&resource_handles[remote_args->qp_num], IBV_WR_RDMA_READ))
//    {
//        fprintf (stderr, "failed to post SR 2\n");
//        if (resources_destroy (&resource_handles[remote_args->qp_num]))
//        {
//            fprintf (stderr, "failed to destroy resources\n");
//        }
//    }
//    rc = poll_completion(&resource_handles[remote_args->qp_num]);

//    printf("Buffer: %s\n", resource_handles[remote_args->qp_num].buf);

//    uint8_t result[20];  // Use 32 for sha256

//    printf("Calc SHA1\n");
//    Sha1(resource_handles[remote_args->qp_num].buf, remote_args->len, result );
    //Sha2_256(resource_handles[remote_args->qp_num].buf, remote_args->len, &result );
//    printf("Result after SHA: ");
//    int x;
//    for(x = 0; x < 128; x++) printf("%02x", result[x]);
//    putchar( '\n' );
//    printf("Result after SHA: 0x%x\n",result);
//    printf("Result after SHA: %s\n",result);
    strcpy(msg, "Finish Server");
    result_p = msg;
    /*return the SHA result*/

    resource_handles[remote_args->qp_num].remote_props.addr = remote_args->dest_add;
//    uint64_t temp_len = resource_handles[remote_args->qp_num].remote_buf_len;
    resource_handles[remote_args->qp_num].remote_buf_len = remote_args->len;
//    uint32_t rkey_temp = resource_handles[remote_args->qp_num].remote_props.rkey;

    resource_handles[remote_args->qp_num].remote_props.rkey = remote_args->dest_key;
    printf("rkey: %x\n", resource_handles[remote_args->qp_num].remote_props.rkey);

    // TODO: Make sure this code works once decompress works
    int index=0;
    uint64_t *tmp_unit64_p = resource_handles[remote_args->qp_num].buf;
    for (i=0; i < VLB_SIZE; i++)
    {

    	while(index < 20)
    	{
    	  *tmp_unit64_p = hashes[i][index];
    	  tmp_unit64_p++;
    	  index++;
    	}
    	index = 0;
    }
//    sprintf(resource_handles[remote_args->qp_num].buf, result);
    if (post_send (&resource_handles[remote_args->qp_num], IBV_WR_RDMA_WRITE))
    {
        fprintf (stderr, "failed to post SR 2\n");
        if (resources_destroy (&resource_handles[remote_args->qp_num]))
        {
            fprintf (stderr, "failed to destroy resources\n");
        }
    }
//    resource_handles[remote_args->qp_num].remote_props.rkey = rkey_temp;
//    resource_handles[remote_args->qp_num].remote_buf_len = temp_len;
//    printf("restored rkey: %x\n", resource_handles[remote_args->qp_num].remote_props.rkey);
//    sleep(2);

    printf("Returning...\n");
    // TODO: make sure to free memory before returning
    // free(buffer);
    // free(de_buffer);
    return (&result_p);
}

char **rdmac_1_svc(rpc_args_t *remote_args, struct svc_req *req) {
    static char msg[256];
    static char *result_p;
    //    resources_init (&res);
    printf("Entering: rmdac_1_svc\n");
    // rpc_args_t* remote_args = (rpc_args_t*)args;

    printf("getting ready to set rdma connection for id: %d\n", remote_args->qp_num);
    in_config.tcp_port = resource_handles[remote_args->qp_num].tcp_port;
    create_res_handle(&resource_handles[remote_args->qp_num], in_config);
    //    printf("getting ready to set global rdma value\n");
    strcpy(msg, "global changed");
    /* sprintf(msg, "%d", last_resource_handle); */
    result_p = msg;
    printf("rdmac_1_svc: Returning\n", msg);
    // TODO: create the QP handle like in rdma_queue and store in a global location for hw_1_svc to use when triggered

    return(&result_p);
}

char **aquirep_1_svc(void *remote_args, struct svc_req *req) {
    static char msg[256];
    static char *result_p;
//    resources_init (&res);
    printf("Entering: acquirep_svc\n");
    last_resource_handle++;  // TODO: Add check that we aren't blowing past the end of the array
    resource_handles[last_resource_handle].tcp_port = INITIAL_PORT + last_resource_handle;
    sprintf(msg, "%d, %d", last_resource_handle, resource_handles[last_resource_handle].tcp_port);
//    strcpy(msg, "global changed");
    result_p = msg;
    printf("acquirep returning %s\n", msg);
    return(&result_p);
}
