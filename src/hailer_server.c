/**
 * @file   hailer_server.c
 * @author Anand S
 * @date   5 Feb 2022
 * @version 0.1
 * @brief
 *
 */

#define HAILER_PEER_DISCOVERY_BROADCAST 1
/*******  Generic headers  ********/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/shm.h>
#include <sys/sem.h>
#ifdef HAILER_PEER_DISCOVERY_BROADCAST
#include <pthread.h>
#include <sys/ioctl.h>
#include <linux/if.h>
#endif
// #include <malloc.h>

/*******  HAILER specific headers  ********/
#include "./include/hailer.h"
#include "./include/hailer_app_id.h"
#include "./include/hailer_msgtype.h"
#include "./include/hailer_server.h"
#ifdef HAILER_PEER_DISCOVERY_BROADCAST
#include "./include/hailer_peer_discovery.h"
#endif

#define HAILER_PEER_DISCOVERY_BROADCAST    1

/*******  Global variables  ********/
unsigned int g_max_fd              = 0;
unsigned int g_hailer_srver_loglvl = HAILER_LOGLEVEL_ERR;
fd_set g_hailer_srvr_read_fds;
int g_hailer_srvr_lstn_fd          = -1;     /* descriptor to listen to msgs with the device */
int g_hailer_srvr_ntwrk_lstn_fd    = -1;     /* descriptor to listen to msgs coming from other devices */
unsigned g_keep_running            = 1;
apps_data_t *g_apps_data_head      = NULL;
#ifdef HAILER_PEER_DISCOVERY_BROADCAST
int g_hailer_peer_discovery_rcv_fd = -1;
hailerShmlist_t *shmList           = NULL;
char g_hailer_interface[MAX_SIZE_80]    = {0};
#endif

void sig_handler(int signum)
{
    if( signum == SIGTERM || signum == SIGINT)
    {
        g_keep_running = 0;
        PRNT_RED
        HAILER_DBG_INFO("Recieved signal=%d. Stopping HAILER Server!", signum);
        PRNT_RST
    }
}

int get_commfd_from_app_id(app_id_t app_id)
{
    for(apps_data_t *app_data = g_apps_data_head; app_data != NULL; app_data = app_data->next)
    {
        if(app_data->app_id == app_id)
        {
            return app_data->fd;
        }
    }

    HAILER_DBG_ERR("Fd for appid=%d is not found", app_id);
    return -1;
}

apps_data_t *hailer_get_app_data_from_fd(int sock_fd)
{
    for(apps_data_t *app_data = g_apps_data_head; app_data != NULL; app_data = app_data->next)
    {
        if(app_data->fd == sock_fd)
        {
            return app_data;
        }
    }
    /* Couldn't find app with the given sock fd. return NULL */
    return NULL;
}

int init_hailer_server_socket()
{
    int srvr_fd = -1, ret = -1;
    struct sockaddr_un srvr_addr;
    int reuse = 1;

    /* Create the server socket */
    if((srvr_fd = socket(AF_LOCAL, SOCK_STREAM, 0)) < 0)
    {
        HAILER_DBG_ERR("Failed to ceate the HAILER Server socket!! Exiting...");
        return HAILER_ERROR;
    }

    if (setsockopt(srvr_fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse)) < 0)
    {
        HAILER_DBG_ERR("s etsockopt(SO_REUSEADDR) failed! \n");
        close(srvr_fd);
        return HAILER_ERROR;
    }

#ifdef SO_REUSEPORT
    if (setsockopt(srvr_fd, SOL_SOCKET, SO_REUSEPORT, (const char*)&reuse, sizeof(reuse)) < 0)
    {
        HAILER_DBG_ERR(" setsockopt(SO_REUSEPORT) failed! \n");
        close(srvr_fd);
        return HAILER_ERROR;
    }
