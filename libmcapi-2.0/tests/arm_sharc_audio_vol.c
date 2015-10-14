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
	_exit(1);
}
char cmd_buf[8] = "";

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

void send (mcapi_endpoint_t send, mcapi_endpoint_t recv,char* msg,mcapi_status_t status,int exp_status) {
	int size = strlen(msg);
	int priority = 1;
	mcapi_request_t request;

	mcapi_msg_send_i(send,recv,msg,size,priority,&request,&status);
	if (status != exp_status) { WRONG}
	if (status == MCAPI_SUCCESS) {
		printf("endpoint=%i has sent: [%s]\n",(int)send,msg);
	}
}

void recv (mcapi_endpoint_t recv,mcapi_status_t status,int exp_status) {
	size_t recv_size;
	char buffer[BUFF_SIZE];
	mcapi_request_t request;
	mcapi_msg_recv_i(recv,buffer,BUFF_SIZE,&request,&status);
	if (status != exp_status) { WRONG}
	if (status == MCAPI_SUCCESS) {
		printf("endpoint=%i has received: [%s]\n",(int)recv,buffer);
	}
}

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


	send (ep1,ep2,cmd_buf,status,MCAPI_SUCCESS);
	if (status != MCAPI_SUCCESS) { WRONG }
	printf("coreA: sending audio cmd%x %x %x %x\n", cmd->cmd_head, cmd->nargs, cmd->arg0, cmd->arg1);

	mcapi_endpoint_delete(ep1,&status);

	mcapi_finalize(&status);

	return 0;
}
