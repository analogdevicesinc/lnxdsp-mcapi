/*
Copyright (c) 2008, The Multicore Association
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

(1) Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.
 
 (2) Redistributions in binary form must reproduce the above copyright
 notice, this list of conditions and the following disclaimer in the
 documentation and/or other materials provided with the distribution. 

 (3) Neither the name of the Multicore Association nor the names of its
 contributors may be used to endorse or promote products derived from
 this software without specific prior written permission. 

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
 OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>

#include <errno.h>

#include <mcapi.h>
#include <transport_sm.h>

#include <icc.h>

#define SEMKEYPATH "/dev/null"  /* Path used on ftok for semget key  */
#define SEMKEYID 1              /* Id used on ftok for semget key    */
#define SHMKEYPATH "/dev/null"  /* Path used on ftok for shmget key  */
#define SHMKEYID 2              /* Id used on ftok for shmget key    */
#define MAGIC_NUM 0xdeadcafe

/* the semaphore id */
uint32_t sem_id;
/* the shared memory address */
void* shm_addr;
/* the shared memory database */
mcapi_database* c_db = NULL;

/* the debug level */
int mcapi_debug = 0;



/* semaphore management */
uint32_t transport_sm_create_semaphore(uint32_t semkey) {
	int sem_id;
	union semun {
		int              val;    /* Value for SETVAL */
		struct semid_ds *buf;    /* Buffer for IPC_STAT, IPC_SET */
		unsigned short  *array;  /* Array for GETALL, SETALL */
		struct seminfo  *__buf;  /* Buffer for IPC_INFO
					    (Linux-specific) */
	} arg;

	arg.val = 1;
	if((sem_id = semget(semkey, 1, IPC_CREAT|IPC_EXCL|0666)) == -1)  {
		return -1;
	} else {
		if (semctl(sem_id, 0, SETVAL, arg) == -1)
			return -1;
	}
	return sem_id;
}

mcapi_boolean_t transport_sm_lock_semaphore(uint32_t semid)
{
	struct sembuf sem_lock={ 0, -1, 0}; 
	if((semop(sem_id, &sem_lock, 1)) == -1) {
		return -1;
	}
	return 0;
}

mcapi_boolean_t transport_sm_unlock_semaphore(uint32_t semid)
{
	struct sembuf sem_unlock={ 0, 1, 0};
	/* Attempt to unlock the semaphore set */
	if((semop(sem_id, &sem_unlock, 1)) == -1) {
		return -1;
	}

	return 0;
}

void* sm_attach_shared_mem(uint32_t shmid){ 
	void *shm_addr;
	int rval;
	struct shmid_ds dsbuf;

	shm_addr = shmat(shmid, 0, 0);
	if ((long)shm_addr == (-1)) {
		return NULL;
	}
	rval = shmctl(shmid, IPC_STAT, &dsbuf);
	if (rval != 0) {
		shmdt(shm_addr);
		return NULL;
	}

	/* if we are the first to attach, then initialize the segment to 0 */
	if (dsbuf.shm_nattch == 1) {
		memset(shm_addr,0,dsbuf.shm_segsz);
	}
	return shm_addr;
}

/* shared memory management */
mcapi_boolean_t transport_sm_create_shared_mem(void** addr,uint32_t shmkey,uint32_t size) 
{
	uint32_t shmid = shmget(shmkey, size, 0666 | IPC_CREAT | IPC_EXCL); 
	if (errno == EEXIST) {
		shmid = shmget(shmkey, size, 0666 | IPC_CREAT); 
	}

	if (shmid == -1) {
		return MCAPI_FALSE;
	}  else {
		*addr = sm_attach_shared_mem(shmid);
		return MCAPI_TRUE;
	}
}

mcapi_boolean_t transport_sm_free_shared_mem(uint32_t shmid,void *shm_address)
{
	struct shmid_ds shmid_struct;

	/* detach the shared memory segment */
	int rc = shmdt(shm_address);
	if (rc==-1) {
		mrapi_dprintf(1,"Warning: mrapi: sysvr4_free_shared_mem shmdt() failed\n");
		return MCAPI_FALSE;
	}

	/* delete the shared memory id */
	rc = shmctl(shmid, IPC_RMID, &shmid_struct);
	if (rc==-1)  {
		mcapi_dprintf(1,"Warning: mrapi: sysvr4_free_shared_mem shmctl() failed\n");
		return MCAPI_FALSE;
	}
	return MCAPI_TRUE;

}