#endif

    memset(&srvr_addr, 0, sizeof(srvr_addr));
    srvr_addr.sun_family = AF_LOCAL;
    strncpy(srvr_addr.sun_path, HAILER_SERVER_ADDRESS, sizeof(srvr_addr.sun_path));

    if((ret = bind(srvr_fd, (struct sockaddr *)&srvr_addr, sizeof(srvr_addr))) != 0)
    {
        HAILER_DBG_ERR("HAILER server socket bind failed!! ret=%d, errno=%d Exiting...", ret, errno);
        close(srvr_fd);
        return HAILER_ERROR;
    }

    if((ret = listen(srvr_fd, 5)) != 0)
    {
        HAILER_DBG_ERR("HAILER server listen() failed!! Exiting...");
        unlink(HAILER_SERVER_ADDRESS);
        close(srvr_fd);
        return HAILER_ERROR;
    }

    HAILER_DBG_INFO("Successfully created HAILER Server socket");
    return srvr_fd;
}

int init_hailer_server_network_socket()
{
    int rcv_sock_fd = -1;
    struct sockaddr_in send_addr;

    rcv_sock_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if(rcv_sock_fd < 1)
    {
        HAILER_DBG_ERR("HAILER server Network listen socket creation failed. Exiting...");
        exit(HAILER_ERROR);
    }

    memset(&send_addr, 0, sizeof(send_addr));
    send_addr.sin_addr.s_addr = INADDR_ANY;
    send_addr.sin_family = AF_INET;
    send_addr.sin_port = htons(HAILER_SERVER_PORT);

    if(bind(rcv_sock_fd, (struct sockaddr*)(&send_addr), sizeof(send_addr)) < 0)
    {
        HAILER_DBG_ERR("Error Binding to Network listen socket. errno = %d Exiting...\n", errno);
        exit(HAILER_ERROR);
    }

    return rcv_sock_fd;
}

static int hailer_send_msg_to_netwk(hailer_msg_hdr *msg_hdr, int len)
{
    struct sockaddr_in rcvr_addr;
    int send_sock_fd = -1;
    int ret = HAILER_ERROR;

    send_sock_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if(send_sock_fd < 1)
    {
        HAILER_DBG_ERR("socket() call failed!");
    }

    memset(&rcvr_addr, 0, sizeof(rcvr_addr));
    rcvr_addr.sin_family = AF_INET;
    inet_pton(AF_INET, msg_hdr->rcvr_ip, &rcvr_addr.sin_addr);
    rcvr_addr.sin_port = htons(HAILER_SERVER_PORT);

    ret = sendto(send_sock_fd, (char*)(msg_hdr), len, 0, (struct sockaddr*)(&rcvr_addr), sizeof(rcvr_addr));
    close(send_sock_fd);
    if(ret < len)
    {
        HAILER_DBG_ERR("sendto() call failed!. errno = %d", errno);
        return ret;
    }

    return HAILER_SUCCESS;
}

int hailer_srvr_send_msg_backto_sender(hailer_msg_hdr *msg_hdr)
{
    int rcvr_fd = -1;
    int ret     = HAILER_ERROR;

    rcvr_fd = get_commfd_from_app_id(msg_hdr->sndr_app_id);
    if(rcvr_fd > 0)
    {
        /* Swap the app Id's here */
        msg_hdr->rcvr_app_id = msg_hdr->sndr_app_id;
        msg_hdr->sndr_app_id = APP_ID_HAILER_SERVER;
        ret = write(rcvr_fd, msg_hdr, sizeof(hailer_msg_hdr) + msg_hdr->msg_len);
    }
    return HAILER_SUCCESS;
}

int process_hailerSrvr_get_logelvel_rqst(hailer_msg_hdr *msg_hdr)
{
    int loglevel = g_hailer_srver_loglvl;
    hailer_msg_hdr *rply_msg_hdr;
    char buf[1024] = {0};

    rply_msg_hdr = (hailer_msg_hdr *)buf;
    rply_msg_hdr->msg_len = sizeof(int);
    rply_msg_hdr->msg_type = HAILER_SRVR_RQST_RPLY_SUCCESS;
    rply_msg_hdr->rcvr_app_id = msg_hdr->rcvr_app_id;
    rply_msg_hdr->sndr_app_id = msg_hdr->sndr_app_id;
    memcpy((char*)(rply_msg_hdr + 1) , &loglevel, rply_msg_hdr->msg_len);
    hailer_srvr_send_msg_backto_sender(rply_msg_hdr);

    return HAILER_SUCCESS;
}

