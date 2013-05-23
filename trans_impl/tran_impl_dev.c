//#include <tls.h>
#include <icc.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <mcapi.h>

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
	pkt.local_ep = src_ep;
	pkt.type = type;
	ret = ioctl(fd, CMD_SM_CREATE, &pkt);
	return ret;
}

int sm_destroy_session(uint32_t src_ep)
{
	int ret;
	pkt.local_ep = src_ep;
	ret = ioctl(fd, CMD_SM_SHUTDOWN, &pkt);
	return ret;
}

int sm_connect_session(uint32_t dst_ep, uint32_t dst_cpu, uint32_t src_ep)
{
	int ret;
	ret = ioctl(fd, CMD_SM_CONNECT, &pkt);
	return ret;
}

int sm_disconnect_session(uint32_t dst_ep, uint32_t src_ep)
{
	int ret;
	ret = ioctl(fd, CMD_SM_CONNECT, &pkt);
	return ret;
}

int sm_send_packet(uint32_t session_idx, uint32_t dst_ep,
		uint32_t dst_cpu, void *buf, uint32_t len)
{
	int ret;
	pkt.session_idx = session_idx;
	pkt.remote_ep = dst_ep;
	pkt.dst_cpu = dst_cpu;
	pkt.buf_len = len;
	pkt.buf = buf;
	ret = ioctl(fd, CMD_SM_SEND, &pkt);
	return 0;
}

int sm_recv_packet(uint32_t session_idx, uint16_t *dst_ep, uint16_t *dst_cpu, void **buf,
		uint32_t *len)
{
	int ret;
	pkt.session_idx = session_idx;
	pkt.buf_len = len;
	ret = ioctl(fd, CMD_SM_RECV, &pkt);
	if (dst_ep)
		*dst_ep = pkt.remote_ep;
	if (dst_cpu)
		*dst_cpu = pkt.dst_cpu;
	*buf = pkt.buf;
	if (len)
		*len = pkt.buf_len;
	return 0;
}
