/* Demo: arm_sharc_msg_demo
   Description: Demo shows the example use of blocking/non-blocking
				msgsend/msgrecv between two different endpoints on different nodes.
   Result: It'll give passed log if demo runs successfully, otherwise it'll give failed log.
*/

#include <mcapi.h>
#include <mcapi_test.h>
#include <stdio.h>
#include <stdlib.h> /* for malloc */
#include <string.h>
#include <getopt.h>

#define BUFF_SIZE 64
#define DOMAIN 0
#define NODE 0

#define CHECK_STATUS(ops, status, line) check_status(ops, status, line);
#define CHECK_SIZE(max, check, line) check_size(max, check, line);

#define SEND_STRING  "hello mcapi core0"
#define RECV_STRING  "hello mcapi core1"

void check_size(size_t max_size, size_t check_size, unsigned line)
{
	if (check_size > max_size || check_size < 0) {
		printf("check_size:%d is out of range [0,%d]!\n",check_size, max_size);
		wrong(line);
	}
}

void check_status(char* ops, mcapi_status_t status, unsigned line)
{
	char status_message[32];
	char print_buf[BUFF_SIZE];
	mcapi_display_status(status, status_message, 32);
	if (NULL != ops) {
		snprintf(print_buf, sizeof(print_buf), "%s:%s", ops, status_message);
		printf("CHECK_STATUS---%s\n", print_buf);
	}
	if ((status != MCAPI_PENDING) && (status != MCAPI_SUCCESS)) {
		wrong(line);
	}
}

void wrong(unsigned line)
{
	fprintf(stderr,"WRONG: line=%u\n", line);
	fflush(stdout);
	_exit(1);
}

static int help(void)
{
	printf("Usage: arm_sharc_msg_demo <options>\n");
	printf("\nAvailable options:\n");
	printf("\t-h,--help\t\tthis help\n");
	printf("\t-m,--mode\t\tselect the mode:\
		    \n\t\t\t\t0 --- nonblocking mode0(default)\
		    \n\t\t\t\t1 --- nonblocking mode1\
		    \n\t\t\t\t2 --- nonblocking mode2\
		    \n\t\t\t\t3 --- blocking mode\n");
	printf("\t-t,--timeout\t\ttimeout value in jiffies(default:5000)\n");
	printf("\t-r,--round\t\tnumber of test round(default:100)\n");
	return 0;
}

void send (mcapi_endpoint_t send, mcapi_endpoint_t recv, char *msg, size_t send_size,
	mcapi_status_t *status, mcapi_request_t *request, int mode, int timeout)
{
	int priority = 1;
	size_t size;

	printf("send() start......\n");
	switch (mode) {
		case 0:
		case 1:
			mcapi_msg_send_i(send,recv,msg,send_size,priority,request,status);
			CHECK_STATUS("send_i", *status, __LINE__);
			do{
				mcapi_test(request, &size, status);
			}while(*status == MCAPI_PENDING);
			CHECK_STATUS("test", *status, __LINE__);
			CHECK_SIZE(send_size, size, __LINE__);

			/* use mcapi_wait to release request */
			mcapi_wait(request, &size, timeout, status);
			CHECK_STATUS("wait", *status, __LINE__);
			CHECK_SIZE(send_size, size, __LINE__);
			break;
		case 2:
			mcapi_msg_send_i(send,recv,msg,send_size,priority,request,status);
			CHECK_STATUS("send_i", *status, __LINE__);
			mcapi_wait(request,&size,timeout,status);
			CHECK_STATUS("wait", *status, __LINE__);
			CHECK_SIZE(send_size, size, __LINE__);
			break;
		case 3:
			mcapi_msg_send(send, recv, msg, send_size, priority, status);
			CHECK_STATUS("send", *status, __LINE__);
			break;
		default:
			printf("Invalid mode value: %d\
					\nIt should be in the range of 0 to 3\n", mode);
			wrong(__LINE__);
	}
	printf("end of send() - endpoint=%i has sent: [%s]\n", (int)send, msg);
}

void recv (mcapi_endpoint_t recv, char *buffer, size_t buffer_size, mcapi_status_t *status,
	mcapi_request_t *request, int mode, int timeout)
{
	size_t size;
	mcapi_uint_t avail;
	if (buffer == NULL) {
		printf("recv() Invalid buffer ptr\n");
		wrong(__LINE__);
	}
	printf("recv() start......\n");
	switch (mode) {
		case 0:
			do {
				avail = mcapi_msg_available(recv, status);
			}while(avail <= 0);
			CHECK_STATUS("available", *status, __LINE__);

			mcapi_msg_recv_i(recv, buffer, buffer_size, request, status);
			CHECK_STATUS("recv_i", *status, __LINE__);
			/* use mcapi_wait to release request */
			mcapi_wait(request, &size, timeout, status);
			CHECK_STATUS("wait", *status, __LINE__);
			CHECK_SIZE(buffer_size, size, __LINE__);
			break;
		case 1:
			mcapi_msg_recv_i(recv, buffer, buffer_size, request, status);
			CHECK_STATUS("recv_i", *status, __LINE__);
			do{
				mcapi_test(request, &size, status);
			}while(*status == MCAPI_PENDING);
			CHECK_STATUS("test", *status, __LINE__);
			CHECK_SIZE(buffer_size, size, __LINE__);

			/* use mcapi_wait to release request */
			mcapi_wait(request, &size, timeout, status);
			CHECK_STATUS("wait", *status, __LINE__);
			CHECK_SIZE(buffer_size, size, __LINE__);
			break;
		case 2:
			mcapi_msg_recv_i(recv, buffer, buffer_size, request, status);
			CHECK_STATUS("recv_i", *status, __LINE__);
			mcapi_wait(request, &size, timeout, status);
			CHECK_STATUS("wait", *status, __LINE__);
			CHECK_SIZE(buffer_size, size, __LINE__);
			break;
		case 3:
			mcapi_msg_recv(recv, buffer, buffer_size, &size, status);
			CHECK_STATUS("recv", *status, __LINE__);
			CHECK_SIZE(buffer_size, size, __LINE__);
			break;
		default:
			printf("Invalid mode value: %d\
					\nIt should be in the range of 0 to 3\n", mode);
			wrong(__LINE__);
	}
	buffer[size] = '\0';
	printf("end of recv() - endpoint=%i size 0x%x has received: [%s]\n", (int)recv, size, buffer);
}