int process_hailerSrvr_set_logelvel_(hailer_msg_hdr *msg_hdr)
{
    int log = -1;
    log = *((char*)(msg_hdr +1));
    hailer_msg_hdr reply_msg_hdr;

    INITIALISE_HAILER_MSG_HDR(reply_msg_hdr);

    if(log < HAILER_LOGLEVEL_NONE || log > HAILER_LOGLEVEL_DBG)
    {
        reply_msg_hdr.msg_type = HAILER_SRVR_RQST_RPLY_ERROR;
    }
    else
    {
        g_hailer_srver_loglvl = log;
        HAILER_DBG_INFO("LOGLEVEL set to %d\n", g_hailer_srver_loglvl);
        reply_msg_hdr.msg_type = HAILER_SRVR_RQST_RPLY_SUCCESS;
    }
    reply_msg_hdr.rcvr_app_id = msg_hdr->rcvr_app_id;
    reply_msg_hdr.sndr_app_id = msg_hdr->sndr_app_id;
    hailer_srvr_send_msg_backto_sender(&reply_msg_hdr);

    return HAILER_SUCCESS;
}

int process_hailer_msgs(hailer_msg_hdr *msg_hdr)
{
    hailer_msg_hdr new_msg_hdr;
    int ret = HAILER_ERROR;
    int rcvr_fd = -1;

    if(msg_hdr == NULL)
    {
        HAILER_DBG_ERR("Invalid argumenets !!");
        return ret;
    }

    if(msg_hdr->rcvr_app_id != APP_ID_HAILER_SERVER)
    {
        rcvr_fd = get_commfd_from_app_id(msg_hdr->rcvr_app_id);
        if(rcvr_fd > 0)
        {
            ret = hailer_send_msg(msg_hdr, rcvr_fd);
            if(ret != HAILER_SUCCESS)
            {
                HAILER_DBG_ERR(" hailer_send_msg() failed, ret= %d", ret);
                return ret;
            }
        }
    }
    else if(msg_hdr->rcvr_app_id == APP_ID_HAILER_SERVER)
    {
        /* An app within this node has made a request. Process it */
        switch(msg_hdr->msg_type)
        {
            case HAILER_CLI_LOGLEVEL_GET:
            {
                process_hailerSrvr_get_logelvel_rqst(msg_hdr);
                break;
            }
            case HAILER_CLI_LOGLEVEL_SET:
            {
                process_hailerSrvr_set_logelvel_(msg_hdr);
                break;
            }
            default:
            {
                /* TODO Handle the unknown request type here */
                break;
            }
        }
    }
    return HAILER_SUCCESS;
}

int process_app_termination_event(int sockfd)
{
    apps_data_t *app_data = NULL;
    apps_data_t *prev_app = NULL;
    int found = FALSE;

    prev_app = g_apps_data_head;

    for(apps_data_t *app_data = g_apps_data_head; app_data != NULL; app_data = app_data->next)
    {
        if(app_data->fd == sockfd)
        {
            HAILER_DBG_INFO("AppId = %d terminating!", app_data->app_id);
            if(app_data == g_apps_data_head)
            {
                /* app_data is in the head node */
                g_apps_data_head = app_data->next;
                free(app_data);
                found = TRUE;
                break;
            }
            else
            {
                prev_app->next = app_data->next;
                free(app_data);
                found = TRUE;
                break;
            }
        }
        prev_app = app_data;
    }
    if(found  == FALSE)
    {
        HAILER_DBG_ERR("Failed to find the app_data!");
        return HAILER_ERROR;
    }
    else
    {
        /* Remove the fd from readlist anc close it */
        FD_CLR(sockfd, &g_hailer_srvr_read_fds);
        if(sockfd == g_max_fd)
        {
            /* If sockfd == g_max_fd, reduce g_max_fd by 1 */
            g_max_fd--;
        }
        close(sockfd);
        HAILER_DBG_INFO("sockfd closed and redfds updated successfully!");
    }

    return HAILER_SUCCESS;
}

