#include <rpc/rpc.h>
#include "hw.h"
#include <infiniband/verbs.h>

#define MAX_RESOURCE_HANDLES 20

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
struct resources resource_handles[MAX_RESOURCE_HANDLES];
int last_resource_handle = 0;



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
	rpc_args_t* remote_args = (rpc_args_t*)a;
	printf ("Entering hw_1_svc\n");
	printf("getting ready to return value\n");
	printf("given queue number=%d\n", remote_args->qp_num);
	strcpy(msg, "Hello world");
	p = msg;
	printf("Returning...\n");
	printf ("Exiting hw_1_svc\n");
	return(&p);
}

char **rdmac_1_svc(void *a, struct svc_req *req) {
	static char msg[256];
	static char *p;
//	resources_init (&res);
	printf("Entering: rmdac_1_svc\n");
	last_resource_handle++;  // TODO: Add check that we aren't blowing past the end of the array
	printf("getting ready to set rdma connection for id: %d\n", last_resource_handle);
	create_res_handle(&resource_handles[last_resource_handle], in_config);
	printf("getting ready to set global rdma value\n");
//	strcpy(msg, "global changed");
	sprintf(msg, "%d", last_resource_handle);
	p = msg;
	printf("Returning qp_id %s\n", msg);
	// TODO: create the QP handle like in rdma_queue and store in a global location for hw_1_svc to use when triggered
	printf("Exiting: rmdac_1_svc\n");
	return(&p);
}
