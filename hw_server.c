#include <rpc/rpc.h>
#include "hw.h"
#include <infiniband/verbs.h>

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
    int sock;                     /* TCP socket file descriptor */
};

static void
resources_init (struct resources *res)
{
    memset (res, 0, sizeof *res);
    res->sock = -1;
}

//./rdma_server -g 0 -d mlx5_0
struct config_t in_config = {
    "mlx5_0",                     /* dev_name */
    NULL,                         /* server_name */
    19875,                        /* tcp_port */
    1,                            /* ib_port */
    0                             /* gid_idx */
};


int test_param=5;
struct resources res;


char **hw_1_svc(rpc_args_t *a, struct svc_req *req) {
	static char msg[256];
	static char *p;
	int rc = 0;
	rpc_args_t* remote_args = (rpc_args_t*)a;

	printf("getting ready to return value\n");
	printf("given queue number=0x%x\n", remote_args->qp_num);

	printf("> > > > > Start post_send");
	res.remote_props.addr = remote_args->src_add;

	/*
	 * Allocate buffer by size of remote len
	 */
    res.buf = (char *) malloc (remote_args->len);
    if (!res.buf)
    {
        fprintf (stderr, "failed to malloc %Zu bytes to memory buffer\n", remote_args->len);
        rc = 1;
        return rc;
    }
    memset (res.buf, 0, remote_args->len);
    if (post_send (&res, IBV_WR_RDMA_READ))
    {
        fprintf (stderr, "failed to post SR 2\n");
        rc = 1;
        if (resources_destroy (&res))
        {
            fprintf (stderr, "failed to destroy resources\n");
            rc = 1;
        }
    }
    if (rc)
	{
        printf("Failed post_send");
	}
    else {
	printf("Buffer: %s", res.buf);
    }
//
//
//	strcpy(msg, "Hello world");
//	p = msg;
//	printf("Returning...\n");

    return rc;
}

char **rdmac_1_svc(void *a, struct svc_req *req) {
	static char msg[256];
	static char *p;
//	resources_init (&res);
	printf("getting ready to set rdma connection\n");
	create_res_handle(&res, in_config);
	test_param += 1;
	printf("getting ready to set global rdma value\n");
//	strcpy(msg, "global changed");
	sprintf(msg, "global changed to %d", test_param);
	p = msg;
	printf("Returning...\n");
	// TODO: create the QP handle like in rdma_queue and store in a global location for hw_1_svc to use when triggered

	return(&p);
}
