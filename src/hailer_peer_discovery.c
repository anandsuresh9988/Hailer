/**
 * @file    hailer_node_discovery.c
 * @author  Anand S
 * @date    23 Oct 2022
 * @version 0.1
 * @brief   Module to see MAC addresses of all linux machines in the LAN which has installed this module through a keep alive message
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/shm.h>	 /* shared memory functions and structs */
#include <sys/sem.h>	 /* semaphore functions and structs */

#include "./include/hailer_peer_discovery.h"
#include "./include/hailer.h"


/*** Global variables ***/
extern unsigned int g_keep_running;
extern hailerShmlist_t *shmList;

/* Print all the devices and it's info present in the device shmList */
void hailer_print_peer_list()
{
    int             i = 0;
    int visited_nodes = 0;
    nodeList_t *curr_node = shmList->shmlistHead;

    hailer_shmList_lock(shmList->sem_lock_id);

    for(i = 0; (i < HAILER_MAX_PEERS && visited_nodes < shmList->activePeerCount); i++, curr_node++)
    {
        if(curr_node->client.isUsed == TRUE)
        {
            HAILER_DBG_INFO("\n");
            HAILER_DBG_INFO("IP : %s\n", inet_ntoa(curr_node->client.ipAddr));
            HAILER_DBG_INFO("Last seen time : %ld\n", curr_node->client.lastSeenTimestamp);
            HAILER_DBG_INFO("Last seen time : %Le\n", curr_node->client.uptime);
            HAILER_DBG_INFO("MAC : %s\n", curr_node->client.mac);
            HAILER_DBG_INFO("\n");
            visited_nodes++;
        }
    }
    hailer_shmList_unlock(shmList->sem_lock_id);
}

#if ENCRYPT_MSGS
/* Simple routine to encrypt the msg before sending */
void hailer_encryptMsg(char *Msg, char *EncryptedMsg)
{
    int i = 0;
    char const key[MAX_BUFFRE_SIZE] = HAILER_ENCRYPTION_KEY;

    for (i = 0; i < strlen(Msg); i++)
    {
        if(Msg[i] != '\n')
        {
            EncryptedMsg[i] = (char)(Msg[i]^ key[i]);
        }
        else
        {
            EncryptedMsg[i] = Msg[i];
        }
    }

#if ENCRYPTION_DBG
    HAILER_DBG_INFO("\n====Encrypting Msg====\n\n");
    HAILER_DBG_INFO("    Msg:\n");

    for (i = 0; i <  strlen(Msg); i++)
    {
        HAILER_DBG_INFO("%02X ", Msg[i]);
    }

    HAILER_DBG_INFO("\n    Encrypted Msg:\n");

    for (i = 0; i <  strlen(EncryptedMsg); i++)
    {
        HAILER_DBG_INFO("%02X ", EncryptedMsg[i]);
    }

    HAILER_DBG_INFO("\n\n====Encrypting Msg Done !====\n");
#endif //ENCRYPTION_DBG

}

/*Simple routine to decrypt the Msg*/
void hailer_decrypt_msg(char * encryptedMsg, char* decryptedMsg)
{
    int i = 0;
    char const key[MAX_BUFFRE_SIZE] = HAILER_ENCRYPTION_KEY;

    for (i = 0; i < strlen(encryptedMsg); i++)
    {
        decryptedMsg[i] = (char)(encryptedMsg[i]^ key[i]);
    }

#if ENCRYPTION_DBG
    HAILER_DBG_INFO("\n====Decrypting Msg====\n");
    HAILER_DBG_INFO("    Encrypted Msg:\n");

    for (i = 0; i <  strlen(encryptedMsg); i++)
    {
        HAILER_DBG_INFO("%02X ", encryptedMsg[i]);
    }

    HAILER_DBG_INFO("\n    Decrypted Msg:\n");

    for (i = 0; i < strlen(decryptedMsg); i++)
    {
        HAILER_DBG_INFO("%02X ", decryptedMsg[i]);
    }

    HAILER_DBG_INFO("\n\n====Decrypting Msg Done====\n\n");
#endif //ENCRYPTION_DBG
}
#endif //ENCRYPT_MSGS

