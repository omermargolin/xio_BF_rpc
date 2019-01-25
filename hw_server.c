#include "hw.h"
#include "xcommon.h"
#include <infiniband/verbs.h>

#define MAX_RESOURCE_HANDLES 20


//./rdma_server -g 0 -d mlx5_0
struct config_t in_config = {
    "mlx5_0",                     /* dev_name */
    NULL,                         /* server_name */
    19875,                        /* tcp_port */
    1,                            /* ib_port */
    0                             /* gid_idx */
};

int test_param=5;
struct resources resource_handles[MAX_RESOURCE_HANDLES];
int last_resource_handle = -1;


char **hw_1_svc(rpc_args_t *a, struct svc_req *req) {
	static char msg[256];
	static char *p;
	rpc_args_t* remote_args = (rpc_args_t*)a;
	int rc;
	printf ("Entering hw_1_svc\n");
	printf("getting ready to return value\n");
	printf("given queue number=%d\n", remote_args->qp_num);
    printf("src value:%p\n", remote_args->src_add);
    printf("dest value:%p\n", remote_args->dest_add);
    printf("dest key: %x\n", remote_args->dest_key);

  // printf("address:%p\n", &remote_args->src_add);
  //printf("*address:%s\n", *remote_args->src_add);

  printf("len:%d\n", remote_args->len);

  // printf("dest val:%s\n", remote_args->dest_add);
  // printf("dest add:%p\n", &remote_args->dest_add);
  // printf("dest ref val:%s\n", *remote_args->dest_add);


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

    strcpy(msg, "Finish Server");
    p = msg;
/*And now return the SHA result*/

    resource_handles[remote_args->qp_num].remote_props.addr =  remote_args->dest_add;
    resource_handles[remote_args->qp_num].remote_buf_len = 64;
    resource_handles[remote_args->qp_num].remote_props.rkey = remote_args->dest_key;

    sprintf(resource_handles[remote_args->qp_num].buf,"12345678");
    if (post_send (&resource_handles[remote_args->qp_num], IBV_WR_RDMA_WRITE))
    {
        fprintf (stderr, "failed to post SR 2\n");
        if (resources_destroy (&resource_handles[remote_args->qp_num]))
        {
            fprintf (stderr, "failed to destroy resources\n");
        }
    }
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
	resource_handles[last_resource_handle].tcp_port = in_config.tcp_port + last_resource_handle;
	sprintf(msg, "%d, %d", last_resource_handle, in_config.tcp_port+last_resource_handle);
//	strcpy(msg, "global changed");
	p = msg;
	printf("aquirep eturning %s\n", msg);
	return(&p);
}
