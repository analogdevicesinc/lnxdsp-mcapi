/* Test: pkt4
   Description: Tests simple out-of-order non-blocking pkt_channel_send and 
   pkt_channel_recv calls to two endpoints on asingle node.  This is a 
   directed test for a bug found using a pthreaded test.  Specifically this 
   test makes sure that pktchan calls in the order: recv send send recv obey FIFO.
*/

#include <mcapi.h>
#include <mca.h>
#include <mcapi_test.h>
#include <stdio.h>
#include <stdlib.h> /* for malloc */
#include <string.h>
#include <mcapi_impl_spec.h>


#define BUFF_SIZE 64
#define DOMAIN 0
#define NODE 0

#define WRONG wrong(__LINE__);
void wrong(unsigned line)
{
  fprintf(stderr,"WRONG: line=%u\n", line);
  fflush(stdout);
  exit(1);
}

char send_buf[32] = "mcapi_pkt1";
const int fail = 1;
const int ok = 0;

int main () {
  size_t size;
  mcapi_endpoint_t ep1,ep2,ep3,ep4;
  mcapi_pktchan_send_hndl_t s1; /* s1 = ep1->ep3 */
  mcapi_pktchan_recv_hndl_t r1; /* r1 = ep4->ep2 */
  int i = 0;
  int rc = 1;
  mcapi_status_t status;
  mcapi_request_t request;
  mcapi_param_t parms;
  mcapi_info_t version;
  void *pbuffer = NULL;
  mcapi_uint_t avail;

  /* create a node */
  mcapi_initialize(DOMAIN,NODE,NULL,&parms,&version,&status);
  if (status != MCAPI_SUCCESS) { WRONG }

  /* create endpoints */
  ep1 = mcapi_endpoint_create(MASTER_PORT_NUM1,&status);
  if (status != MCAPI_SUCCESS) { WRONG }
  printf("ep1 %x   \n", ep1);
  ep2 = mcapi_endpoint_create(MASTER_PORT_NUM2,&status);
  if (status != MCAPI_SUCCESS) { WRONG }
  printf("ep2 %x   \n", ep1);

  ep3 = mcapi_endpoint_get(DOMAIN,SLAVE_NODE_NUM, SLAVE_PORT_NUM1,MCA_INFINITE, &status);
  if (status != MCAPI_SUCCESS) { WRONG }
  ep4 = mcapi_endpoint_get(DOMAIN,SLAVE_NODE_NUM, SLAVE_PORT_NUM2,MCA_INFINITE, &status);
  if (status != MCAPI_SUCCESS) { WRONG }



  /*************************** connect the channels *********************/
  mcapi_pktchan_connect_i(ep1,ep3,&request,&status);

  mcapi_pktchan_connect_i(ep2,ep4,&request,&status);

  /*************************** open the channels *********************/
  printf("open pktchan send\n");
  mcapi_pktchan_send_open_i(&s1 /*send_handle*/,ep1, &request, &status);
  printf("status %d\n", status);

  printf("pktchan send i\n");
  mcapi_pktchan_send_i(s1,send_buf,32,&request,&status);
  printf("status %d\n", status);

  printf("close pktchan send\n");
  /* close the channels */
  mcapi_pktchan_send_close_i(s1,&request,&status); 
  printf("status %d\n", status);


  mcapi_pktchan_recv_open_i(&r1 /*recv_handle*/,ep2, &request, &status);
  printf("status %d\n", status);

  while (1) {
	  avail = mcapi_pktchan_available(r1, &status);
	  if (avail > 0) {
		  mcapi_pktchan_recv_i(r1,(void **)&pbuffer,&request,&status);
		  printf("RRRRRRRRRRRRr pktchan recv on coreA ok buffer %s, status %d\n",pbuffer,status);
		  mcapi_pktchan_release(pbuffer, &status);
		  break;
	  }
	  sleep(2);
  }


  mcapi_finalize(&status);
  if (rc == 0) {
    printf("   Test PASSED\n");
  } else {
    printf("   Test FAILED\n");
  }
  return rc;
}
