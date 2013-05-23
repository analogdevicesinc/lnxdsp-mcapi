//#include <tls.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <mcapi.h>
#include <icc.h>

struct sm_packet pkt;
int fd;

int sm_dev_initialize()
{
	fd = open("/dev/icc", O_RDWR);
	if (fd < 0) {
		perror("unable to open /dev/icc");
	}
	return fd;
}

void sm_dev_finalize(int fd)
{
	close(fd);
}

int sm_create_session(uint32_t src_ep, uint32_t type)
{
	int ret;
	memset(&pkt, 0, sizeof(pkt));
	pkt.local_ep = src_ep;
	pkt.type = type;
	ret = ioctl(fd, CMD_SM_CREATE, &pkt);
	return pkt.session_idx;
}

int sm_destroy_session(uint32_t session_idx)
{
	int ret;
	memset(&pkt, 0, sizeof(pkt));
	pkt.session_idx = session_idx;
	ret = ioctl(fd, CMD_SM_SHUTDOWN, &pkt);
	return ret;
}

int sm_connect_session(uint32_t session_idx, uint32_t dst_ep, uint32_t dst_cpu)
{
	int ret;
	memset(&pkt, 0, sizeof(pkt));
	pkt.session_idx = session_idx;
	pkt.remote_ep = dst_ep;
	pkt.dst_cpu = dst_cpu;
	ret = ioctl(fd, CMD_SM_CONNECT, &pkt);
	return ret;
}

int sm_disconnect_session(uint32_t session_idx, uint32_t dst_ep, uint32_t dst_cpu)
{
	int ret;
	memset(&pkt, 0, sizeof(pkt));
	pkt.session_idx = session_idx;
	pkt.remote_ep = dst_ep;
	pkt.dst_cpu = dst_cpu;
	ret = ioctl(fd, CMD_SM_CONNECT, &pkt);
	return ret;
}

int sm_open_session(uint32_t session_idx)
{
	int ret;
	memset(&pkt, 0, sizeof(pkt));
	pkt.session_idx = session_idx;
	ret = ioctl(fd, CMD_SM_OPEN, &pkt);
	return ret;
}

int sm_close_session(uint32_t session_idx)
{
	int ret;
	memset(&pkt, 0, sizeof(pkt));
	pkt.session_idx = session_idx;
	ret = ioctl(fd, CMD_SM_CLOSE, &pkt);
	return ret;
}

int sm_send_packet(uint32_t session_idx, uint32_t dst_ep,
		uint32_t dst_cpu, void *buf, uint32_t len)
{
	int ret;
	memset(&pkt, 0, sizeof(pkt));
	pkt.session_idx = session_idx;
	pkt.remote_ep = dst_ep;
	pkt.dst_cpu = dst_cpu;
	pkt.buf_len = len;
	pkt.buf = buf;
	ret = ioctl(fd, CMD_SM_SEND, &pkt);
	return ret;
}

int sm_recv_packet(uint32_t session_idx, uint16_t *dst_ep, uint16_t *dst_cpu, void *buf,
		uint32_t *len)
{
	int ret;
	memset(&pkt, 0, sizeof(pkt));
	printf("session_idx %d\n", session_idx);
	pkt.session_idx = session_idx;
	if (buf)
		pkt.buf = buf;
	else
		return -EINVAL;
	pkt.buf_len = len;
	ret = ioctl(fd, CMD_SM_RECV, &pkt);
	if (ret)
		return ret;
	if (dst_ep)
		*dst_ep = pkt.remote_ep;
	if (dst_cpu)
		*dst_cpu = pkt.dst_cpu;
	if (len)
		*len = pkt.buf_len;
	return 0;
}

int sm_get_session_status(uint32_t session_idx, uint32_t *avail, uint32_t *uncomplete, uint32_t *status)
{
	int ret;
	struct sm_session_status param;
	memset(&pkt, 0, sizeof(struct sm_packet));
	memset(&param, 0, sizeof(param));
	pkt.session_idx = session_idx;
	pkt.param = &param;
	pkt.param_len = sizeof(param);
	ret = ioctl(fd, CMD_SM_GET_SESSION_STATUS, &pkt);

	if (avail)
		*avail = param.avail;
	if (uncomplete)
		*uncomplete = param.uncomplete;
	if (status)
		*status = param.status;
	return ret;
}


int sm_get_node_status(uint32_t node, uint32_t *session_mask, uint32_t *session_pending, uint32_t *nfree)
{
	int ret;
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
