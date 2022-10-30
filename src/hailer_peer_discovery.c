/**
 * @file    hailer_node_discovery.c
 * @author  Anand S
 * @date    23 Oct 2022
 * @version 0.1
 * @brief   Module to see MAC addresses of all linux machines in the LAN which has installed this module through a keep alive message
 *
 */

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<signal.h>

#include "./include/hailer_peer_discovery.h"
#include "./include/hailer.h"


/*** Global variables ***/
extern unsigned int g_keep_running;
nodeList_t *g_clientListHead = NULL;

/* Print all the devices and it's info present in the deviceList */
void printList()
{
    nodeList_t *current = g_clientListHead;

    while (current != NULL)
    {
        printf("\n");
        printf("IP : %s\n", inet_ntoa(current->client->ipAddr));
        printf("Last seen time : %ld\n", current->client->lastSeenTimestamp);
        printf("Last seen time : %Le\n", current->client->uptime);
        printf("MAC : %s\n", current->client->mac);
        printf("\n");

        current = current->next;
    }
}

#if ENCRYPT_MSGS
/* Simple routine to encrypt the msg before sending */
void encryptMsg(char *Msg, char *EncryptedMsg)
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
    printf("\n====Encrypting Msg====\n\n");
    printf("    Msg:\n");

    for (i = 0; i <  strlen(Msg); i++)
    {
        printf("%02X ", Msg[i]);
    }

    printf("\n    Encrypted Msg:\n");

    for (i = 0; i <  strlen(EncryptedMsg); i++)
    {
        printf("%02X ", EncryptedMsg[i]);
    }

    printf("\n\n====Encrypting Msg Done !====\n");
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
    printf("\n====Decrypting Msg====\n");
    printf("    Encrypted Msg:\n");

    for (i = 0; i <  strlen(encryptedMsg); i++)
    {
        printf("%02X ", encryptedMsg[i]);
    }

    printf("\n    Decrypted Msg:\n");

    for (i = 0; i < strlen(decryptedMsg); i++)
    {
        printf("%02X ", decryptedMsg[i]);
    }

    printf("\n\n====Decrypting Msg Done====\n\n");
#endif //ENCRYPTION_DBG

}
#endif //ENCRYPT_MSGS
/*Get the device hostname using the 'hostname' command*/
void getDeviceHostname(char *hstname)
{
    char cmd[256] = {0};
    FILE *fp = NULL;

    sprintf(cmd , "hostname");

    fp = popen(cmd, "r");
    if(fp == NULL)
    {
        printf("Error executing the hostname command \n");
    }
    else
    {
        fscanf(fp, "%s", hstname);
        pclose(fp);
    }
}

/* Get the device uptime form /proc/uptime */
void getDeviceuptime(long double * uptime)
{
    char cmd[256] = {0};
    FILE *fp = NULL;

    sprintf(cmd, "%s", "cat /proc/uptime");

    fp = popen(cmd, "r");
    if(fp == NULL)
    {
        printf("Error obtaining the system uptime\n");
    }
    else
    {
        fscanf(fp, "%Le", uptime);
        pclose(fp);
    }
}

/* Fill the jsonMsg object with required fileds*/
void fillJsonMsg(struct json_object *jsonMsg)
{
    char hostname [MAX_HOSTNAME_SIZE] = {0};
    long double uptime;
    char *broadcast_msg = "Peer Boardcast messagae : Keep ALive";

    getDeviceHostname(hostname);
    getDeviceuptime(&uptime);
    json_object_object_add(jsonMsg, MSG, json_object_new_string(broadcast_msg));
    json_object_object_add(jsonMsg, HOSTNAME, json_object_new_string(hostname));
    json_object_object_add(jsonMsg, UPTIME, json_object_new_double(uptime));

#if JSON_DBG
    printf("jsonMsg : %s\n", json_object_to_json_string_ext(jsonMsg, JSON_C_TO_STRING_SPACED | JSON_C_TO_STRING_PRETTY));
#endif

}