mcapi_boolean_t mcapi_trans_add_node (mcapi_uint_t node_num) 
{
	mcapi_boolean_t rc = MCAPI_TRUE;

	/* lock the database */
	mcapi_trans_access_database_pre_nocheck();

	/* mcapi should have checked that the node doesn't already exist */

	if (c_db->num_nodes == MAX_NODES) {
		rc = MCAPI_FALSE;
	}

	if (rc) {
		/* setup our local (private data) */
		/* we do this while we have the lock because we don't want an inconsistency/
		   race condition where the node exists in the database but not yet in
		   the transport layer's cached state */
		mcapi_trans_set_node_num(node_num);

		/* add the node */
		c_db->nodes[c_db->num_nodes].finalized = MCAPI_FALSE;  
		c_db->nodes[c_db->num_nodes].valid = MCAPI_TRUE;  
		c_db->nodes[c_db->num_nodes].node_num = node_num;
		c_db -> num_nodes++;
	}

	/* unlock the database */
	mcapi_trans_access_database_post_nocheck();

	return rc;
}

/***************************************************************************
NAME:mcapi_trans_encode_handle_internal 
DESCRIPTION:
Our handles are very simple - a 32 bit integer is encoded with 
an index (16 bits gives us a range of 0:64K indices).
Currently, we only have 2 indices for each of: node array and
endpoint array.
PARAMETERS: 
node_index -
endpoint_index -
RETURN VALUE: the handle
 ***************************************************************************/
uint32_t mcapi_trans_encode_handle_internal (uint16_t node_index,uint16_t endpoint_index) 
{
	/* The database should already be locked */
	uint32_t handle = 0;
	uint8_t shift = 16;

	assert ((node_index < MAX_NODES) && (endpoint_index < MAX_ENDPOINTS));

	handle = node_index;
	handle <<= shift;
	handle |= endpoint_index;

	return handle;
}

/***************************************************************************
NAME:mcapi_trans_decode_handle_internal
DESCRIPTION: Decodes the given handle into it's database indices
PARAMETERS: 
handle -
node_index -
endpoint_index -
RETURN VALUE: true/false indicating success or failure
 ***************************************************************************/
mcapi_boolean_t mcapi_trans_decode_handle_internal (uint32_t handle, uint16_t *node_index,
		uint16_t *endpoint_index) 
{
	int rc = MCAPI_FALSE;
	uint8_t shift = 16;

	/* The database should already be locked */
	*node_index              = (handle & 0xffff0000) >> shift;
	*endpoint_index          = (handle & 0x0000ffff);

	if ((*node_index < MAX_NODES) && (*endpoint_index < MAX_ENDPOINTS) &&
			(c_db->nodes[*node_index].node_d.endpoints[*endpoint_index].valid)) {
		rc = MCAPI_TRUE;
	}

	return rc;
}



/****************** error checking queries *************************/
/* checks if the given node is valid */
mcapi_boolean_t mcapi_trans_valid_node(mcapi_uint_t node_num)
{
  return MCAPI_FALSE;
}

mcapi_boolean_t mcapi_trans_valid_endpoints(mcapi_uint_t node_num)
{
  return MCAPI_FALSE;
}

/* checks to see if the port_num is a valid port_num for this system */
mcapi_boolean_t mcapi_trans_valid_port(mcapi_uint_t port_num)
{
  return MCAPI_FALSE;
}



/* checks if the endpoint handle refers to a valid endpoint */
mcapi_boolean_t mcapi_trans_valid_endpoint (mcapi_endpoint_t endpoint)
{
  return MCAPI_FALSE;
}



/* checks if the channel is open for a given endpoint */
mcapi_boolean_t mcapi_trans_endpoint_channel_isopen (mcapi_endpoint_t endpoint)
{
  return MCAPI_FALSE;
}



/* checks if the channel is open for a given pktchan receive handle */
mcapi_boolean_t mcapi_trans_pktchan_recv_isopen (mcapi_pktchan_recv_hndl_t receive_handle) 
{
  return MCAPI_FALSE;
}



/* checks if the channel is open for a given pktchan send handle */
mcapi_boolean_t mcapi_trans_pktchan_send_isopen (mcapi_pktchan_send_hndl_t send_handle) 
{
  return MCAPI_FALSE;
}



/* checks if the channel is open for a given sclchan receive handle */
mcapi_boolean_t mcapi_trans_sclchan_recv_isopen (mcapi_sclchan_recv_hndl_t receive_handle) 
{
  return MCAPI_FALSE;
}



