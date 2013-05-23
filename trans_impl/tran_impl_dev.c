//#include <tls.h>
#include <icc.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <mcapi.h>

__thread int fd;
struct sm_packet pkt;

int sm_initialize()
{
	fd = open("/dev/icc", O_RDWR);
	if (fd < 0) {
		perror("unable to open /dev/icc");
	}
	return fd;
}

int sm_finalize()
{
	close(fd);
	return 0;
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
	pkt.local_ep = src_ep;
	ioctl(fd, CMD_SM_SHUTDOWN, &pkt);
}

int sm_connect_session(uint32_t dst_ep, uint32_t dst_cpu, uint32_t src_ep)
{
}

int sm_disconnect_session(uint32_t dst_ep, uint32_t src_ep)
{
}

int sm_send_packet(uint32_t session_idx, uint32_t dst_ep,
		uint32_t dst_cpu, void *buf, uint32_t len)
{
	pkt.session_idx = session_idx;
	pkt.remote_ep = dst_ep;
	pkt.dst_cpu = dst_cpu;
	pkt.buf_len = len;
	pkt.buf = buf;
	ioctl(fd, CMD_SM_SEND, &pkt);
	return 0;
}

int sm_recv_packet(uint32_t session_idx, void **buf,
		uint32_t len)
{
	pkt.session_idx = session_idx;
	pkt.buf_len = len;
	ioctl(fd, CMD_SM_RECV, &pkt);
	*buf = pkt.buf;
	return 0;
}
