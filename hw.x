struct rpc_args_t
{

    uint64_t *src_add;           	  /* Source address of the buffer */
    int len;                  	  /* length of the source buffer*/
    char *dest_add;           	  /* Destination address for the rdma result */
    uint32_t qp_num;      	      /* qid to use */
};

program HELLO_WOLRD_PROG {
	version HELLO_WORLD_VERS {
		string HW(rpc_args_t) = 1;
	} = 1;
} = 0x30000824;

program RDMA_CONNECT_PROG {
	version RDMA_CONNECT_VERS {
		string RDMAC(void) = 1;
	} = 1;
} = 0x30000825;