/*Get the MAC address from the IP address using arp command*/
void findMACfromIp(char *ip, char *mac )
{
    char cmd[256] = {0};
    FILE *fp = NULL;

    sprintf(cmd, "arp -a | grep \"%s\" | awk '{print $4}'", ip);

    fp = popen(cmd,"r");
    if(fp == NULL)
    {
        printf("Error executing arp command to find the MAC address of %s", ip);
    }
    else
    {
       fscanf(fp, "%s", mac);
       printf("MAC of %s : %s\n", ip, mac);
       pclose(fp);
    }
}

/* Check whether the device with the client->ipAddr IP already exists in the DeviceList */
unsigned int isClientExists(clientDesc_t *client)
{
    unsigned int found = FALSE;
    nodeList_t *current = g_clientListHead;

    while(current != NULL)
    {
        //printf("%s %s\n",inet_ntoa(client->ipAddr) , inet_ntoa(current->client->ipAddr));
        /*TODO : Not sure why the below commented code didn't worked !!. Need to check*/
        //if(strcmp(inet_ntoa(client->ipAddr), inet_ntoa(current->client->ipAddr)) == 0 )
        if(client->ipAddr.s_addr == current->client->ipAddr.s_addr)
        {
            found = TRUE;
            //printf("Client already exists\n");
            break;
        }

        current = current->next;
    }

    return found;
}

/* Add the client to the DEviceList */
void addDeviceToList(clientDesc_t *client)
{
    nodeList_t *newNode = (nodeList_t*)malloc(sizeof(nodeList_t));
    nodeList_t *lastNode = g_clientListHead;

    memset(newNode, 0, sizeof(nodeList_t));

    newNode->client = client;
    strcpy(newNode->client->hostname, client->hostname);
    findMACfromIp(inet_ntoa(client->ipAddr), newNode->client->mac);
    newNode->client->ipAddr.s_addr = client->ipAddr.s_addr;
    newNode->client->lastSeenTimestamp = client->lastSeenTimestamp;
    newNode->client->uptime = client->uptime;

    newNode->next = NULL;

    if(lastNode == NULL)
    {
        printf("Adding first node to list\n");
        g_clientListHead = newNode;
    }
    else
    {
        printf("Adding a new node \n");
        while(lastNode->next != NULL)
        {
            lastNode = lastNode->next;
        }

        lastNode->next = newNode;
    }

    return;
}

/* Check whether any device which is currently not sending the Keep Alive messgae is present in the
   DeviceList. If present delete the device */
void deleteExpiredDevices()
{
    nodeList_t *current = g_clientListHead, *prev = NULL;

    time_t now = time(NULL);
    while(current != NULL)
    {
        if(now - current->client->lastSeenTimestamp > TIME_TO_WAIT_TO_DEL_DEVICE)
        {
            if(current == g_clientListHead)
            {
                g_clientListHead = current->next;
                free(current);
                current = g_clientListHead;
            }
            else
            {
                prev->next = current->next;
                free(current);
                current = prev->next;
            }
        }
        else
        {
           prev = current;
           current = current->next;
        }
    }
}

/* Update the file CLIENTS_LIST_FILE with the latest info available in the list pointed by g_clientListHead */
void updateClientFile()
{
    FILE *fp = fopen(CLIENTS_LIST_FILE, "w");
    nodeList_t *current = g_clientListHead;
    unsigned int count = 0;

    while(current != NULL)
    {
        count++;
        fprintf(fp, "Client NO: %d\n", count );
        fprintf(fp, "IP Address: %s\n", inet_ntoa(current->client->ipAddr));
        fprintf(fp, "Last seen time stamp: %ld\n", current->client->lastSeenTimestamp);
        fprintf(fp, "Uptime: %Le\n", current->client->uptime);
        fprintf(fp, "MAC: %s\n", current->client->mac);
        fprintf(fp, "\n");
        current = current->next;
    }

    fclose(fp);
}