/* checks if the channel is open for a given sclchan send handle */
mcapi_boolean_t mcapi_trans_sclchan_send_isopen (mcapi_sclchan_send_hndl_t send_handle) 
{
  return MCAPI_FALSE;
}



/* checks if the given endpoint is owned by the given node */
mcapi_boolean_t mcapi_trans_endpoint_isowner (mcapi_endpoint_t endpoint)
{
  return MCAPI_FALSE;
}



channel_type mcapi_trans_channel_type (mcapi_endpoint_t endpoint)
{
  return 0;
}



mcapi_boolean_t mcapi_trans_channel_connected  (mcapi_endpoint_t endpoint)
{
  return MCAPI_FALSE;
}



mcapi_boolean_t mcapi_trans_recv_endpoint (mcapi_endpoint_t endpoint)
{
  return MCAPI_FALSE;
}



mcapi_boolean_t mcapi_trans_send_endpoint (mcapi_endpoint_t endpoint)
{
  return MCAPI_FALSE;
}



/* checks if the given endpoints have compatible attributes */
mcapi_boolean_t mcapi_trans_compatible_endpoint_attributes  (mcapi_endpoint_t send_endpoint, mcapi_endpoint_t recv_endpoint)
{
  return MCAPI_FALSE;
}



/* checks if the given channel handle is valid */
mcapi_boolean_t mcapi_trans_valid_pktchan_send_handle( mcapi_pktchan_send_hndl_t handle)
{
  return MCAPI_FALSE;
}


mcapi_boolean_t mcapi_trans_valid_pktchan_recv_handle( mcapi_pktchan_recv_hndl_t handle)
{
  return MCAPI_FALSE;
}


mcapi_boolean_t mcapi_trans_valid_sclchan_send_handle( mcapi_sclchan_send_hndl_t handle)
{
  return MCAPI_FALSE;
}


mcapi_boolean_t mcapi_trans_valid_sclchan_recv_handle( mcapi_sclchan_recv_hndl_t handle)
{
  return MCAPI_FALSE;
}

mcapi_boolean_t mcapi_trans_initialized (mcapi_node_t node_id)
{
  return MCAPI_FALSE;
}

mcapi_uint32_t mcapi_trans_num_endpoints()
{
  return 0;
}

mcapi_boolean_t mcapi_trans_valid_priority(mcapi_priority_t priority)
{
  return MCAPI_FALSE;
}

mcapi_boolean_t mcapi_trans_connected(mcapi_endpoint_t endpoint)
{
  return MCAPI_FALSE;
}

mcapi_boolean_t valid_status_param (mcapi_status_t* mcapi_status){
  return MCAPI_TRUE;
}

mcapi_boolean_t valid_version_param (mcapi_version_t* mcapi_version)
{
  return MCAPI_TRUE;
}

mcapi_boolean_t valid_buffer_param (void* buffer)
{
  return MCAPI_TRUE;
}

mcapi_boolean_t valid_request_param (mcapi_request_t* request)
{
  return MCAPI_TRUE;
}

mcapi_boolean_t valid_size_param (size_t* size)
{
  return MCAPI_TRUE;
}

/****************** initialization *************************/
mcapi_boolean_t mcapi_trans_initialize_() 
{
	int semkey = ftok(SEMKEYPATH,SEMKEYID);
	int shmkey = 0;
	mcapi_boolean_t rc = MCAPI_TRUE;

	if (!sem_id) {
		/* create the semaphore (it may already exist) */
		sem_id =  transport_sm_create_semaphore(semkey);
		if (!sem_id) {
			return MCAPI_FALSE;
		}
	}  

	/* lock the database */
	transport_sm_lock_semaphore(sem_id);

	if (c_db == NULL) {
		/* create the shared memory (it may already exist) */
		shmkey = ftok(SEMKEYPATH,SEMKEYID);
		transport_sm_create_shared_mem(&shm_addr,shmkey,sizeof(mcapi_database));  

		if (!shm_addr) {
			return MCAPI_FALSE;
		}

		c_db = shm_addr; 
	}
	transport_sm_unlock_semaphore(sem_id);
	return MCAPI_TRUE;
}

/* initialize the transport layer */
mcapi_boolean_t mcapi_trans_initialize(mcapi_uint_t node_num)
{
	if (mcapi_trans_initialize_()) {
		mcapi_trans_add_node(node_num);
		return MCAPI_TRUE;
	}
	return MCAPI_FALSE;
}



