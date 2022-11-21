#ifndef NBHUS_H
#define NBHUS_H
/**
 * @file   hailer.h
 * @author Anand S
 * @date   30 Jan 2022
 * @version 0.1
 * @brief
 *
 */


/*********  Headers   *******/
#include <stdio.h>
#include <arpa/inet.h>
#include "hailer_app_id.h"
#include "hailer_msgtype.h"


/*********  MACROS   ***************/
#define HAILER_SERVER_ADDRESS "/var/.hailer_server_address.sock"


/************* HAILER ERROR CODES *******/
#define HAILER_SUCCESS 0
#define HAILER_ERROR   -1


/********* LOGGING  ************/
#define HAILER_DBG_ENABLE 1

#if (HAILER_DBG_ENABLE == 1)

    #define HAILER_DBG_ERR(fmt, args...) \
        do { \
                printf("HAILER_ERR: %s %d %s : " fmt "\n", __FILE__, __LINE__, __FUNCTION__, ##args); \
        } while(0)

    #define HAILER_DBG_INFO(fmt, args...) \
        do { \
                printf("HAILER_INFO: %s %d %s : " fmt "\n", __FILE__, __LINE__, __FUNCTION__, ##args); \
        } while(0)

#else

    #define HAILER_DBG_ERR(fmt, args...) while(0);
    #define HAILER_DBG_INFO(fmt, args...) while(0);

#endif //HAILER_DBG_ENABLE

/**** Macros for coloured fonts ****/
#define PRNT_GRN printf("\033[0;32m");
#define PRNT_RST printf("\033[0m");
#define PRNT_RED printf("\033[1;31m");

#define HAILER_MAX_PEERS           255   // Max no of peers hailer peer discovery mechanism can support.
                                         // Use this value to intialise the shared memroy list to store peer details
#define HAILER_SHMLIST_KEY         0x647651cefd98     // Key for the Hailer shared memory
#define HAILER_SHM_LCK_KEY         0x647651cefd99     // Key for the Hailer semaphore for synchronising the shared memory access
#define MAX_HOSTNAME_SIZE          256

#ifndef FALSE
#define FALSE (0)
#endif //FALSE

#ifndef TRUE
#define TRUE (1)
#endif  //TRUE


/*********   Common structs   ******/
typedef struct _hailer_msg_handle
{
    int comm_fd;
    int app_id;
} hailer_msg_handle;

typedef struct _hailer_msg_hdr
{
    char sndr_ip[15];
    char rcvr_ip[15];
    int sndr_app_id;
    int rcvr_app_id;
    msg_type_t msg_type;
    void *msg;
    int msg_len;
} hailer_msg_hdr;

typedef struct _clientDesc
{
    int isUsed;
    struct in_addr ipAddr;
    char hostname[MAX_HOSTNAME_SIZE];
    char mac[18];
    time_t lastSeenTimestamp;
    long double uptime;
} clientDesc_t;

typedef struct _nodeList
{
    clientDesc_t client;
    struct _nodeList *next;
} nodeList_t;

typedef struct _hailer_shmlist
{
    nodeList_t shmlistHead[HAILER_MAX_PEERS];
    int activePeerCount;
    int shmid;
    int sem_lock_id;
    struct _hailer_shmlist *shmaddr;
} hailerShmlist_t;

#define INITIALISE_HAILER_MSG_HDR(msg_hdr) \
        do { \
            strncpy(&msg_hdr.sndr_ip[0], "127.0.0.1", strlen("127.0.0.1") + 1); \
            strncpy(&msg_hdr.rcvr_ip[0], "127.0.0.1", strlen("127.0.0.1") + 1); \
            msg_hdr.sndr_app_id = -1; \
            msg_hdr.rcvr_app_id = APP_ID_HAILER_SERVER; \
            msg_hdr.msg_type = -1; \
            msg_hdr.msg = NULL; \
            msg_hdr.msg_len = 0; \
        } while(0)


/**********  API's  ****************/

/* Call this API during initilisation of all app using hailer*/
int hailer_app_register(hailer_msg_handle *msg_handle, app_id_t app_id);

/* API to send Msgs to other apps in same device and apps in other devices in network*/
int hailer_send_msg(hailer_msg_hdr *msg_hdr, int fd);

/* API to Rcv msgs without timeout */
int hailer_rcv_msg(int fd, hailer_msg_hdr *msg_hdr);

/* API to Rcv msgs without timeout */
/* TODO: Need to implement */
//int hailer_rcv_msg_with_timeout(hailer_msg_handle *msg_handle, hailer_msg_hdr *msg_hdr, int timeout);

/* Call this API during exit of all app using hailer*/
int hailer_app_unregister(hailer_msg_handle *msg_handle);

/* API to init the shmList shared memory for hailer client apps */
hailerShmlist_t *hailer_client_shmlist_init(void);

/* API to lock semaphore for synchronising shared memory access */
void hailer_shmList_lock(int sem_lock_id);

/* API to ulock semaphore for synchronising shared memory access */
void hailer_shmList_unlock(int sem_lock_id);

#endif //NBHUS_H
