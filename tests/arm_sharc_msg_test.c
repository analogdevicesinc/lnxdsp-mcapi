/*
 * Copyright (c) 2020, Analog Devices, Inc.  All rights reserved.
 *
 * Test: arm_sharc_msg_test
 * Description: 1. It calls all mcapi APIs supported and checks the result;
 *				2. It tests all the examples of blocking and nonblocking
 *					msgsend/msgrecv between two different endpoints on different nodes.
 * Result: It'll give passed log if test passes, otherwise it'll give failed log.
*/

#include <mcapi.h>
#include <mcapi_test.h>
#include <stdio.h>
#include <stdlib.h> /* for malloc */
#include <stdbool.h>
#include <string.h>
#include <limits.h>
#include <getopt.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>

/* One Domain can have multi Nodes */
#define DOMAIN				0

/**
 * MASTER NODE for CORE 0
 * One Node can have multi EPs (ports), use the port to create the Endpoint.
 */
#define MASTER_NODE			0		/* Master node0 has Master port1 and Master port2 (EPs)   */
#define MASTER_PORT_1		101		/* Master port1 is used to conncet with Slave port1 */
#define MASTER_PORT_2		200		/* Master port2 is used to conncet with Slave port2 */

/* SLAVE NODE for CORE 1 */
#define SLAVE_NODE_1		1		/* Slave node1 has Slave port1(EP) */
#define SLAVE_PORT_1		5		/* Slave port1 is used to connect with Master port1 */

/* SLAVE NODE for CORE 2 */
#define SLAVE_NODE_2		2		/* Slave node2 has Slave port2(EP) */
#define SLAVE_PORT_2		6		/* Slave port2 is used to connect with Master port2 */

#define CHECK_STATUS(ops, status, line, i) check_status(ops, status, line, i)
#define CHECK_SIZE(max, check, line) check_size(max, check, line)

/* Maximum size of the buffer array within the message struct */
#define BUFF_SIZE			64u

/* Maximum mode number */
#define MAX_MODE 3

/* Commands for execution, Common between ARM and SHARC Cores */
enum DSP_COMMAND {
  DSP_CMD_TERMINATE = 1,
  DSP_CMD_RESPONSE,
  DSP_CMD_EXECUTE
};

struct DSP_MSG {
	enum DSP_COMMAND cmd;
	uint32_t buffSize;
	char buffer[BUFF_SIZE];
};

static mcapi_endpoint_t local_endpoint[2];
static pthread_t pthId[2];
/* flag represents if the mcapi stack should be freed */
static bool need_free = false;

struct mcapi_prams {
	mcapi_domain_t domain;
	mcapi_node_t master_node;
	mcapi_port_t master_port;
	mcapi_node_t slave_node;
	mcapi_port_t slave_port;
	int timeout;
	int test_round;
};

static bool check_size(size_t max_size, size_t check_size, unsigned line)
{
	if (check_size > max_size || check_size < 0) {
		printf("check_size:%d is out of range [0,%d]!\n",check_size, max_size);
		wrong(line);
		return false;
	}
	return true;
}

static bool check_status(char* ops, mcapi_status_t status, unsigned line, int thNum)
{
	char status_message[32];
	char print_buf[BUFF_SIZE];
	mcapi_display_status(status, status_message, 32);
	if (NULL != ops) {
		snprintf(print_buf, sizeof(print_buf), "%s:%s", ops, status_message);
		printf("Thread [%d] CHECK_STATUS---%s\n", thNum, print_buf);
	}
	if ((status != MCAPI_PENDING) && (status != MCAPI_SUCCESS)) {
		wrong(line);
		return false;
	}

	return true;
}

void wrong(unsigned line)
{
	fprintf(stderr,"WRONG: line=%u\n", line);
	fflush(stdout);
}

static void stop(int singno)
{
	int i;
	char * p = "therad is over";
	mcapi_status_t status;
	mcapi_endpoint_t local_ep;

	for ( i = 0; i < 2; i++ ) {
		if ( pthId[i] != NULL ) {
			pthread_cancel(pthId[i]);
			sleep (2);
			pthread_detach(pthId[i]);
		}
		local_ep = local_endpoint[i];
		if (local_ep != 0) {
			mcapi_endpoint_delete(local_ep, &status);
			CHECK_STATUS("del_ep", status, __LINE__, 0);
		}
	}

	if (need_free) {
		mcapi_finalize(&status);
		need_free = false;
	}
}

static int help(void)
{
	printf("Usage: arm_sharc_msg_test <options>\n");
	printf("\nAvailable options:\n");
	printf("\t-h,--help\t\tthis help\n");
	printf("\t-t,--timeout\t\ttimeout value in jiffies(default:10,000)\n");
	printf("\t-r,--round\t\tnumber of test round(default:100)\n");
	return 0;
}

