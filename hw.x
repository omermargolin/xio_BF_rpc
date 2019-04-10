struct rpc_args_t
{

    uint64_t src_add;           	  /* Source address of the buffer */
    int len;                  	  /* length of the source buffer*/
    uint64_t dest_add;           	  /* Destination address for the rdma result */
    uint32_t qp_num;      	      /* qid to use */
    uint32_t dest_key;
    char device_name[11];          /*device name to read from and write to */
    uint32_t lba;
    uint32_t num_lbas;
    uint32_t dest_lba;
    
};

program HELLO_WOLRD_PROG {
	version HELLO_WORLD_VERS {
		string HW(rpc_args_t) = 1;
	} = 1;
} = 0x30000824;

program RDMA_CONNECT_PROG {
	version RDMA_CONNECT_VERS {
		string RDMAC(rpc_args_t) = 1;
	} = 1;
} = 0x30000825;

program AQUIRE_TCP_PORT_PROG {
	version AQUIRE_TCP_PORT_VERS {
		string AQUIREP(void) = 1;
	} = 1;
} = 0x30000826;