/*Get the device hostname using the 'hostname' command*/
void hailer_get_device_hostname(char *hstname)
{
    char cmd[256] = {0};
    FILE *fp = NULL;

    sprintf(cmd , "hostname");

    fp = popen(cmd, "r");
    if(fp == NULL)
    {
        HAILER_DBG_ERR("Error executing the hostname command \n");
    }
    else
    {
        fscanf(fp, "%s", hstname);
        pclose(fp);
    }
}

/* Get the device uptime form /proc/uptime */
void hailer_get_device_uptime(long double * uptime)
{
    char cmd[256] = {0};
    FILE *fp = NULL;

    sprintf(cmd, "%s", "cat /proc/uptime");

    fp = popen(cmd, "r");
    if(fp == NULL)
    {
        HAILER_DBG_ERR("Error obtaining the system uptime\n");
    }
    else
    {
        fscanf(fp, "%Le", uptime);
        pclose(fp);
    }
}

/* Fill the jsonMsg object with required fileds*/
void hailer_fill_jsonMsg(struct json_object *jsonMsg)
{
    char hostname [MAX_HOSTNAME_SIZE] = {0};
    long double uptime;
    char *broadcast_msg = "Peer Boardcast messagae : Keep ALive";

    hailer_get_device_hostname(hostname);
    hailer_get_device_uptime(&uptime);
    json_object_object_add(jsonMsg, MSG, json_object_new_string(broadcast_msg));
    json_object_object_add(jsonMsg, HOSTNAME, json_object_new_string(hostname));
    json_object_object_add(jsonMsg, UPTIME, json_object_new_double(uptime));

#if JSON_DBG
    HAILER_DBG_INFO("jsonMsg : %s\n", json_object_to_json_string_ext(jsonMsg, JSON_C_TO_STRING_SPACED | JSON_C_TO_STRING_PRETTY));
#endif

}

/*Get the MAC address from the IP address using arp command*/
void hailer_find_MAC_from_IP(char *ip, char *mac )
{
    char cmd[256] = {0};
    FILE *fp = NULL;

    sprintf(cmd, "arp -a | grep \"%s\" | awk '{print $4}'", ip);

    fp = popen(cmd,"r");
    if(fp == NULL)
    {
        HAILER_DBG_ERR("Error executing arp command to find the MAC address of %s", ip);
    }
    else
    {
       fscanf(fp, "%s", mac);
       HAILER_DBG_INFO("MAC of %s : %s\n", ip, mac);
       pclose(fp);
    }
}

/* Check whether the device with the client->ipAddr IP already exists in the DeviceList */
unsigned int hailer_is_client_exist(clientDesc_t *client)
{
    unsigned int found = FALSE;
    int              i = 0;
    int visited_nodes  = 0;
    nodeList_t *curr_node = shmList->shmlistHead;

    hailer_shmList_lock(shmList->sem_lock_id);

    for(i = 0; ((i < HAILER_MAX_PEERS) && (visited_nodes < shmList->activePeerCount)); i++, curr_node++)
    {
        //printf("%s %s\n",inet_ntoa(client->ipAddr) , inet_ntoa(current->client->ipAddr));
        /*TODO : Not sure why the below commented code didn't worked !!. Need to check*/
        //if(strcmp(inet_ntoa(client->ipAddr), inet_ntoa(current->client->ipAddr)) == 0 )
        if(curr_node->client.isUsed == TRUE)
        {
            if(client->ipAddr.s_addr == curr_node->client.ipAddr.s_addr)
            {
                found = TRUE;
                break;
            }
            visited_nodes ++;
        }
    }
    hailer_shmList_unlock(shmList->sem_lock_id);

    return found;
}