static int send ( mcapi_endpoint_t send,
		    mcapi_endpoint_t recv,
		    struct DSP_MSG  *pxsMsg,
		    size_t send_size,
		    mcapi_status_t *status,
		    mcapi_request_t *request,
		    int mode,
		    int timeout,
		    int thNum )
{
	int priority = 1;
	size_t size;

	printf("Thread [%d] send() start......\n", thNum);
	switch (mode) {
		case 0:
		case 1:
			mcapi_msg_send_i(send,recv,pxsMsg,send_size,priority,request,status);
			if( CHECK_STATUS("send_i", *status, __LINE__, thNum) != true )
				goto send_error;

			do{
				mcapi_test(request, &size, status);
			}while(*status == MCAPI_PENDING);
			if( ( CHECK_STATUS("test", *status, __LINE__, thNum)
				 & CHECK_SIZE(send_size, size, __LINE__) ) != true )
				goto send_error;

			/* use mcapi_wait to release request */
			mcapi_wait(request, &size, timeout, status);

			if( ( CHECK_STATUS("wait", *status, __LINE__, thNum)
				& CHECK_SIZE(send_size, size, __LINE__) ) != true )
				goto send_error;

			break;
		case 2:
			mcapi_msg_send_i(send,recv,pxsMsg,send_size,priority,request,status);
			if( CHECK_STATUS("send_i", *status, __LINE__, thNum) != true)
				goto send_error;

			mcapi_wait(request,&size,timeout,status);
			if( ( CHECK_STATUS("wait", *status, __LINE__, thNum)
				& CHECK_SIZE(send_size, size, __LINE__) ) != true )
				goto send_error;

			break;
		case 3:
			mcapi_msg_send(send, recv, pxsMsg, send_size, priority, status);
			if( CHECK_STATUS("send", *status, __LINE__, thNum) != true )
				goto send_error;

			break;
		default:
			printf("Thread [%d] Invalid mode value: %d\
					\nIt should be in the range of 0 to 3\n", mode);
			wrong(__LINE__);
			goto send_error;
	}
	printf("Thread [%d] end of send() - endpoint=%i has sent: [%s]\n",
				thNum, (int)send, pxsMsg->buffer);
	return 0;

send_error:
	printf("Thread [%d] send error! \n", thNum );
	return -1;
}

static int recv (	mcapi_endpoint_t recv,
			struct DSP_MSG *pxrMsg,
			size_t recv_size,
			mcapi_status_t *status,
			mcapi_request_t *request,
			int mode,
			int timeout,
			int thNum)
{
	size_t size;
	mcapi_uint_t avail;
	if (pxrMsg == NULL) {
		printf("Thread [%d] recv() Invalid message ptr\n", thNum);
		wrong(__LINE__);
	}
	printf("Thread [%d] recv() start......\n", thNum);
	switch (mode) {
		case 0:
			do {
				avail = mcapi_msg_available(recv, status);
			} while(avail <= 0);
			if( CHECK_STATUS("available", *status, __LINE__, thNum) != true )
			   goto recv_error;

			mcapi_msg_recv_i(recv, pxrMsg, recv_size, request, status);
			if( CHECK_STATUS("recv_i", *status, __LINE__, thNum) != true )
			   goto recv_error;

			/* use mcapi_wait to release request */
			mcapi_wait(request, &size, timeout, status);
			if( ( CHECK_STATUS("wait", *status, __LINE__, thNum)
				& CHECK_SIZE(recv_size, size, __LINE__) ) != true )
			   goto recv_error;

			break;
		case 1:
			mcapi_msg_recv_i(recv, pxrMsg, recv_size, request, status);
			if( CHECK_STATUS("recv_i", *status, __LINE__, thNum) != true )
			   goto recv_error;
			do {
				mcapi_test(request, &size, status);
			} while(*status == MCAPI_PENDING);
			if( ( CHECK_STATUS("test", *status, __LINE__, thNum)
				& CHECK_SIZE(recv_size, size, __LINE__) ) != true )
			   goto recv_error;

			/* use mcapi_wait to release request */
			mcapi_wait(request, &size, timeout, status);
			if( ( CHECK_STATUS("wait", *status, __LINE__, thNum)
				& CHECK_SIZE(recv_size, size, __LINE__) ) != true )
			   goto recv_error;

			break;
		case 2:
			mcapi_msg_recv_i(recv, pxrMsg, recv_size, request, status);
			if( CHECK_STATUS("recv_i", *status, __LINE__, thNum) != true )
			   goto recv_error;

			mcapi_wait(request, &size, timeout, status);
			if( ( CHECK_STATUS("wait", *status, __LINE__, thNum)
				& CHECK_SIZE(recv_size, size, __LINE__) ) != true )
			   goto recv_error;

			break;
		case 3:
			mcapi_msg_recv(recv, pxrMsg, recv_size, &size, status);
			if( ( CHECK_STATUS("recv", *status, __LINE__, thNum)
				& CHECK_SIZE(recv_size, size, __LINE__) ) != true )
			   goto recv_error;

			break;
		default:
			printf("Thread [%d] Invalid mode value: %d\
					\nIt should be in the range of 0 to 3\n",thNum, mode);
			wrong(__LINE__);
			goto recv_error;
	}
	pxrMsg->buffer[pxrMsg->buffSize] = '\0';
	printf("Thread [%d] end of recv() - endpoint=%i size 0x%x has received: [%s]\n",
				thNum, (int)recv, size, pxrMsg->buffer);
	return 0;

recv_error:
	printf("Thread [%d] receive error!", thNum);
	return -1;
}