int process_app_launch_event(int fd, hailer_msg_hdr *msg_hdr)
{
    hailer_msg_hdr new_msg_hdr;
    int ret = HAILER_ERROR;

    if( msg_hdr == NULL  || fd < 0)
    {
        HAILER_DBG_ERR("Invalid argumenets !!");
        return ret;
    }

    INITIALISE_HAILER_MSG_HDR(new_msg_hdr);
    apps_data_t *new_app = (apps_data_t*) malloc(sizeof(apps_data_t));

    new_app->app_id = msg_hdr->sndr_app_id;
    new_app->fd = fd;

    if(g_apps_data_head == NULL)
    {
        g_apps_data_head = new_app;
        new_app->next = NULL;
    }
    else
    {
        new_app->next = g_apps_data_head;
        g_apps_data_head = new_app;
    }
    HAILER_DBG_INFO("App added to appsData structure, appId=%d, fd = %d", new_app->app_id, new_app->fd);
    /* Add the new fd to ReadFds and update the maxFd so that HAILER server
     * can listen to events/msgs from the App
     */
    FD_SET(fd, &g_hailer_srvr_read_fds);
    UPDATE_MAXFD(g_max_fd, fd);

    /* Send the HAILER_APP_REG_SUCCESS event back to the app */
    new_msg_hdr.sndr_app_id = APP_ID_HAILER_SERVER;
    new_msg_hdr.rcvr_app_id = new_app->app_id;
    new_msg_hdr.msg_type = HAILER_MSG_APP_REG_SUCCESS;
    ret = write(fd, &new_msg_hdr, sizeof(new_msg_hdr));
    if(ret < sizeof(new_msg_hdr))
    {
        HAILER_DBG_ERR(" write() failed, ret= %d errno=%d", ret, errno);
        return ret;
    }
    else
    {
        HAILER_DBG_INFO("Sent HAILER_MSG_APP_REG_SUCCESS event to AppId = %d, fd = %d, ret = %d",
                         new_msg_hdr.rcvr_app_id, fd, ret);
    }
    return HAILER_SUCCESS;
}