/* Add the client to the DeviceList */
void hailer_add_device_to_list(clientDesc_t *client)
{
    int              i = 0;
    int visited_nodes  = 0;
    nodeList_t *curr_node = shmList->shmlistHead;

    hailer_shmList_lock(shmList->sem_lock_id);

    for(i = 0; i < HAILER_MAX_PEERS; i++, curr_node++)
    {
        if(curr_node->client.isUsed == FALSE)
        {
            strcpy(curr_node->client.hostname, client->hostname);
            hailer_find_MAC_from_IP(inet_ntoa(client->ipAddr), curr_node->client.mac);
            curr_node->client.ipAddr.s_addr = client->ipAddr.s_addr;
            curr_node->client.lastSeenTimestamp = client->lastSeenTimestamp;
            curr_node->client.uptime = client->uptime;
            curr_node->client.isUsed = TRUE;

            shmList->activePeerCount++;
            break;
        }
    }

    hailer_shmList_unlock(shmList->sem_lock_id);
}

/* Check whether any device which is currently not sending the Keep Alive messgae is present in the
   DeviceList. If present delete the device */
void hailer_delete_expired_devices()
{
    nodeList_t *curr_node = shmList->shmlistHead;
    int i                 = 0;
    int visited_nodes     = 0;
    time_t now            = time(NULL);

    hailer_shmList_lock(shmList->sem_lock_id);
    for(i = 0; ((i < HAILER_MAX_PEERS) && (visited_nodes < shmList->activePeerCount)); i++, curr_node++)
    {
        if(curr_node->client.isUsed == TRUE)
        {
            if(now - curr_node->client.lastSeenTimestamp > TIME_TO_WAIT_TO_DEL_DEVICE)
            {
                memset(curr_node, 0, sizeof(nodeList_t));
                curr_node->client.isUsed = FALSE;
                shmList->activePeerCount--;
                /* Reduce the visited_count as well to make sure that for loop's
                 * termination condition will be satisfied as expected without
                 * the loop running for HAILER_MAX_PEERS times.
                 */
                visited_nodes--;
            }
            visited_nodes++;
        }
    }
    hailer_shmList_unlock(shmList->sem_lock_id);
}

/* Update the file CLIENTS_LIST_FILE with the latest info available in the list pointed by shmList->shmlistHead; */
void hailer_update_client_file()
{
    FILE *fp = fopen(CLIENTS_LIST_FILE, "w");
    nodeList_t *curr_node = shmList->shmlistHead;
    int i = 0;
    int visited_nodes = 0;

    hailer_shmList_lock(shmList->sem_lock_id);

    fprintf(fp, "Total Peers : %d\n\n\n", shmList->activePeerCount);
    for(i = 0; ((i < HAILER_MAX_PEERS) && (visited_nodes < shmList->activePeerCount)); i++, curr_node++)
    {
        if(curr_node->client.isUsed == TRUE)
        {
            visited_nodes++;
            fprintf(fp, "Client NO: %d\n", visited_nodes);
            fprintf(fp, "IP Address: %s\n", inet_ntoa(curr_node->client.ipAddr));
            fprintf(fp, "Last seen time stamp: %ld\n", curr_node->client.lastSeenTimestamp);
            fprintf(fp, "Uptime: %Le\n", curr_node->client.uptime);
            fprintf(fp, "MAC: %s\n", curr_node->client.mac);
            fprintf(fp, "\n");
        }
    }
    hailer_shmList_unlock(shmList->sem_lock_id);

    fclose(fp);
}

/* If the device is already present in the list just update it's timestamp */
void hailer_update_timestamp(clientDesc_t *client)
{
    int i             = 0;
    int visited_nodes = 0;
    nodeList_t *curr_node = shmList->shmlistHead;

    hailer_shmList_lock(shmList->sem_lock_id);

    for(i = 0; ((i < HAILER_MAX_PEERS) && (visited_nodes < shmList->activePeerCount)); i++, curr_node++)
    {
        if(curr_node->client.isUsed == TRUE)
        {
            /*TODO : Not sure why the below commented code didn't worked !!. Need to check*/
            //if(strncmp(inet_ntoa(client->ipAddr), inet_ntoa(current->client->ipAddr), sizeof(struct in_addr)) ==0 )
            if(client->ipAddr.s_addr == curr_node->client.ipAddr.s_addr)
            {
                curr_node->client.lastSeenTimestamp = client->lastSeenTimestamp;
                curr_node->client.uptime = client->uptime;
                break;
            }
            visited_nodes++;
        }
    }
    hailer_shmList_unlock(shmList->sem_lock_id);
}

