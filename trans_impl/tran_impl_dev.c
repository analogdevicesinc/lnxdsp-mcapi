/*
 ** Copyright (c) 2019, Analog Devices, Inc.  All rights reserved.
*/

#include <tls.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <mcapi.h>
#include <transport_sm.h>
#include <icc.h>
#include <assert.h>

int fd;
extern mcapi_database* c_db;

int sm_dev_initialize()
{
	fd = open("/dev/icc", O_RDWR);
	if (fd < 0) {
		perror("unable to open /dev/icc");
	}
	return fd;
}

void sm_dev_finalize()
{
	close(fd);
}

int sm_create_session(uint32_t src_ep, uint32_t type)
{
	int ret;
	struct sm_packet pkt;

	memset(&pkt, 0, sizeof(pkt));
	pkt.local_ep = src_ep;
	pkt.type = type;
	ret = ioctl(fd, CMD_SM_CREATE, &pkt);
	return pkt.session_idx;
}

int sm_destroy_session(uint32_t session_idx)
{
	int ret;
	struct sm_packet pkt;

	memset(&pkt, 0, sizeof(struct sm_packet));
	pkt.session_idx = session_idx;
	ret = ioctl(fd, CMD_SM_SHUTDOWN, &pkt);
	return ret;
}

int sm_connect_session(uint32_t session_idx, uint32_t dst_ep, uint32_t dst_cpu, uint32_t type)
{
	int ret;
	struct sm_packet pkt;

	memset(&pkt, 0, sizeof(struct sm_packet));
	pkt.session_idx = session_idx;
	pkt.remote_ep = dst_ep;
	pkt.dst_cpu = dst_cpu;
	pkt.type = type;
	ret = ioctl(fd, CMD_SM_CONNECT, &pkt);
	return ret;
}

int sm_disconnect_session(uint32_t session_idx, uint32_t dst_ep, uint32_t dst_cpu)
{
	int ret;
	struct sm_packet pkt;

	memset(&pkt, 0, sizeof(struct sm_packet));
	pkt.session_idx = session_idx;
	pkt.remote_ep = dst_ep;
	pkt.dst_cpu = dst_cpu;
	ret = ioctl(fd, CMD_SM_CONNECT, &pkt);
	return ret;
}

int sm_open_session(uint32_t session_idx)
{
	int ret;
	struct sm_packet pkt;

	memset(&pkt, 0, sizeof(struct sm_packet));
	pkt.session_idx = session_idx;
	ret = ioctl(fd, CMD_SM_OPEN, &pkt);
	return ret;
}

int sm_close_session(uint32_t session_idx)
{
	int ret;
	struct sm_packet pkt;

	memset(&pkt, 0, sizeof(struct sm_packet));
	pkt.session_idx = session_idx;
	ret = ioctl(fd, CMD_SM_CLOSE, &pkt);
	return ret;
}

int sm_send_packet(uint32_t session_idx, uint32_t dst_ep, uint32_t dst_cpu,
		void *buf, uint32_t len, uint32_t *payload, int blocking)
{
	int ret;
	int flags;
	struct sm_packet pkt;

	memset(&pkt, 0, sizeof(struct sm_packet));
	pkt.session_idx = session_idx;
	pkt.remote_ep = dst_ep;
	pkt.dst_cpu = dst_cpu;
	pkt.buf_len = len;
	pkt.buf = buf;
	if (flags = fcntl(fd, F_GETFL, 0) > 0) {
		if (blocking)
			flags &= ~O_NONBLOCK;
		else
			flags |= O_NONBLOCK;
		fcntl(fd, F_SETFL, flags);
	}
	ret = ioctl(fd, CMD_SM_SEND, &pkt);
	if (payload)
		*payload = pkt.payload;
	return ret;
}