int hailer_process_events()
{
    fd_set read_fds;
    int ret = HAILER_ERROR, i = 0;
    struct timeval tv = {1, 0};    /*Set select timeout = 1s*/
    hailer_msg_hdr msg_hdr;
    char buffer[MAX_BUFFRE_SIZE] = {0};
#if ENCRYPT_MSGS
    char decryptedMsg[MAX_BUFFRE_SIZE] = {0};
#endif // ENCRYPT_MSGS

    FD_ZERO(&read_fds);
    read_fds = g_hailer_srvr_read_fds;
    ret = select(g_max_fd+1, &read_fds, NULL, NULL, &tv);
    if(ret > 0)
    {
        for(i = 0; i <= g_max_fd+1; i++)
        {
            if (!FD_ISSET(i, &read_fds))
            {
                continue;
            }

            if(i == g_hailer_srvr_lstn_fd)
            {
                /* We have recieved an msg from an app for the first.
                 * First msg from an app should be App Launch Msg.
                 * Accept the connection and store the fd so that
                 * HAILER server can listen to them.
                 * */
                struct sockaddr_un client_addr;
                int sock_addr_size = 0;
                int fd = -1;

                sock_addr_size = sizeof(client_addr);

                if((fd = accept(g_hailer_srvr_lstn_fd, (struct sockaddr *)&client_addr, &sock_addr_size)) < 0)
                {
                    HAILER_DBG_ERR("HAILER server connection accept() failed!!");
                }
                else
                {
                    ret = read(fd, &msg_hdr, sizeof(hailer_msg_hdr));
                    /*Need to implement a logic to parse custom msgs embedded inside the hailer_msg_hdr*/
                    if(ret < 0)
                    {
                        HAILER_DBG_ERR("hailer_rcv_msg() failed!!");
                        close(fd);
                    }
                    else
                    {
                        if(msg_hdr.msg_type == HAILER_MSG_APP_LAUNCH && msg_hdr.rcvr_app_id == APP_ID_HAILER_SERVER)
                        {
                            HAILER_DBG_INFO("Recieved HAILER_MSG_APP_LAUNCH from AppId = %d", msg_hdr.sndr_app_id );
                            if((ret = process_app_launch_event(fd, &msg_hdr)) != HAILER_SUCCESS)
                            {
                                HAILER_DBG_ERR("process_app_launch_event() failed!!");
                                return ret;
                            }
                        }
                    }
                }
            }
            else if(i == g_hailer_srvr_ntwrk_lstn_fd)
            {
                /* We have got a messgae from an external node in the Network.
                 * Collect the msg and forward it to the destination app.
                 */
                struct sockaddr_in cli_addr;

                int cli_addr_len = sizeof(cli_addr);
                memset(&cli_addr, 0, sizeof(cli_addr));

                ret = recvfrom(g_hailer_srvr_ntwrk_lstn_fd, (char*)&msg_hdr, sizeof(msg_hdr), MSG_WAITALL, (struct sockaddr*)(&cli_addr), &cli_addr_len);
                if(ret >= sizeof(msg_hdr))
                {
                    process_hailer_msgs((hailer_msg_hdr*)&msg_hdr);
                }
                else
                {
                    HAILER_DBG_ERR("recvfrom() failed!!");
                }
            }
 #ifdef HAILER_PEER_DISCOVERY_BROADCAST
            else if(i == g_hailer_peer_discovery_rcv_fd)
            {
                /* We have recieved a node discovery broadcast msg from a peer device */
                struct sockaddr_in cli_addr;

                int cli_addr_len = sizeof(cli_addr);
                memset(&cli_addr, 0, sizeof(cli_addr));
                ret = recvfrom(g_hailer_peer_discovery_rcv_fd,
                               (char*)buffer, MAX_BUFFRE_SIZE,
                               MSG_WAITALL,
                               (struct sockaddr*)(&cli_addr), &cli_addr_len);

                if(ret > 0)
                {
                    buffer[ret] = '\0';
#if ENCRYPT_MSGS
                    hailer_decrypt_msg(buffer, decryptedMsg);
                    hailer_process_broadcast_packets(decryptedMsg, cli_addr);
#else
                    hailer_process_broadcast_packets(buffer, cli_addr);
#endif //ENCRYPT_MSGS
                }

            }
#endif
            else
            {
                /* This msg is from an app within the node in which this hailer_server instance is running */
                ret = read(i, &msg_hdr, 1024);
                if(ret  == 0)
                {
                    /* Looks like an app has terminated. Close the sockdt fd and remove the app data */
                    process_app_termination_event(i);
                    return HAILER_SUCCESS;
                }
                if(strncmp(msg_hdr.rcvr_ip, "127.0.0.1", strlen("127.0.0.1")) != 0)
                {
                    /* This destination of this msg is an external node */
                    int len = msg_hdr.msg_len + sizeof(hailer_msg_hdr);
                    hailer_send_msg_to_netwk(&msg_hdr, len);
                }
                else
                {
                    /* Destination App is within this node */
                    process_hailer_msgs(&msg_hdr);
                }
            }
        }
    }
    return HAILER_SUCCESS;
}

/* Check if the interface passed to the hailer server is up. */
static int hailer_is_interface_up(char *interface)
{
    int fd = -1, ret = -1;
    struct ifreq ifr;

    memset(&ifr, 0, sizeof(ifr));
    /* Create a fd for invoking ioctl call */
    fd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(fd == -1)
    {
        HAILER_DBG_ERR("socket() call failed!");
        return FALSE;
    }

    strncpy(ifr.ifr_name, interface, IFNAMSIZ);
    if( ret = ioctl(fd, SIOCGIFFLAGS, &ifr) == -1)
    {
        HAILER_DBG_ERR("ioctl call() failed! errno = %d, ret = %d\n",errno, ret);
        close(fd);
        return FALSE;
    }
    if ((ifr.ifr_flags & IFF_UP) == 0)
    {
        HAILER_DBG_ERR("%s interface is NOT up!\n", interface);
        close(fd);
        return FALSE;
    }

    close(fd);
    return TRUE;
}

/* Display info regarding, how to start hailer server */
static void hailer_server_usgae(void)
{
    PRNT_RED
    printf("usage: hailer_server <interface> \n");
    PRNT_RST
}