/****************** tear down ******************************/
void mcapi_trans_finalize()
{
	void *shm_addr = c_db;
	uint32_t shmkey = ftok(SEMKEYPATH,SEMKEYID);
	uint32_t shmid = shmget(shmkey, sizeof(mcapi_database), 0666); 
	transport_sm_free_shared_mem(shmid,shm_addr);
}


/****************** endpoints ******************************/
/* create endpoint <node_num,port_num> and return it's handle */
mcapi_boolean_t mcapi_trans_create_endpoint(mcapi_endpoint_t *endpoint,  mcapi_uint_t port_num,mcapi_boolean_t anonymous)
{
	int node_index = mcapi_trans_get_node_index(0);
	int index = sm_create_session(port_num, SP_SESSION_PACKET);
	assert(index >= 0);
	if (index < 0) {
		return index;
	}
	*endpoint = mcapi_trans_encode_handle_internal(node_index,index);
	return MCAPI_FALSE;
}



/* non-blocking get endpoint for the given <node_num,port_num> and set endpoint parameter to it's handle */
void mcapi_trans_get_endpoint_i(  mcapi_endpoint_t* endpoint, mcapi_uint_t node_num, mcapi_uint_t port_num,mcapi_request_t* request,mcapi_status_t* mcapi_status)
{
}




/* blocking get endpoint for the given <node_num,port_num> and return it's handle */
mcapi_boolean_t mcapi_trans_get_endpoint(mcapi_endpoint_t *endpoint,mcapi_uint_t node_num, mcapi_uint_t port_num)
{
  return MCAPI_FALSE;
}



/* delete the given endpoint */
void mcapi_trans_delete_endpoint( mcapi_endpoint_t endpoint)
{
	uint16_t sn,se;
	assert(mcapi_trans_decode_handle_internal(endpoint,&sn,&se));
	sm_destroy_session(se);
}



/* get the attribute for the given endpoint and attribute_num */
void mcapi_trans_get_endpoint_attribute( mcapi_endpoint_t endpoint, mcapi_uint_t attribute_num, void* attribute, size_t attribute_size)
{
}



/* set the given attribute on the given endpoint */
void mcapi_trans_set_endpoint_attribute( mcapi_endpoint_t endpoint, mcapi_uint_t attribute_num, const void* attribute, size_t attribute_size)
{
}




/****************** msgs **********************************/
void mcapi_trans_msg_send_i( mcapi_endpoint_t  send_endpoint, mcapi_endpoint_t  receive_endpoint, char* buffer, size_t buffer_size, mcapi_request_t* request,mcapi_status_t* mcapi_status)
{
	uint16_t sn,se;
	uint16_t rn,re;
	int ret;
	int index;
	mcapi_boolean_t completed =  (*mcapi_status == MCAPI_SUCCESS) ? MCAPI_FALSE : MCAPI_TRUE;

	assert(mcapi_trans_decode_handle_internal(send_endpoint,&sn,&se));
	assert(mcapi_trans_decode_handle_internal(receive_endpoint,&rn,&re));

	index = se;
	ret = sm_send_packet(index, re, rn, buffer, buffer_size);

}



mcapi_boolean_t mcapi_trans_msg_send( mcapi_endpoint_t  send_endpoint, mcapi_endpoint_t  receive_endpoint, char* buffer, size_t buffer_size)
{
  return MCAPI_FALSE;
}



void mcapi_trans_msg_recv_i( mcapi_endpoint_t  receive_endpoint,  char* buffer, size_t buffer_size, mcapi_request_t* request,mcapi_status_t* mcapi_status)
{
	uint16_t sn,se;
	uint16_t rn,re;
	int ret;
	int index;
	mcapi_boolean_t completed =  (*mcapi_status == MCAPI_SUCCESS) ? MCAPI_FALSE : MCAPI_TRUE;

	assert(mcapi_trans_decode_handle_internal(receive_endpoint,&rn,&re));

	index = re;
	ret = sm_recv_packet(index, buffer, buffer_size);

}



mcapi_boolean_t mcapi_trans_msg_recv( mcapi_endpoint_t  receive_endpoint,  char* buffer, size_t buffer_size, size_t* received_size)
{
  return MCAPI_FALSE;
}



mcapi_uint_t mcapi_trans_msg_available( mcapi_endpoint_t receive_endoint)
{
  return 0;
}



/****************** channels general ****************************/