int sm_recv_packet(uint32_t session_idx, uint16_t *dst_ep, uint16_t *dst_cpu,
		void *buf, uint32_t *len, int blocking)
{
	int ret = 0;
	int flags;
	struct sm_packet pkt;

	memset(&pkt, 0, sizeof(struct sm_packet));
	pkt.session_idx = session_idx;
	if (buf)
		pkt.buf = buf;
	else
		return -EINVAL;
	if (len)
		pkt.buf_len = *len;

	if (flags = fcntl(fd, F_GETFL, 0) > 0) {
		if (blocking)
			flags &= ~O_NONBLOCK;
		else
			flags |= O_NONBLOCK;
		fcntl(fd, F_SETFL, flags);
	}
	ret = ioctl(fd, CMD_SM_RECV, &pkt);
	if (!ret) {
		if (dst_ep)
			*dst_ep = pkt.remote_ep;
		if (dst_cpu)
			*dst_cpu = pkt.dst_cpu;
		if (len)
			*len = pkt.buf_len;
	}
	return ret;
}

int sm_send_scalar(uint32_t session_idx, uint16_t dst_ep, uint16_t dst_cpu,
		uint32_t scalar0, uint32_t scalar1, uint32_t size, int blocking)
{
	int ret;
	int flags;
	struct sm_packet pkt;

	memset(&pkt, 0, sizeof(struct sm_packet));
	pkt.session_idx = session_idx;
	pkt.remote_ep = dst_ep;
	pkt.dst_cpu = dst_cpu;

	pkt.buf_len = scalar1;
	pkt.buf = scalar0;
	switch (size) {
	case 1:
		pkt.type = SM_SESSION_SCALAR_READY_8;
		break;
	case 2:
		pkt.type = SM_SESSION_SCALAR_READY_16;
		break;
	case 4:
		pkt.type = SM_SESSION_SCALAR_READY_32;
		break;
	case 8:
		pkt.type = SM_SESSION_SCALAR_READY_64;
		break;
	}
	if (flags = fcntl(fd, F_GETFL, 0) > 0) {
		if (blocking)
			flags &= ~O_NONBLOCK;
		else
			flags |= O_NONBLOCK;
		fcntl(fd, F_SETFL, flags);
	}
	ret = ioctl(fd, CMD_SM_SEND, &pkt);
	return ret;
}

int sm_recv_scalar(uint32_t session_idx, uint16_t *src_ep, uint16_t *src_cpu,
		uint32_t *scalar0, uint32_t *scalar1, uint32_t *size, int blocking)
{
	int ret = 0;
	int flags;
	struct sm_packet pkt;

	memset(&pkt, 0, sizeof(struct sm_packet));
	pkt.session_idx = session_idx;
	pkt.type = SM_SESSION_SCALAR_READY_64;
	if (flags = fcntl(fd, F_GETFL, 0) > 0) {
		if (blocking)
			flags &= ~O_NONBLOCK;
		else
			flags |= O_NONBLOCK;
		fcntl(fd, F_SETFL, flags);
	}
	ret = ioctl(fd, CMD_SM_RECV, &pkt);
	if (ret)
		return ret;
	if (src_ep)
		*src_ep = pkt.local_ep;
	if (src_cpu)
		*src_cpu = pkt.src_cpu;
	if (scalar0)
		*scalar0 = pkt.buf;
	if (scalar1)
		*scalar1 = pkt.buf_len;
	if (size)
		*size = pkt.type;
	return 1;
}

int sm_get_remote_ep(uint32_t dst_ep, uint32_t dst_cpu, int timeout, int blocking)
{
	int ret;
	int flags;
	struct sm_packet pkt;

	memset(&pkt, 0, sizeof(struct sm_packet));
	pkt.remote_ep = dst_ep;
	pkt.dst_cpu = dst_cpu;
	pkt.timeout = timeout;
	if (flags = fcntl(fd, F_GETFL, 0) > 0) {
		if (blocking)
			flags &= ~O_NONBLOCK;
		else
			flags |= O_NONBLOCK;
		fcntl(fd, F_SETFL, flags);
	}
	ret = ioctl(fd, CMD_SM_QUERY_REMOTE_EP, &pkt);
	return ret;
}

int sm_get_session_status(uint32_t session_idx, struct sm_session_status *status)
{
	int ret;
	struct sm_packet pkt;

	if (!status)
		return -EINVAL;
	memset(&pkt, 0, sizeof(struct sm_packet));
	memset(status, 0, sizeof(*status));
	pkt.session_idx = session_idx;
	pkt.param = status;
	pkt.param_len = sizeof(*status);
	ret = ioctl(fd, CMD_SM_GET_SESSION_STATUS, &pkt);
	return ret;
}