int main(int argc, char * argv[])
{

#ifdef HAILER_PEER_DISCOVERY_BROADCAST
    pthread_t hailer_broadcast_threadid = 1;
#endif

    if(argc < 2)
    {
        hailer_server_usgae();
    }
    else
    {
        if(argv[1] == NULL)
        {
            HAILER_DBG_ERR("Invalid interface");
            exit(-1);
        }
        else
        {
            if(hailer_is_interface_up(argv[1]) == FALSE)
            {
                HAILER_DBG_ERR("%s interface is not up. Hailer server exiting!!\n", argv[1]);
                exit(-1);
            }
            else
            {
                strncpy(g_hailer_interface, argv[1], sizeof(argv[1])+1);
                HAILER_DBG_INFO(" interface = %s", g_hailer_interface);
            }
        }
    }
    /* Register the signal handler */
    //signal(SIGSEGV, sig_handler);   /* Signum = 11  */
    signal(SIGINT, sig_handler);    /* Signum = 2  */
    signal(SIGTERM, sig_handler);   /* Signum = 15 */

    PRNT_GRN
    HAILER_DBG_INFO("Starting HAILER SERVER !");
    PRNT_RST

    FD_ZERO(&g_hailer_srvr_read_fds);

    /* Create a socket which will listen for msgs from other apps in this device */
    g_hailer_srvr_lstn_fd = init_hailer_server_socket();
    FD_SET(g_hailer_srvr_lstn_fd, &g_hailer_srvr_read_fds);
    UPDATE_MAXFD(g_max_fd, g_hailer_srvr_lstn_fd);

    /* Create a socket to listen to other nodes within the network */
    g_hailer_srvr_ntwrk_lstn_fd = init_hailer_server_network_socket();
    FD_SET(g_hailer_srvr_ntwrk_lstn_fd, &g_hailer_srvr_read_fds);
    UPDATE_MAXFD(g_max_fd, g_hailer_srvr_ntwrk_lstn_fd);

#ifdef HAILER_PEER_DISCOVERY_BROADCAST
    /* Create the shared memory to store the peers info which can be used by other process */
    shmList = hailer_srvr_shmlist_init();
    if(shmList == NULL)
    {
        PRNT_RED
        HAILER_DBG_INFO("Error creating shmlist shared memory\n. Exiting");
        PRNT_RST
        exit(-1);
    }
    /* Create a socket to listen to node discovery broadcast msgs*/
    g_hailer_peer_discovery_rcv_fd = init_hailer_peer_discovery_rcv_socket();
    FD_SET(g_hailer_peer_discovery_rcv_fd, &g_hailer_srvr_read_fds);
    UPDATE_MAXFD(g_max_fd, g_hailer_peer_discovery_rcv_fd);

    /* Create a thread that periodically send the broacast peer discovery packets */
    pthread_create(&hailer_broadcast_threadid, NULL, hailer_broadcast_discovery_packets, NULL);
#endif

    while(g_keep_running)
    {
        hailer_process_events();
    }

    /* Make sure that we are removing the HAILER_SERVER_ADDRESS
     * file before exiting the HAILER. Also close the server sock
     */
    close(g_hailer_srvr_lstn_fd);
    close(g_hailer_srvr_ntwrk_lstn_fd);
#ifdef HAILER_PEER_DISCOVERY_BROADCAST
    close(g_hailer_peer_discovery_rcv_fd);

    /* De-allocate the shared memory segment */
    if (semctl(shmList->sem_lock_id, 1, IPC_RMID) == -1)
    {
        PRNT_RED
        HAILER_DBG_ERR("Error in freeing shared memory segment\n");
        PRNT_RST
    }

    if(shmdt(shmList->shmaddr) == 0)
    {
        PRNT_GRN
        HAILER_DBG_INFO("shmList shared memory destroyed\n");
        PRNT_RST
    }
    else
    {
        PRNT_RED
        HAILER_DBG_ERR("Error in freeing the shmList shared memory\n");
        PRNT_RST
    }
#endif

    unlink(HAILER_SERVER_ADDRESS);
    PRNT_RED
    HAILER_DBG_INFO("HAILER Server Exiting!");
    PRNT_RST

    return 0;
}

