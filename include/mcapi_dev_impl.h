/*
 ** Copyright (c) 2019, Analog Devices, Inc.  All rights reserved.
*/
#ifndef _MCAPI_DEV_IMPL_H_
#define _MCAPI_DEV_IMPL_H_
#include <stdint.h>
#include <icc.h>

int sm_dev_initialize(void);

void sm_dev_finalize(void);

int sm_create_session(uint32_t src_ep, uint32_t type);
int sm_destroy_session(uint32_t src_ep);
int sm_connect_session(uint32_t dst_ep, uint32_t dst_cpu, uint32_t src_ep);
int sm_disconnect_session(uint32_t dst_ep, uint32_t src_ep);
int sm_send_packet(uint32_t session_idx, uint32_t dst_ep, uint32_t dst_cpu,
		void *buf, uint32_t len, int *payload, int blocking);
int sm_recv_packet(uint32_t session_idx, uint16_t *dst_ep,
		uint16_t *dst_cpu, void *buf, uint32_t *len, int blocking);
int sm_send_scalar(uint32_t session_idx, uint16_t dst_ep, uint16_t dst_cpu, 
		uint32_t scalar0, uint32_t scalar1, uint32_t size, int blocking);
int sm_recv_scalar(uint32_t session_idx, uint16_t *src_ep, uint16_t *src_cpu, uint32_t *scalar0,
		uint32_t *scalar1, uint32_t *size, int blocking);
int sm_get_session_status(uint32_t session_idx, struct sm_session_status *status);
int sm_get_node_status(uint32_t node, uint32_t *session_mask, uint32_t *session_pending, uint32_t *nfree);
int sm_wait_nonblocking(uint32_t session_idx, uint32_t dst_ep, uint32_t dst_cpu,
		void *buf, uint32_t *len, uint32_t type, uint32_t payload, unsigned int timeout, int blocking);
int sm_get_remote_ep(uint32_t dst_ep, uint32_t dst_cpu, int timeout, int blocking);

#endif
