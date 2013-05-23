/* Test: msg1
   Description: Tests simple blocking msgsend and msgrecv calls to several endpoints
   on a single node 
*/

#include <mcapi.h>
#include <mcapi_datatypes.h>
#include <stdio.h>
#include <stdlib.h> /* for malloc */
#include <string.h>

#define NODE_NUM 1
#define PORT_NUM1 100
#define PORT_NUM2 200

#define BUFF_SIZE 64

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

  if (exp_status == MCAPI_EMESS_LIMIT) {
    size = MAX_MSG_SIZE+1;
  }

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
  mcapi_version_t version;
  mcapi_endpoint_t ep1,ep2;
  int i;

  /* create a node */
  mcapi_initialize(NODE_NUM,&version,&status);
  if (status != MCAPI_SUCCESS) { WRONG }
    
  /* create endpoints */
  ep1 = mcapi_create_endpoint (PORT_NUM1,&status);
  if (status != MCAPI_SUCCESS) { WRONG }
  printf("ep1 %x   \n", ep1);

 ep2 = mcapi_trans_encode_handle_internal(1,5);

  /* send and recv messages on the endpoints */
  /* regular endpoints */
  send (ep1,ep2,"1Hello MCAPI",status,MCAPI_SUCCESS);
  recv (ep2,status,MCAPI_SUCCESS);
 
  mcapi_finalize(&status);
  printf("   Test PASSED\n");
  return 0;
}