int sm_wait_nonblocking(uint32_t session_idx, uint32_t dst_ep, uint32_t dst_cpu,
	void *buf, uint32_t *len, uint32_t type, uint32_t payload, unsigned int timeout, int blocking)
{
	int ret;
	int flags;
	struct sm_packet pkt;

	memset(&pkt, 0, sizeof(struct sm_packet));
	pkt.session_idx = session_idx;
	pkt.remote_ep = dst_ep;
	pkt.dst_cpu = dst_cpu;
	pkt.buf = buf;
	pkt.type = type;
	pkt.timeout = timeout;
	pkt.payload = payload;
	if (len)
		pkt.buf_len = *len;
	if (flags = fcntl(fd, F_GETFL, 0) > 0) {
		if (blocking)
			flags &= ~O_NONBLOCK;
		else
			flags |= O_NONBLOCK;
		fcntl(fd, F_SETFL, flags);
	}

	ret = ioctl(fd, CMD_SM_WAIT, &pkt);
	if (len)
		*len = pkt.buf_len;
	return ret;
}

int sm_get_node_status(uint32_t node, uint32_t *session_mask, uint32_t *session_pending, uint32_t *nfree)
{
	int ret;
	struct sm_packet pkt;
	struct sm_node_status param;

	memset(&pkt, 0, sizeof(struct sm_packet));
	memset(&param, 0, sizeof(param));
	pkt.param = &param;
	pkt.param_len = sizeof(param);
	ret = ioctl(fd, CMD_SM_GET_NODE_STATUS, &pkt);

	if (session_mask)
		*session_mask = param.session_mask;
	if (session_pending)
		*session_pending = param.session_pending;
	if (nfree)
		*nfree = param.nfree;
	return ret;
}

void *sm_request_uncached_buf(uint32_t size, uint32_t *paddr)
{
	int ret;
	struct sm_packet pkt;

	memset(&pkt, 0, sizeof(struct sm_packet));
	pkt.type = CMD_SM_REQUEST_UNCACHED_BUF;
	pkt.buf_len = size;
	ret = ioctl(fd, CMD_SM_REQUEST_UNCACHED_BUF, &pkt);
	if (ret)
		return NULL;
	if (paddr)
		*paddr = pkt.paddr;

	return pkt.buf;
}

int sm_release_uncached_buf(void *buf, uint32_t size, uint32_t paddr)
{
	int ret;
	struct sm_packet pkt;

	memset(&pkt, 0, sizeof(struct sm_packet));
	pkt.type = CMD_SM_RELEASE_UNCACHED_BUF;
	pkt.buf_len = size;
	pkt.buf = buf;
	pkt.paddr = paddr;
	ret = ioctl(fd, CMD_SM_RELEASE_UNCACHED_BUF, &pkt);
	return ret;
}

void mcapi_trans_connect_channel_internal (mcapi_endpoint_t send_endpoint,
		mcapi_endpoint_t receive_endpoint,channel_type type)
{
	uint16_t sd,sn,se;
	uint16_t rd,rn,re;
	int index;
	int ret;
	uint32_t icc_type;

	/* the database should already be locked */

	assert(mcapi_trans_decode_handle_internal(send_endpoint,&sd,&sn,&se));
	assert(mcapi_trans_decode_handle_internal(receive_endpoint,&rd,&rn,&re));

	index = mcapi_trans_get_port_index(sn, se);
	if (index >= MCAPI_MAX_ENDPOINTS) {
		return;
	}

	if (type == MCAPI_PKT_CHAN)
		icc_type = SP_SESSION_PACKET;
	else if(type == MCAPI_SCL_CHAN)
		icc_type = SP_SESSION_SCALAR;
	else
		return;
	ret = sm_connect_session(index, re, rn, icc_type);
	if (ret) {
		printf("%s failed\n", __func__);
		return;
	} else {
		/* update the send endpoint */
		c_db->domains[0].nodes[0].node_d.endpoints[index].connected = MCAPI_TRUE;
		c_db->domains[0].nodes[0].node_d.endpoints[index].recv_queue.recv_endpt = receive_endpoint;
		c_db->domains[0].nodes[0].node_d.endpoints[index].recv_queue.channel_type = type;

		printf("%s %d connected %d\n", __func__, send_endpoint, receive_endpoint);
	}

}
