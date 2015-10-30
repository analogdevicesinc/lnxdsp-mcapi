/* Test: msg1
Description: Tests simple blocking msgsend and msgrecv calls to several endpoints
on a single node 
*/

#include <mcapi.h>
#include <mca.h>
#include <mcapi_test.h>
#include <stdio.h>
#include <stdlib.h> /* for malloc */
#include <string.h>
#include <mcapi_impl_spec.h>
#include <getopt.h>


#define NUM_SIZES 1
#define BUFF_SIZE 64
#define DOMAIN 0
#define NODE 0

#define MCAPI_AUDIO_VOL 0x81

struct mcapi_audio_cmd {
	uint8_t cmd_head;
	int8_t ret;
	uint8_t nargs;
	uint8_t arg0;
	uint8_t arg1;
	uint8_t arg2;
	uint8_t arg3;
	uint8_t arg4;
	uint8_t arg5;
};

#define WRONG wrong(__LINE__);
void wrong(unsigned line)
{
	fprintf(stderr,"WRONG: line=%u\n", line);
	fflush(stdout);
	exit(1);
}
static uint8_t cmd_buf[BUFF_SIZE];
static uint8_t ret_buf[BUFF_SIZE];

__attribute__ ((noreturn))
static void show_usage(int exit_status)
{
        printf(
                "\nUsage: arm_sharc_audio_vol [options] \n"
                "\n"
                "Options:\n"
                        "-v volume\n"
                        "-h help\n"
        );
        exit(exit_status);
}

#define GETOPT_FLAGS "v:h"
#define a_argument required_argument
static struct option const long_opts[] = {
        {"volume",        no_argument, NULL, 'v'},
        {"help",        no_argument, NULL, 'h'},
        {NULL,          no_argument, NULL, 0x0}
};


int main (int argc, char **argv) {
	mcapi_status_t status;
	mcapi_param_t parms;
	mcapi_info_t version;
	mcapi_endpoint_t ep1,ep2;
	int i,s = 0, rc = 0,pass_num=0;
	mcapi_uint_t avail;
	int c;
	int vol;
	struct mcapi_audio_cmd *cmd;
	mcapi_request_t request;

	/* create a node */
	mcapi_initialize(DOMAIN,NODE,NULL,&parms,&version,&status);
	if (status != MCAPI_SUCCESS) { WRONG }

	/* create endpoints */
	ep1 = mcapi_endpoint_create(MASTER_PORT_NUM1,&status);
	if (status != MCAPI_SUCCESS) { WRONG }
	printf("ep1 %x   \n", ep1);

	ep2 = mcapi_endpoint_get (DOMAIN,SLAVE_NODE_NUM,SLAVE_PORT_NUM1,MCA_INFINITE,&status);
	if (status != MCAPI_SUCCESS) { WRONG }
	printf("ep2 %x   \n", ep2);

	/* send and recv messages on the endpoints */
	/* regular endpoints */
	cmd = (struct mcapi_audio_cmd*)cmd_buf;

	while ((c=getopt_long(argc, argv, GETOPT_FLAGS, long_opts, NULL)) != -1) {
		switch (c) {
			case 'v':
				vol = atoi(optarg);
				printf("%s %d\n", argv[0], vol);
				cmd->cmd_head = MCAPI_AUDIO_VOL;
				cmd->nargs = 2;
				cmd->arg0 = vol;
				cmd->arg1 = vol;
				break;
			case 'h':
				show_usage(1);
				break;
			default:
				show_usage(1);

		}
	}


	mcapi_msg_send_i(ep1, ep2, cmd_buf, sizeof(struct mcapi_audio_cmd), 1, &request, &status);
	if (status != MCAPI_SUCCESS) { WRONG }
	printf("core 0: Sending audio cmd%x %x %x %x\n", cmd->cmd_head, cmd->nargs, cmd->arg0, cmd->arg1);

	cmd = (struct mcapi_audio_cmd*)ret_buf;

	while (1) {
		avail = mcapi_msg_available(ep1, &status);
		if (avail > 0) {
			mcapi_msg_recv_i(ep1, ret_buf, BUFF_SIZE, &request, &status);
  			if (status != MCAPI_SUCCESS) { WRONG }
			if (cmd->ret == 1)
       			        printf("Core 0 : Audio cmd is executed by core 1. ret=%d\n", cmd->ret);
			else
       	        		printf("Core 0 : Audio cmd failed by core 1. ret=%d\n", cmd->ret);
	               	break;
		}
		sleep(1);
	}


	mcapi_endpoint_delete(ep1,&status);

	mcapi_finalize(&status);

	return 0;
}