int main (int argc, char *argv[])
{
	mcapi_status_t status;
	mcapi_param_t parms;
	mcapi_info_t version;
	mcapi_endpoint_t ep1,ep2;
	char recv_buf[BUFF_SIZE];
	char cmp_buf[BUFF_SIZE];
	char send_buf[32] = "";
	int s = 0, pass_num = 0;
	size_t size;
	int mode = 0;
	int b_block = 0;
	int timeout = 5*1000;
	int test_round = 100;
	static const char short_options[] = "hm:t:r:";
	static const struct option long_options[] = {
		{"help", 0, NULL, 'h'},
		{"mode", 1, NULL, 'm'},
		{"timeout", 1, NULL, 't'},
		{"round", 1, NULL, 'r'},
		{NULL, 0, NULL, 0},
	};
	mcapi_request_t request;

	if (!strstr(argv[0], "arm_sharc_msg_demo")) {
		printf("command should be named arm_sharc_msg_demo\n");
		return 1;
	}

	while (1) {
		int c;
		if ((c = getopt_long(argc, argv, short_options, long_options, NULL)) < 0)
			break;
		switch (c) {
		case 'h':
			help();
			return 0;
		case 'm':
			mode = strtol(optarg, NULL, 0);
			break;
		case 't':
			timeout = strtol(optarg, NULL, 0);
			break;
		case 'r':
			test_round = strtol(optarg, NULL, 0);
			break;
		default:
			printf("Invalid switch or option needs an argument.\
				  \ntry arm_sharc_msg_demo --help for more information.\n");
			return 1;
		}
	}

	switch(mode) {
		case 0:
		case 1:
		case 2:
			b_block = 0;
			break;
		case 3:
			b_block = 1;
			break;
		default:
			printf("Invalid mode value: %d\
				\nIt should be in the range of 0 to 3\n",mode);
			wrong(__LINE__);
	}

	if (timeout <= 0) {
		printf("Invalid timeout value: %d\
			  \nIt should be in the range of 1 to MCA_INFINITE\n", timeout);
		return 1;
	}
	if (test_round <= 0 || test_round > 100) {
		printf("Invalid round value: %d\
			  \nIt should be in the range of 1 to 100\n", test_round);
		return 1;
	}
	/* create a node */
	mcapi_initialize(DOMAIN,NODE,NULL,&parms,&version,&status);
	CHECK_STATUS("initialize", status, __LINE__);
	/* create endpoints */
	ep1 = mcapi_endpoint_create(MASTER_PORT_NUM1,&status);
	CHECK_STATUS("create_ep", status, __LINE__);
	printf("ep1 %x   \n", ep1);
	if (b_block) {
		ep2 = mcapi_endpoint_get(DOMAIN,SLAVE_NODE_NUM,SLAVE_PORT_NUM1,
					timeout,&status);
		CHECK_STATUS("get_ep", status, __LINE__);
	} else {
		mcapi_endpoint_get_i(DOMAIN,SLAVE_NODE_NUM,SLAVE_PORT_NUM1,&ep2,&request,&status);
		CHECK_STATUS("get_ep_i", status,   __LINE__);

		if (mode == 1) {
			do{
				mcapi_test(&request, &size, &status);
			}while(status == MCAPI_PENDING);
			CHECK_STATUS("test", status, __LINE__);
		}
		mcapi_wait(&request, &size, timeout, &status);
		CHECK_STATUS("wait", status, __LINE__);
	}
	printf("ep2 %x   \n", ep2);

	/* send and recv messages on the endpoints */
	/* start demo test process */
	for (s = 0; s < test_round; s++) {
		snprintf(send_buf,sizeof(send_buf), "%s %d", SEND_STRING, s);
		send(ep1, ep2, send_buf, strlen(send_buf), &status, &request, mode, timeout);
		printf("Core0: mode(%d) message send. The %d time sending\n", mode, s);
		recv(ep1, recv_buf, sizeof(recv_buf)-1, &status, &request, mode, timeout);
		snprintf(cmp_buf, sizeof(cmp_buf), "%s %d", RECV_STRING, s+1);
		if (!strncmp(recv_buf, cmp_buf, sizeof(recv_buf))) {
			pass_num++;
			printf("Core0: mode(%d) message recv. The %d time receiving\n",mode, s);
		} else
			printf("Core0: mode(%d) %d time message recv failed\n", mode, s);
	}

	mcapi_endpoint_delete(ep1,&status);
	CHECK_STATUS("del_ep", status, __LINE__);

	mcapi_finalize(&status);
	CHECK_STATUS("finalize", status, __LINE__);

	if (pass_num == test_round)
		printf("Core0 %d rounds mode(%d) demo Test PASSED!!\n", test_round, mode);
	else
		printf("Core0 only %d rounds mode(%d) test passed, demo Test FAILED!!\n", pass_num, mode);

	return 0;
}
