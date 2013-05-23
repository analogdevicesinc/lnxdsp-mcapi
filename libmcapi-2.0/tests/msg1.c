/* Test: msg1
   Description: Tests simple blocking msgsend and msgrecv calls to several endpoints
   on a single node 
*/

#include <mcapi.h>
#include <mca.h>
#include <mcapi_test.h>
#include <stdio.h>
#include <stdlib.h> /* for malloc */
#include <string.h>
#include <mcapi_impl_spec.h>


#define NUM_SIZES 4
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

void send (mcapi_endpoint_t send, mcapi_endpoint_t recv,char* msg,mcapi_status_t status,int exp_status) {
  int size = strlen(msg);
  int priority = 1;
  mcapi_request_t request;

  mcapi_msg_send_i(send,recv,msg,size,priority,&request,&status);
  if (status != exp_status) { WRONG}
  if (status == MCAPI_SUCCESS) {
    printf("endpoint=%i has sent: [%s]\n",(int)send,msg);
  }
}

void recv (mcapi_endpoint_t recv,mcapi_status_t status,int exp_status) {
  size_t recv_size;
  char buffer[BUFF_SIZE];
  mcapi_request_t request;
  mcapi_msg_recv_i(recv,buffer,BUFF_SIZE,&request,&status);
  if (status != exp_status) { WRONG}
  if (status == MCAPI_SUCCESS) {
    printf("endpoint=%i has received: [%s]\n",(int)recv,buffer);
  }
}

int main () {
  mcapi_status_t status;
  mcapi_param_t parms;
  mcapi_info_t version;
  mcapi_endpoint_t ep1,ep2;
  int i,s = 0, rc = 0;
  mcapi_uint_t avail;

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

  /* send and recv messages on the endpoints */
  /* regular endpoints */

  for (s = 0; s < NUM_SIZES; s++) {

  send (ep1,ep2,"1Hello MCAPI",status,MCAPI_SUCCESS);
  if (status != MCAPI_SUCCESS) { WRONG }
  printf("coreA: The %d time sending, status %d\n",s, status);

  }
  
#if 1
  rc = 0;
  while (1) {
	avail = mcapi_msg_available(ep1, &status);
	if (avail > 0) {
		recv (ep1,status,MCAPI_SUCCESS);
  		if (status != MCAPI_SUCCESS) { WRONG }
                printf("CoreA : message recv. The %d time receiving ok, status %i\n", rc, status);

		if (rc == NUM_SIZES)
                break;
                rc++;
	}
	sleep(2);
  }
#endif

  mcapi_endpoint_delete(ep1,&status);
  mcapi_endpoint_delete(ep2,&status);

  mcapi_finalize(&status);
  printf("CoreA Test PASSED\n");
  return 0;
}