static int mcapi_msg_trans(int thNum, struct mcapi_prams * pPrams)
{
	struct mcapi_prams *pthPrams = pPrams;
	int ret, thNuM = thNum;
	int mode, timeout, test_round;
	mcapi_domain_t domain;
	mcapi_node_t master_node, slave_node;
	mcapi_port_t master_port, slave_port;
	mcapi_endpoint_t local_ep, remote_ep;
	mcapi_status_t status;
	mcapi_request_t request;

	struct DSP_MSG xsMsg;
	struct DSP_MSG xrMsg;
	char cmp_buf[BUFF_SIZE];
	uint32_t round = 0, pass_num = 0;
	size_t size;

	domain		= pthPrams->domain;
	master_node = pthPrams->master_node;
	master_port = pthPrams->master_port;

	slave_node  = pthPrams->slave_node;
	slave_port  = pthPrams->slave_port;

	timeout		= pthPrams->timeout;
	test_round	= pthPrams->test_round;

	/* create endpoints */
	local_endpoint[slave_node-1] = mcapi_endpoint_create(master_port, &status);
	local_ep = local_endpoint[slave_node-1];
	if (CHECK_STATUS("create_ep", status, __LINE__, thNum) != true)
		return -1;

	printf("Thread [%d] local endpoint: %x\n", thNum, local_ep);

	remote_ep = mcapi_endpoint_get(domain,
									slave_node,
									slave_port,
									timeout,
									&status);

	if ( CHECK_STATUS("get_ep", status, __LINE__, thNum) != true)
		goto end_test;

	mcapi_endpoint_get_i(domain,
						 slave_node,
						 slave_port,
						 &remote_ep,
						 &request,
						 &status);
	if (CHECK_STATUS("get_ep_i", status,   __LINE__, thNum) != true)
		goto end_test;

	do {
		mcapi_test(&request, &size, &status);
	} while(status == MCAPI_PENDING);
	if ( CHECK_STATUS("test", status, __LINE__, thNum) != true )
		goto end_test;
	mcapi_wait(&request, &size, timeout, &status);
	if ( CHECK_STATUS("wait", status, __LINE__, thNum) != true )
		goto end_test;

	printf("Thread [%d] remote endpoint: %x\n", thNum, remote_ep);

	/* send and recv messages on the endpoints */
	/* start mcapi test process */
	for ( round = 0; round < test_round; round++ ) {
		mode = round % MAX_MODE;
		xsMsg.cmd = DSP_CMD_RESPONSE;
		xsMsg.buffSize = BUFF_SIZE;
		snprintf( xsMsg.buffer,
				  xsMsg.buffSize,
				  "hello core %d message from core %d - %u",
				  slave_node,
				  master_node,
				  round );
		xsMsg.buffSize = strlen( xsMsg.buffer );

		send( local_ep, remote_ep, &xsMsg, sizeof(struct DSP_MSG),
			  &status, &request, mode, timeout, thNum );

		printf( "Thread [%d] core0: mode(%d) message send. The %d time sending\n",
					thNum, mode, round );

		recv( local_ep, &xrMsg, sizeof(struct DSP_MSG),
			  &status, &request, mode, timeout, thNum );

		snprintf( cmp_buf,
				  BUFF_SIZE,
				  "hello core %d message from core %d - %u",
				  master_node,
				  slave_node,
				  round + 1 );
		cmp_buf[strlen(cmp_buf)] = '\0';
		xrMsg.buffer[xrMsg.buffSize] = '\0';

		if ( !strncmp(xrMsg.buffer, cmp_buf, strlen(cmp_buf)) ) {
			pass_num++;
			printf( "Thread [%d] core0: mode(%d) message recv. The %d time receiving\n",
					thNum, mode, round );
		} else
			printf( "Thread [%d] core0: mode(%d), The %d time message recv failed\n",
					thNum, mode, round );

		pthread_testcancel();
	}

end_test:

	mcapi_endpoint_delete( local_ep, &status);
	if( CHECK_STATUS("del_ep", status, __LINE__, thNum) != true )
		return  -1;
	else
		local_endpoint[slave_node - 1] = 0x0;

	if (pass_num == test_round) {
		printf("Thread [%d] core0 %d rounds mode(%d) Test PASSED!!\n",
					thNum, test_round, mode);
		return 0;
	}
	else {
		printf("Thread [%d] core0 only %d rounds mode(%d) test passed, Test FAILED!!\n",
					thNum, pass_num, mode);
		return -1;
	}
}