/* Update the DeviceList and the file based on the latest Rcvd Broadcast packet */
void hailer_update_client_list(clientDesc_t *client)
{
    unsigned int newDevice = 0;

    if(hailer_is_client_exist(client) == FALSE)
    {
        HAILER_DBG_INFO("Adding new client to list\n");
        hailer_add_device_to_list(client);
        hailer_print_peer_list();
        newDevice = 1;
    }

    hailer_update_timestamp(client);
    hailer_delete_expired_devices();
    hailer_update_client_file();
    if(newDevice == 0)
    {
        free(client);
    }
}

/* Function to initialise the hailer shared memory list
 * For server we should create the shared memory and allocate enough memory
 * for MAX PEERS hailer will support and initialse all the peer data structures.
 * For all other applications using this shared memory, the 'client' init routine
 * should return the shared memory starting address, so that they can use the info
 */
hailerShmlist_t *hailer_srvr_shmlist_init()
{
    int shmid                = -1;
    int sem_lock_id          = -1;
    int shm_total_sz         = 0;
    hailerShmlist_t *shmList = NULL;
    int i                    = 0;
    nodeList_t *curr_node    = NULL;

    /* semaphore value, for semctl() */
    union semun
    {
        int val;
        struct semid_ds *buf;
        ushort * array;
    } sem_val;

    HAILER_DBG_INFO("Creating Hailer shmList\n");

    /* Calcualte the required size of the shared memory.
     * When any new data strucutres are added to the hailer
     * shared memory, include the extra memory required here
     */
    shm_total_sz = sizeof(hailerShmlist_t) + sizeof(nodeList_t);
    /* Create the required shared memory */
    shmid  = shmget((key_t)HAILER_SHMLIST_KEY, shm_total_sz, 0666|IPC_CREAT);
    if(shmid != -1)
    {
        HAILER_DBG_INFO("Created shmList shared memory\n");
    }
    else
    {
        HAILER_DBG_ERR("ShmList shared memory creation failed!!\n");
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

    /* Create Semaphore for synchronising the access to the shared memory */
    sem_lock_id = semget((key_t)HAILER_SHM_LCK_KEY, 1, 0666 | IPC_CREAT | IPC_EXCL);
    if(sem_lock_id != -1)
    {
        HAILER_DBG_INFO("semget() success\n");
    }
    else
    {
        HAILER_DBG_ERR("semget() failed\n");
        return NULL;
    }

    /* Intialize the semaphore to '1' */
    sem_val.val = 1;
    if (semctl(sem_lock_id, 0, SETVAL, sem_val) == -1)
    {
	    HAILER_DBG_ERR("Semaphore initialisation failed!");
	    return NULL;
    }

    shmList->sem_lock_id = sem_lock_id;
    shmList->activePeerCount = 0;
    shmList->shmid = shmid;
    shmList->shmaddr = shmList;

    HAILER_DBG_INFO("Intialising Hailer shmlist\n");

    /* Intialise the list */
    curr_node = shmList->shmlistHead;
    hailer_shmList_lock(shmList->sem_lock_id);

    for(i = 0; i < HAILER_MAX_PEERS; i++, curr_node++)
    {
        memset(curr_node, 0, sizeof(nodeList_t));
        curr_node->client.isUsed = FALSE;
        curr_node->next = curr_node + 1;
    }
    curr_node->next = NULL;
    hailer_shmList_unlock(shmList->sem_lock_id);

    return shmList;
}

/* Routine to process the info present in the Broadcat UDP packet Rcvd */
//TODO : Due to some reasons this logic is not working properly. Need to look into this.
int hailer_process_broadcast_packets(char *buffer ,struct sockaddr_in cliAddr)
{
    struct hostent *host;
    clientDesc_t *client = (clientDesc_t *)malloc(sizeof(clientDesc_t));
    json_object *jsonMsg;
    json_object *uptime;

    memset(client, 0, sizeof(client));
    jsonMsg = json_tokener_parse(buffer);
    json_object_object_get_ex(jsonMsg, UPTIME, &uptime);

    client->lastSeenTimestamp = time(NULL);
    client->ipAddr = cliAddr.sin_addr;
    client->uptime = json_object_get_double(uptime);

    host = gethostbyaddr(&cliAddr.sin_addr, sizeof(cliAddr.sin_addr), AF_INET);
    if(host == NULL)
    {
        json_object *hostname;
        json_object_object_get_ex(jsonMsg, HOSTNAME, &hostname);
        strncpy(client->hostname, json_object_get_string(hostname),json_object_get_string_len(hostname));
    }
    else
    {
        strncpy(client->hostname, host->h_name, sizeof(client->hostname));
    }
    
    hailer_update_client_list(client);
    return HAILER_SUCCESS;
}

int init_hailer_peer_discovery_rcv_socket(void)
{
    int ret = -1;
    int rcvSockFd = -1;
    struct sockaddr_in sendAddr;
    char buffer[MAX_BUFFRE_SIZE] = {0};
#if ENCRYPT_MSGS
    char decryptedMsg[MAX_BUFFRE_SIZE] = {0};
#endif // ENCRYPT_MSGS

    rcvSockFd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if(rcvSockFd <1)
    {
        HAILER_DBG_ERR("Error creating the socket");
        return HAILER_ERROR;
    }

    memset(&sendAddr, 0, sizeof(sendAddr));
    sendAddr.sin_addr.s_addr = INADDR_ANY;
    sendAddr.sin_family = AF_INET;
    sendAddr.sin_port = htons(HAILER_PEER_DISCOVERY_BROADCAST_PORT);

    if(bind(rcvSockFd, (struct sockaddr*)(&sendAddr), sizeof(sendAddr)) < 0)
    {
        perror("Error binding to the socket");
        return HAILER_ERROR;
    }

    return rcvSockFd;
}

void *hailer_broadcast_discovery_packets()
{
    struct sockaddr_in broadcastAddr;
    int sendSockFd = -1;
    int ret = -1;
    int EnableBroacast = 1;
    struct json_object *jsonMsg;
#if ENCRYPT_MSGS
    char encryptedMsg[MAX_BUFFRE_SIZE] = {0};
#endif //ENCRYPT_MSGS

    sendSockFd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if(sendSockFd <1)
    {
        perror("Error creating the socket");
        return -1;
    }

    ret = setsockopt(sendSockFd, SOL_SOCKET, SO_BROADCAST, &EnableBroacast, sizeof(EnableBroacast));
    if(ret)
    {
        HAILER_DBG_ERR("setsockopt failed");
    }

    memset(&broadcastAddr, 0, sizeof(broadcastAddr));
    broadcastAddr.sin_addr.s_addr = INADDR_ANY;
    // TODO : Need a logic to automatically find the braodcast address
    inet_pton(AF_INET, "192.168.1.255", &broadcastAddr.sin_addr);
    broadcastAddr.sin_port = htons(HAILER_PEER_DISCOVERY_RECV_PORT);

    while(g_keep_running == 1)
    {
        jsonMsg = json_object_new_object();
        hailer_fill_jsonMsg(jsonMsg);
#if ENCRYPT_MSGS
        hailer_encryptMsg((char*)json_object_to_json_string(jsonMsg), encryptedMsg);
        ret = sendto(sendSockFd, encryptedMsg, strlen(encryptedMsg), 0,(struct sockaddr*)(&broadcastAddr), sizeof(broadcastAddr));
#else
        ret = sendto(sendSockFd, json_object_to_json_string(jsonMsg), strlen(json_object_to_json_string(jsonMsg)), 0,(struct sockaddr*)(&broadcastAddr), sizeof(broadcastAddr));
#endif //ENCRYPT_MSGS
        if(ret < 0)
        {
            HAILER_DBG_ERR("Error sending the broadcast message, sendto() failed");
        }
        sleep(KEEP_ALIVE_BROADCAST_INT);
    }

    //pthread_join(recvThreadid, NULL);  //TODO: When the SIGINT is raised the braodcast thread is not closing.
    close(sendSockFd);
}