/* If the device is already present in the list just update it's timestamp */
void updateTimestamp(clientDesc_t *client)
{
    nodeList_t *current = g_clientListHead;
    while(current != NULL)
    {
        /*TODO : Not sure why the below commented code didn't worked !!. Need to check*/
        //if(strncmp(inet_ntoa(client->ipAddr), inet_ntoa(current->client->ipAddr), sizeof(struct in_addr)) ==0 )
        if(client->ipAddr.s_addr == current->client->ipAddr.s_addr)
        {
            current->client->lastSeenTimestamp = client->lastSeenTimestamp;
            current->client->uptime = client->uptime;

            break;
        }
        current = current->next;
    }
}

/* Update the DeviceList and the file based on the latest Rcvd Broadcast packet */
void updateClientList(clientDesc_t *client)
{
    unsigned int newDevice = 0;
    if(isClientExists(client) == FALSE)
    {
        printf("Adding new client to list\n");
        addDeviceToList(client);
        printList();
        newDevice = 1;
    }

    updateTimestamp(client);
    deleteExpiredDevices();
    updateClientFile();
    if(newDevice == 0)
    {
        free(client);
    }
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
    //printf("Rcvd Msg from IP -%s \n", inet_ntoa(cliAddr.sin_addr));

    jsonMsg = json_tokener_parse(buffer);
    json_object_object_get_ex(jsonMsg, UPTIME, &uptime);

    client->lastSeenTimestamp = time(NULL);
    client->ipAddr = cliAddr.sin_addr;
    client->uptime = json_object_get_double(uptime);

    host = gethostbyaddr(&cliAddr.sin_addr, sizeof(cliAddr.sin_addr), AF_INET);
    if(host == NULL)
    {
        //printf("Error getting hostanme for IP %s. \
                Using value from the json formatted msg from the client\n",\
                                              inet_ntoa(cliAddr.sin_addr));
        json_object *hostname;
        json_object_object_get_ex(jsonMsg, HOSTNAME, &hostname);
        strncpy(client->hostname, json_object_get_string(hostname),json_object_get_string_len(hostname));
        //printf("Hostname : %s\n", client->hostname);
    }
    else
    {
        //printf("Hostname : %s\n", host->h_name);
        strncpy(client->hostname, host->h_name, sizeof(client->hostname));
    }

    updateClientList(client);
    //printf("\n");
    return 0;
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
        perror("Error creating the socket");
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
        perror("setsockopt failed");
    }

    memset(&broadcastAddr, 0, sizeof(broadcastAddr));
    broadcastAddr.sin_addr.s_addr = INADDR_ANY;
    // TODO : Need a logic to automatically find the braodcast address
    inet_pton(AF_INET, "192.168.1.255", &broadcastAddr.sin_addr);
    broadcastAddr.sin_port = htons(HAILER_PEER_DISCOVERY_RECV_PORT);

    while(g_keep_running == 1)
    {
        jsonMsg = json_object_new_object();
        fillJsonMsg(jsonMsg);
#if ENCRYPT_MSGS
        encryptMsg((char*)json_object_to_json_string(jsonMsg), encryptedMsg);
        ret = sendto(sendSockFd, encryptedMsg, strlen(encryptedMsg), 0,(struct sockaddr*)(&broadcastAddr), sizeof(broadcastAddr));
#else
        ret = sendto(sendSockFd, json_object_to_json_string(jsonMsg), strlen(json_object_to_json_string(jsonMsg)), 0,(struct sockaddr*)(&broadcastAddr), sizeof(broadcastAddr));
#endif //ENCRYPT_MSGS
        if(ret < 0)
        {
            perror("Error sending the broadcast message, sendto() failed");
        }
        sleep(KEEP_ALIVE_BROADCAST_INT);
    }

    //pthread_join(recvThreadid, NULL);  //TODO: When the SIGINT is raised the braodcast thread is not closing.
    close(sendSockFd);
}