static void *thread_trans_fun( void* arg )
{
	struct mcapi_prams *pthPrams = (struct mcapi_prams *) arg;
	int ret = 0;
	int thNum = (int) pthPrams->slave_node;

	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,NULL);
	pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED,NULL);

	ret = mcapi_msg_trans(thNum, pthPrams);
	if (ret)
		printf("Thread [%d] test failed: %d\n",thNum, ret);
	else
		printf("Thread [%d] test success\n", thNum);
}

int main (int argc, char *argv[])
{
	mcapi_status_t status;
	mcapi_param_t parms;
	mcapi_info_t version;
	unsigned int timeout = 10*1000;
	unsigned int test_round = 100;
	const char short_options[] = "ht:r:";
	const struct option long_options[] = {
		{"help", 0, NULL, 'h'},
		{"timeout", 1, NULL, 't'},
		{"round", 1, NULL, 'r'},
		{NULL, 0, NULL, 0},
	};

	struct mcapi_prams pthPrams[2];
	int ret = 0;

	signal(SIGHUP, stop);
	signal(SIGINT, stop);
	signal(SIGQUIT, stop);
	signal(SIGTERM, stop);
	on_exit(stop, NULL);

	if (!strstr(argv[0], "arm_sharc_msg_test")) {
		printf("command should be named arm_sharc_msg_test\n");
		return -1;
	}

	while (1) {
		int c;
		if ((c = getopt_long(argc, argv, short_options, long_options, NULL)) < 0)
			break;
		switch (c) {
		case 'h':
			help();
			return 0;
		case 't':
			timeout = strtol(optarg, NULL, 0);
			break;
		case 'r':
			test_round = strtol(optarg, NULL, 0);
			break;
		default:
			printf("Invalid switch or option needs an argument.\
				  \ntry arm_sharc_msg_test --help for more information.\n");
			return -1;
		}
	}

	if (timeout <= 0) {
		printf("Invalid timeout value: %d\
			  \nIt should be in the range of 1 to MCA_INFINITE\n", timeout);
		return -1;
	}

	if (test_round <= 0 || test_round > UINT_MAX) {
		printf("Invalid test round value: %d\
			  \nIt should be in the range of 1 to UINT_MAX[0x%x] \n",\
			  test_round, UINT_MAX);
		return -1;
	}

	/* create a node for core 0*/
	mcapi_initialize(DOMAIN, MASTER_NODE, NULL, &parms, &version, &status);
	if ( CHECK_STATUS("initialize", status, __LINE__, 0) != true
				&& ( status != MCAPI_ERR_NODE_INITIALIZED ) )
		return -1;

	need_free = true;

	/* init the mcapi_prams and create the thread1 for comms with slave core 1*/
	pthPrams[0].domain		= DOMAIN;
	pthPrams[0].master_node = MASTER_NODE;
	pthPrams[0].master_port	= MASTER_PORT_1;
	pthPrams[0].slave_node	= SLAVE_NODE_1;
	pthPrams[0].slave_port	= SLAVE_PORT_1;
	pthPrams[0].timeout		= timeout;
	pthPrams[0].test_round	= test_round;

	ret = pthread_create(&pthId[0], NULL, thread_trans_fun, (void*)&pthPrams[0]);
	if (ret) {
		printf("create thread1 failed:%d \n", ret);
		goto out;
	}

	/* init the mcapi_prams and create the thread2 for comms with slave core 2*/
	pthPrams[1].domain		= DOMAIN;
	pthPrams[1].master_node = MASTER_NODE;
	pthPrams[1].master_port	= MASTER_PORT_2;
	pthPrams[1].slave_node	= SLAVE_NODE_2;
	pthPrams[1].slave_port	= SLAVE_PORT_2;
	pthPrams[1].timeout		= timeout;
	pthPrams[1].test_round	= test_round;

	ret = pthread_create(&pthId[1], NULL,
				thread_trans_fun, (void*)&pthPrams[1]);
	if (ret) {
		printf("create thread2 failed:%d \n", ret);
		goto out;
	}

	ret = pthread_join(pthId[0], NULL);
	if (ret) {
		printf("error join thread1\n");
		goto out;
	}

	ret = pthread_join(pthId[1], NULL);
	if (ret) {
		printf("error join thread2\n");
		goto out;
	}

out:
	return ret;
}
