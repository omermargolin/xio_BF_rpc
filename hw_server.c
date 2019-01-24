#include <rpc/rpc.h>
#include "hw.h"
#include <infiniband/verbs.h>

#define MAX_RESOURCE_HANDLES 20
//dumb
struct cm_con_data_t
{
    uint64_t addr;                /* Buffer address */
    uint32_t rkey;                /* Remote key */
    uint32_t qp_num;              /* QP number */
    uint16_t lid;                 /* LID of the IB port */
    uint8_t gid[16];              /* gid */
} __attribute__ ((packed));


struct config_t
{
    const char *dev_name;         /* IB device name */
    char *server_name;            /* server host name */
    u_int32_t tcp_port;           /* server TCP port */
    int ib_port;                  /* local IB port to work with */
    int gid_idx;                  /* gid index to use */
};

struct resources
{
    struct ibv_device_attr device_attr;
    /* Device attributes */
    struct ibv_port_attr port_attr;       /* IB port attributes */
    struct cm_con_data_t remote_props;    /* values to connect to remote side */
    struct ibv_context *ib_ctx;   /* device handle */
    struct ibv_pd *pd;            /* PD handle */
    struct ibv_cq *cq;            /* CQ handle */
    struct ibv_qp *qp;            /* QP handle */
    struct ibv_mr *mr;            /* MR handle for buf */
    char *buf;                    /* memory buffer pointer, used for RDMA and send
                                     ops */
    uint64_t remote_buf_len;	  /* Remote Buffer length */

    int sock;                     /* TCP socket file descriptor */
    u_int32_t tcp_port;           /* TCP port for this connection */
};
struct resources resource_handles[MAX_RESOURCE_HANDLES];
int last_resource_handle = -1;



//./rdma_server -g 0 -d mlx5_0
struct config_t in_config = {
    "mlx5_0",                     /* dev_name */
    NULL,                         /* server_name */
    20875,                        /* tcp_port */
    1,                            /* ib_port */
    0                             /* gid_idx */
};


int test_param=5;
struct resources res;


char **hw_1_svc(rpc_args_t *a, struct svc_req *req) {
	static char msg[256];
	static char *p;
	rpc_args_t* remote_args = (rpc_args_t*)a;
	printf ("Entering hw_1_svc\n");
	printf("getting ready to return value\n");
	printf("given queue number=%d\n", remote_args->qp_num);

	printf("> > > > > Start post_send");
	res.remote_props.addr = (uint64_t) remote_args->src_add;

	/*
	 * Allocate buffer by size of remote len
	 */
    res.buf = (char *) malloc (remote_args->len);
    if (!res.buf)
    {
        fprintf (stderr, "failed to malloc %Zu bytes to memory buffer\n", remote_args->len);
        strcpy(msg, "Finish Server");
  	p = msg;
    	printf("Returning...\n");
        return (&p);
    }
    memset (res.buf, 0, remote_args->len);
    if (post_send (&res, IBV_WR_RDMA_READ))
    {
        fprintf (stderr, "failed to post SR 2\n");
        if (resources_destroy (&res))
        {
            fprintf (stderr, "failed to destroy resources\n");
        }
    }
    printf("Buffer: %s", res.buf);


    strcpy(msg, "Finish Server");
    p = msg;
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
