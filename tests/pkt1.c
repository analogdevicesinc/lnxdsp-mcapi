/* Test: pkt1
   Description: Tests blocking pkt_channel_send and pkt_channel_recv calls between
   endpoints on a single node.  Tests just about all error conditions.
*/

#include <mcapi.h>
#include <mcapi_datatypes.h>
#include <stdio.h>
#include <stdlib.h> /* for malloc */
#include <string.h>
#include <assert.h>

#define NODE_NUM 1
#define PORT_NUM1 100
#define PORT_NUM2 200
#define PORT_NUM3 300
#define PORT_NUM4 400


#define BUFF_SIZE 64

#define MAX_BUFFERS 8


#define WRONG wrong(__LINE__);
void wrong(unsigned line)
{
  fprintf(stderr,"WRONG: line=%u\n", line);
  fflush(stdout);
  exit(1);
}

void* buffers [MAX_BUFFERS+1];
int num_elements = 0;

void send (mcapi_pktchan_send_hndl_t send_handle, mcapi_endpoint_t recv,char* msg,mcapi_status_t status,int exp_status) {
  int size = strlen(msg);
  

 if (exp_status == MCAPI_EPACK_LIMIT) {
    size = MAX_PKT_SIZE+1;
  }
  mcapi_pktchan_send(send_handle,msg,size,&status);
  if (status != exp_status) { WRONG}
  if (status == MCAPI_SUCCESS) {
    fprintf(stderr,"endpoint=%i has sent: [%s]\n",(int)send,msg);
  }
}

void recv (int free_buffer,mcapi_pktchan_recv_hndl_t recv_handle,mcapi_status_t status,int exp_status) {
  size_t recv_size;
  char *buffer;

  /* test invalid buffer */
  if (exp_status == MCAPI_SUCCESS) {
    mcapi_pktchan_recv(recv_handle,NULL,&recv_size,&status); 
    if (status != MCAPI_EPARAM) { WRONG }
  }
  
  mcapi_pktchan_recv(recv_handle,(void **)((void*)&buffer),&recv_size,&status);
  if (status != exp_status) { WRONG}
  if (status == MCAPI_SUCCESS) {
    fprintf(stderr,"endpoint=%i has received %i bytes: [%s]\n",(int)recv_handle,(int)recv_size,buffer);
    if (free_buffer) {
      mcapi_pktchan_free((void *)buffer,&status);
      if (status != MCAPI_SUCCESS) { WRONG} 
    } else {
      num_elements++;
      buffers[num_elements-1] = buffer;
      assert(num_elements < (MAX_BUFFERS+1));
    }
  }
}

int main () {
  mcapi_status_t status;
  mcapi_request_t request;
  size_t size;
  mcapi_version_t version;
  mcapi_endpoint_t ep1,ep2,ep3,ep4,ep5,ep6,ep7,ep8,ep9,ep10,ep11;
  /* cases:
     1: both named endpoints (1,2)
     2: both anonymous endpoints (3,4)
     3: anonymous sender, named receiver (5,6)
     4: anonymous receiver, named sender (7,8)
  */
  mcapi_pktchan_send_hndl_t s1,s2,s3,s4,s5; /* s1 = ep1->ep2, s2 = ep3->ep4, s3 = ep5->ep6, s4 = ep7->ep8 */
  mcapi_pktchan_recv_hndl_t r1,r2,r3,r4,r5; /* r1 = ep1->ep2, r2 = ep3->ep4, r3 = ep5->ep6, r4 = ep7->ep8 */
  mcapi_uint_t avail;
  int i,j;
  char* dummy_buffer;

  for (i = 0; i < MAX_BUFFERS; i++) {
    buffers[i] = NULL;
  }

  /* create a node */
  mcapi_initialize(NODE_NUM,&version,&status);
  if (status != MCAPI_SUCCESS) { WRONG }
 
  /* create endpoints */
  ep1 = mcapi_create_endpoint (PORT_NUM1,&status);
  if (status != MCAPI_SUCCESS) { WRONG }

  ep2 = mcapi_create_endpoint (PORT_NUM2,&status);
  if (status != MCAPI_SUCCESS) { WRONG }

    mcapi_connect_pktchan_i(ep1,ep2, &request, &status);
  
  /* send and recv messages on the channels */
  /* regular endpoints */
  send (s1,ep2,"Hello MCAPI",status,MCAPI_SUCCESS);

  /* error: invalid buffer */
  mcapi_pktchan_free((void*)&dummy_buffer,&status);
  if (status != MCAPI_ENOT_VALID_BUF) { WRONG }

  /* error: endpoints receive queue is not full, but there are no more buffers available */
  send (s1,ep2,"no more buffers available...",status,MCAPI_ENO_BUFFER);
  /* check if packets are available */
  send (s1,ep2,"pkt is available",status,MCAPI_SUCCESS);
  avail = mcapi_pktchan_available(r1, &status);
  if (status != MCAPI_SUCCESS) { WRONG }    
  if (!avail) { WRONG }
  avail = mcapi_pktchan_available(r2, &status);
  if (status != MCAPI_SUCCESS) { WRONG }    
  if (avail) { WRONG }

  /* close the channels */

  mcapi_finalize(&status);
  printf("   Test PASSED\n");
  return 0;
}
