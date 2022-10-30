/**
 * @file    lan_monit.h
 * @author  Anand S
 * @date    26 Sep 2021
 * @version 0.1
 * @brief   Adule to see MAC addresses of all linux machines in the LAN which has installed this module through a keep alive message
 * 
 */

#ifndef HAILER_PEER_DISCOVERY_H
#define HAILER_PEER_DISCOVERY_H 1

#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include <netdb.h>
#include<time.h>

#include <json-c/json.h>

/*Feature configs*/
#define JSON_DBG       0  // Enable/Disable json debugging
#define ENCRYPTION_DBG 0  // Enable/Disable the encryption debugging
#define ENCRYPT_MSGS   1  // Enable/Disable Msg encryption

#define HAILER_PEER_DISCOVERY_BROADCAST_PORT   50002   // Broadcast port
#define HAILER_PEER_DISCOVERY_RECV_PORT        50002
#define MAX_BUFFRE_SIZE            1024 *2
#define MAX_HOSTNAME_SIZE          256
#define KEEP_ALIVE_BROADCAST_INT   5   //Send the Keep Alive Brodcast UDP Packet every 2 secs.
#define CLIENTS_LIST_FILE          "/tmp/hailer_peer_list"
#define TIME_TO_WAIT_TO_DEL_DEVICE 10   // wait 10 secs before deleting a device from the devicelist if we 
                                        // are not getting anymore KEEP ALIVE messages from it

/*JSON Msg keys*/

#define MSG      "msg"
#define HOSTNAME "hostname"
#define UPTIME   "uptime"

#ifndef FALSE
#define FALSE (0)
#endif //FALSE

#ifndef TRUE
#define TRUE (1)
#endif  //TRUE

#ifdef ENCRYPT_MSGS
/*Key used for encrypting and decrypting the messages*/
#define HAILER_ENCRYPTION_KEY    "VAYIKKANAARUMINGOTTVARENDA"
#endif //
typedef struct _clientDesc
{
    struct in_addr ipAddr;
    char hostname[MAX_HOSTNAME_SIZE];
    char mac[18];
    time_t lastSeenTimestamp;
    long double uptime;
} clientDesc_t;

typedef struct _nodeList
{
    clientDesc_t *client;
    struct _nodeList *next;
} nodeList_t;

/* Function prototypes */
int  init_hailer_peer_discovery_rcv_socket(void);
void *hailer_broadcast_discovery_packets();
int  hailer_process_broadcast_packets(char *buffer ,struct sockaddr_in cliAddr);
void hailer_decrypt_msg(char * encryptedMsg, char* decryptedMsg);

#endif //LAN_MONIT_H