/*
 ** Copyright (c) 2019, Analog Devices, Inc.  All rights reserved.
*/
/*
    send audio playing back parameter to SHARC 
*/

#include <mcapi.h>
#include <mca.h>
#include <mcapi_test.h>
#include <stdio.h>
#include <stdlib.h> /* for malloc */
#include <string.h>
#include <mcapi_impl_spec.h>
#include <getopt.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define NUM_SIZES 1
#define BUFF_SIZE 64
#define DOMAIN 0
#define NODE 0

#define MAXDATASIZE 1000
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

#define GETOPT_FLAGS "v:b:m:t:h"
#define a_argument required_argument
static struct option const long_opts[] = {
        {"volume",        no_argument, NULL, 'v'},
        {"bass",        no_argument, NULL, 'b'},
        {"mid",        no_argument, NULL, 'm'},
        {"treble",        no_argument, NULL, 't'},
        {"help",        no_argument, NULL, 'h'},
        {NULL,          no_argument, NULL, 0x0}
};


int main (int argc, char **argv) {

    int server_sockfd,server_client_sockfd;
    int recvbytes;
    struct sockaddr_in server_address;

/* creat an unnamed socket */
    if ((server_sockfd = socket(AF_INET,SOCK_STREAM,0)) < 0 ){
        printf("socket error!\n");
        return -1;
    }
/* name the socket */
    server_address.sin_family =AF_INET;
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    server_address.sin_port = htons(9734);

 /* bind the socket */
    if (bind(server_sockfd,(struct sockaddr *)&server_address,sizeof(server_address)) != 0 ){
        printf("bind error!\n");
        return -1;
    }
/* listen the socket */
    if (listen(server_sockfd,10) != 0){
        printf("listen error!\n");
        return -1;
    }

    mcapi_status_t status;
    mcapi_param_t parms;
    mcapi_info_t version;
    mcapi_endpoint_t ep1,ep2;
    int i,s = 0, rc = 0,pass_num=0;
    mcapi_uint_t avail;
    int c;
    int vol, bass, mid, treb;
    struct mcapi_audio_cmd *cmd;
    mcapi_request_t request;
    /* create a node */
    mcapi_initialize(DOMAIN,NODE,NULL,&parms,&version,&status);
    if (status != MCAPI_SUCCESS) { WRONG }

    char buf[MAXDATASIZE];
    /* accept a connection */
    if ((server_client_sockfd = accept(server_sockfd,NULL,0)) < 0 ){
            printf("accept error!\n");
	    return -1;
    }
    while(1){
	/* create endpoints */
	ep1 = mcapi_endpoint_create(MASTER_PORT_NUM1,&status);
	if (status != MCAPI_SUCCESS) { WRONG }

	ep2 = mcapi_endpoint_get (DOMAIN,SLAVE_NODE_NUM,SLAVE_PORT_NUM1,MCA_INFINITE,&status);
	if (status != MCAPI_SUCCESS) { WRONG }

	/* send and recv messages on the endpoints */
	/* regular endpoints */

        recvbytes = recv(server_client_sockfd,buf,MAXDATASIZE, 0 );
        buf[recvbytes] = '\0';
        if (recvbytes > 0){
            printf("Received: %s\n",buf);
            cmd = (struct mcapi_audio_cmd*)cmd_buf;
            const char * split = "-";
            char *p,*cmd_value;
            p = strtok(buf,split);
            while(p != NULL)
            {
            	cmd_value = &p[1];
                switch (p[0]) {
                    case 'v':
                        vol = atoi(cmd_value);
                        cmd->cmd_head = MCAPI_AUDIO_VOL;
                        cmd->arg0 = vol;
                        break;
                    case 'b':
                        bass = atoi(cmd_value);
                        cmd->arg1 = bass;
                        break;
                    case 'm':
                        mid = atoi(cmd_value);
                        cmd->arg2 = mid;
                        break;
                    case 't':
                        treb = atoi(cmd_value);
                        cmd->arg3 = treb;
                        break;
                    case 'h':
                        show_usage(1);
                        break;
                    default:
                        show_usage(1);
                 }
                 p = strtok(NULL,split);
            }
            mcapi_msg_send_i(ep1, ep2, cmd_buf, sizeof(struct mcapi_audio_cmd), 1, &request, &status);
            if (status != MCAPI_SUCCESS) { WRONG }
            printf("  griffin in tuning the sound\n");

            cmd = (struct mcapi_audio_cmd*)ret_buf;
            static int count = 0;
            while (1) {
                avail = mcapi_msg_available(ep1, &status);
                if (avail > 0) {
                     mcapi_msg_recv_i(ep1, ret_buf, BUFF_SIZE, &request, &status);
                     if (status != MCAPI_SUCCESS) { WRONG }
                     if (cmd->ret == 1)
                            printf("Core 0 : Audio cmd is executed by core 1. ret=%d\n", cmd->ret);
                     else {
                            printf("Core 0 : Audio cmd failed by core 1. ret=%d\n", cmd->ret);
	                    break;
                          }
                 }
                count ++;
                if(count>1)
                break;
            }
     	    mcapi_endpoint_delete(ep1,&status);
        }
        else if (recvbytes == 0){
   	    mcapi_finalize(&status);
	    close(server_client_sockfd);
    	    close(server_sockfd);
            break;
        }
        else {
            printf("rcv error!\n");
            break;
        }
    }
    return 0;
}
