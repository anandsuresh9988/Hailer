/**
 * @file   hailer.c
 * @author Anand S
 * @date   30 Jan 2022
 * @version 0.1
 * @brief
 *
 */

/*******  Generic headers  ********/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <errno.h>
#include <sys/shm.h>
#include <sys/sem.h>

/*******  HAILER specific headers  ********/
#include "./include/hailer.h"
#include "./include/hailer_app_id.h"
#include "./include/hailer_msgtype.h"


int hailer_app_register(hailer_msg_handle *msg_handle, app_id_t app_id)
{
    struct sockaddr_un hailer_srv_addr;
    int ret = HAILER_ERROR;
    hailer_msg_hdr msg_hdr;
    INITIALISE_HAILER_MSG_HDR(msg_hdr);

    if (msg_handle == NULL || app_id <= APP_ID_HAILER_SERVER)
    {
        HAILER_DBG_ERR("Invalid argumenets !!");
        return ret;
    }

    msg_handle->app_id = app_id;
    msg_handle->comm_fd = socket(AF_LOCAL, SOCK_STREAM, 0);
    if( msg_handle->comm_fd < 0 )
    {
        HAILER_DBG_ERR("Error creating client socket for App, AppId = %d ", app_id);
        return ret;
    }

    memset(&hailer_srv_addr, 0, sizeof(hailer_srv_addr));
    hailer_srv_addr.sun_family = AF_LOCAL;
    strncpy(hailer_srv_addr.sun_path, HAILER_SERVER_ADDRESS, sizeof(hailer_srv_addr.sun_path));

    ret = connect(msg_handle->comm_fd, (struct sockaddr*) &hailer_srv_addr, sizeof(hailer_srv_addr));
    if( ret != 0 )
    {
        HAILER_DBG_ERR("Error connecting to HAILER server, AppId = %d, errno=%d", app_id, errno);
        close(msg_handle->comm_fd);
        return ret;
    }
    else
    {
        HAILER_DBG_INFO("App successfully conncted to HAILER Server. AppId = %d", app_id);
    }

    /* Send APP Launch msg to HAILER server */
    msg_hdr.msg_type = HAILER_MSG_APP_LAUNCH;
    msg_hdr.sndr_app_id = msg_handle->app_id;
    msg_hdr.rcvr_app_id = APP_ID_HAILER_SERVER;

    hailer_send_msg(&msg_hdr, msg_handle->comm_fd);

    return HAILER_SUCCESS;
}

int hailer_app_unregister(hailer_msg_handle *msg_handle)
{
    int ret = HAILER_ERROR;

    if( msg_handle == NULL )
    {
        HAILER_DBG_ERR("Invalid argumenets !!");
        return ret;
    }

    /* Close the socket file descriptor  of the s[pecific app*/
    close(msg_handle->comm_fd);
    return HAILER_SUCCESS;
}

int hailer_send_msg(hailer_msg_hdr *msg_hdr, int fd)
{
    int ret = HAILER_ERROR;
    int len = 0;

    if( fd < 0 || msg_hdr == NULL )
    {
        HAILER_DBG_ERR("Invalid argumenets !!");
        return ret;
    }

    len = sizeof(hailer_msg_hdr) + msg_hdr->msg_len;
    ret = write(fd, msg_hdr, len);
    if(ret < len)
    {
        HAILER_DBG_ERR(" write() failed, ret= %d errno=%d", ret, errno);
    }
    return HAILER_SUCCESS;
}

int hailer_rcv_msg(int fd, hailer_msg_hdr *msg_hdr)
{
    int ret = HAILER_ERROR;
    if( fd < 0 || msg_hdr == NULL )
    {
        HAILER_DBG_ERR("Invalid argumenets !!");
        return ret;
    }

    ret = read(fd, msg_hdr, sizeof(hailer_msg_hdr));
    return ret;
}

hailerShmlist_t *hailer_client_shmlist_init(void)
{
    int shmid                = -1;
    int shm_total_sz         = 0;
    hailerShmlist_t *shmList = NULL;
    int i                    = 0;
    nodeList_t *curr_node    = NULL;

    /* Calcualte the required size of the shared memory.
     * When any new data strucutres are added to the hailer
     * shared memory, include the extra memory required here
     */
    shm_total_sz = sizeof(hailerShmlist_t) + sizeof(nodeList_t);

    /* Get the ID of shared memory which will be already cretaed by hailer server */
    shmid  = shmget((key_t)HAILER_SHMLIST_KEY, shm_total_sz, 0666);
    if(shmid != -1)
    {
        HAILER_DBG_INFO("shmList ID = %d\n", shmid);
    }
    else
    {
        HAILER_DBG_ERR("Failed to get shmid\n");
        return NULL;
    }

    shmList =(hailerShmlist_t *)shmat(shmid, (void*)0, 0);
    if(shmList != NULL)
    {
        HAILER_DBG_INFO("Cshmat() success\n");
    }
    else
    {
         HAILER_DBG_ERR("ShmList shmat() failed!!\n");
    }

    /* Note:
     * Donot modify the values of the hailerShmlist_t pointer or intilise any nodes here
     * This will be alreday done by hailer server. Here just return the starting address
     * of the shared memory for other apps to retrieve the data when required.
    */
    return shmList;
}

/* API to lock semaphore for synchronising shared memory access */
void hailer_shmList_lock(int sem_lock_id)
{
    /* Structure for semaphore operations */
    struct sembuf sem_op;

    /* Wait on the semaphore, unless it's value is non-negative */
    sem_op.sem_num = 0;
    sem_op.sem_op  = -1;
    sem_op.sem_flg = 0;
    semop(sem_lock_id, &sem_op, 1);
}

/* API to ulock semaphore for synchronising shared memory access */
void hailer_shmList_unlock(int sem_lock_id)
{
    /* Structure for semaphore operations */
    struct sembuf sem_op;

    /* signal the semaphore - increase its value by one */
    sem_op.sem_num = 0;
    sem_op.sem_op  = 1;
    sem_op.sem_flg = 0;
    semop(sem_lock_id, &sem_op, 1);
}