/****************** pkt channels ****************************/
void mcapi_trans_connect_pktchan_i( mcapi_endpoint_t  send_endpoint, mcapi_endpoint_t  receive_endpoint, mcapi_request_t* request,mcapi_status_t* mcapi_status)
{
}



void mcapi_trans_open_pktchan_recv_i( mcapi_pktchan_recv_hndl_t* recv_handle, mcapi_endpoint_t receive_endpoint, mcapi_request_t* request,mcapi_status_t* mcapi_status)
{
}

 

void mcapi_trans_open_pktchan_send_i( mcapi_pktchan_send_hndl_t* send_handle, mcapi_endpoint_t  send_endpoint, mcapi_request_t* request,mcapi_status_t* mcapi_status)
{
}



void  mcapi_trans_pktchan_send_i( mcapi_pktchan_send_hndl_t send_handle, void* buffer, size_t size, mcapi_request_t* request,mcapi_status_t* mcapi_status)
{
}



mcapi_boolean_t  mcapi_trans_pktchan_send( mcapi_pktchan_send_hndl_t send_handle, void* buffer, size_t size)
{
  return MCAPI_FALSE;
}



void mcapi_trans_pktchan_recv_i( mcapi_pktchan_recv_hndl_t receive_handle,  void** buffer, mcapi_request_t* request,mcapi_status_t* mcapi_status)
{
}



mcapi_boolean_t mcapi_trans_pktchan_recv( mcapi_pktchan_recv_hndl_t receive_handle, void** buffer, size_t* received_size)
{
  return MCAPI_FALSE;
}



mcapi_uint_t mcapi_trans_pktchan_available( mcapi_pktchan_recv_hndl_t   receive_handle)
{
  return 0;
}



mcapi_boolean_t mcapi_trans_pktchan_free( void* buffer)
{
  return MCAPI_FALSE;
}



void mcapi_trans_pktchan_recv_close_i( mcapi_pktchan_recv_hndl_t  receive_handle,mcapi_request_t* request,mcapi_status_t* mcapi_status)
{
}



void mcapi_trans_pktchan_send_close_i( mcapi_pktchan_send_hndl_t  send_handle,mcapi_request_t* request,mcapi_status_t* mcapi_status)
{
}



/****************** scalar channels ****************************/
void mcapi_trans_connect_sclchan_i( mcapi_endpoint_t  send_endpoint, mcapi_endpoint_t  receive_endpoint, mcapi_request_t* request,mcapi_status_t* mcapi_status)
{
}



void mcapi_trans_open_sclchan_recv_i( mcapi_sclchan_recv_hndl_t* recv_handle, mcapi_endpoint_t receive_endpoint, mcapi_request_t* request,mcapi_status_t* mcapi_status)
{
}

 

void mcapi_trans_open_sclchan_send_i( mcapi_sclchan_send_hndl_t* send_handle, mcapi_endpoint_t  send_endpoint, mcapi_request_t* request,mcapi_status_t* mcapi_status)
{
}



mcapi_boolean_t mcapi_trans_sclchan_send( mcapi_sclchan_send_hndl_t send_handle,  uint64_t dataword, uint32_t size)
{
  return MCAPI_FALSE;
}



mcapi_boolean_t mcapi_trans_sclchan_recv( mcapi_sclchan_recv_hndl_t receive_handle,uint64_t *data,uint32_t size)
{
  return MCAPI_FALSE;
}



mcapi_uint_t mcapi_trans_sclchan_available_i( mcapi_sclchan_recv_hndl_t receive_handle)
{
  return 0;
}



void mcapi_trans_sclchan_recv_close_i( mcapi_sclchan_recv_hndl_t  recv_handle,mcapi_request_t* mcapi_request,mcapi_status_t* mcapi_status)
{
}



void mcapi_trans_sclchan_send_close_i( mcapi_sclchan_send_hndl_t send_handle,mcapi_request_t* mcapi_request,mcapi_status_t* mcapi_status)
{
}



/****************** test,wait & cancel ****************************/
mcapi_boolean_t mcapi_trans_test_i( mcapi_request_t* request, size_t* size,mcapi_status_t* mcapi_status)
{
  return MCAPI_FALSE;
}



mcapi_boolean_t mcapi_trans_wait( mcapi_request_t* request, size_t* size,mcapi_status_t* mcapi_status)
{
  return MCAPI_FALSE;
}



int mcapi_trans_wait_first( size_t number, mcapi_request_t** requests, size_t* size)
{
  return 0;
}



void mcapi_trans_cancel( mcapi_request_t* request)
{
}


