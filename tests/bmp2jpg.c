/*
 ** Copyright (c) 2019, Analog Devices, Inc.  All rights reserved.
*/
/* Test: bmp2jpg 
   Description: bmp to jpg encoding demo coreA part
*/

#include <mcapi.h>
#include <mca.h>
#include <mcapi_test.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mcapi_impl_spec.h>
#include <fcntl.h>

#include <sys/stat.h>
#include <unistd.h>

#define NUM_SIZES 4
#define BUFF_SIZE 64
#define DOMAIN 0
#define NODE 0

#define WRONG wrong(__LINE__);

extern void *sm_request_uncached_buf(uint32_t size, uint32_t *paddr);

struct stat bmp_statbuf;

static struct image_info {
  unsigned long bmp_buffer;
  unsigned long jpg_buffer;
  unsigned long bmp_len;
  unsigned long jpg_len;
  unsigned int cmd;
  unsigned int status;
}img_info;

static int bmp_fd;
static int jpg_fd;
static void *bmp_buffer, *jpg_buffer;
static unsigned long bmp_len, jpg_len;
static char *bmp_path = "test.bmp";
static char *jpg_path = "test.jpg";
	
static void jpg_output(void)
{
  if ((jpg_fd = open(jpg_path, O_RDWR | O_CREAT, S_IRWXU)) < 0)
		printf("opening data file '%s' failed", jpg_path);

  printf("img->jpg_buffer = 0x%x\n", img_info.jpg_buffer);
  printf("img->jpg_len = 0x%x\n", img_info.jpg_len);

  write(jpg_fd, img_info.jpg_buffer, img_info.jpg_len);
}

unsigned long paddr;
extern int sm_dev_initialize(void);

static int image_info_alloc(void)
{
  img_info.jpg_len = img_info.bmp_len/2;

  sm_dev_initialize();

  if ((img_info.bmp_buffer = sm_request_uncached_buf(img_info.bmp_len, &paddr)) == NULL){
                printf("malloc() failed");
		return -1;
  }
  printf("#########img_info.bmp_buffer = 0x%x\n", img_info.bmp_buffer);

  if ((img_info.jpg_buffer = sm_request_uncached_buf(img_info.jpg_len, &paddr)) == NULL){
                printf("malloc() failed");
		return -1;
  }
  printf("#########img_info.jpg_buffer = 0x%x\n", img_info.jpg_buffer);

  return 0;
}

void wrong(unsigned line)
{
  fprintf(stderr,"WRONG: line=%u\n", line);
  fflush(stdout);
  _exit(1);
}
char send_string[] = "HELLO_MCAPI";

void send (mcapi_endpoint_t send, mcapi_endpoint_t recv, void* msg,int size, mcapi_status_t status,int exp_status) {
  int priority = 1;
  mcapi_request_t request;

  mcapi_msg_send_i(send,recv,msg,size,priority,&request,&status);
  if (status != exp_status) { WRONG}
  if (status == MCAPI_SUCCESS) {
    printf("endpoint=%i has sent: [%x]\n",(int)send,msg);
  }
}

void recv (mcapi_endpoint_t recv,mcapi_status_t *status,int exp_status) {
  size_t recv_size;
  mcapi_request_t request;
  mcapi_msg_recv_i(recv,(void *)(&img_info),sizeof(struct image_info),&request,status);
  if (*status != exp_status) { WRONG}
  if (*status == MCAPI_SUCCESS) {
    printf("endpoint=%i has received: [%x]\n",(int)recv,img_info.jpg_len);
  }
}

int main () {
  mcapi_status_t status;
  mcapi_param_t parms;
  mcapi_info_t version;
  mcapi_endpoint_t ep1,ep2;
  int i,s = 0, rc = 0,pass_num=0;
  mcapi_uint_t avail;
  unsigned int *buffer;

  system("ffmpeg -f video4linux2 -r 5 -s 320x240 -i /dev/video0 test.bmp");

  if ((bmp_fd = open(bmp_path, O_RDONLY, 0)) < 0)
		printf("opening data file '%s' failed", bmp_path);

  if(stat(bmp_path, &bmp_statbuf) == -1)
	return -1;
  img_info.bmp_len = bmp_statbuf.st_size;

  img_info.status = 0;

  printf("##########img_info.bmp_len = 0x%x\n", img_info.bmp_len);

  if(image_info_alloc())
    return -1;

  if (read(bmp_fd, img_info.bmp_buffer, img_info.bmp_len) != img_info.bmp_len){
		printf("reading file from '%s' failed", bmp_path);
		return -1;
  }

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

  printf("img_info = 0x%x\n", img_info);
  printf("&img_info = 0x%x\n", &img_info);

  static unsigned long send_buf[4];

  send (ep1,ep2,&img_info,sizeof(struct image_info),status,MCAPI_SUCCESS);

  while (1) {
	avail = mcapi_msg_available(ep1, &status);
	if (avail > 0) {
		recv (ep1,&status,MCAPI_SUCCESS);
  		if (status != MCAPI_SUCCESS) { WRONG }
		else{
			jpg_output();
			break;
		    }
	}
	sleep(2);
  }

  mcapi_endpoint_delete(ep1,&status);
  mcapi_finalize(&status);

  return 0;
}
