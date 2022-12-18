/**
 * @file   hailer_server.c
 * @author Anand S
 * @date   5 Feb 2022
 * @version 0.1
 * @brief
 *
 */

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
// #include <malloc.h>

/*******  HAILER specific headers  ********/
#include "./include/hailer.h"
#include "./include/hailer_app_id.h"
#include "./include/hailer_msgtype.h"
#include "./include/hailer_server.h"

/*******  Global variables  ********/
unsigned int g_max_fd = 0;
fd_set g_hailer_srvr_read_fds;
int g_hailer_srvr_lstn_fd        = -1;     /* descriptor to listen to msgs with the device */
int g_hailer_srvr_ntwrk_lstn_fd   = -1;     /* descriptor to listen to msgs coming from other devices */
unsigned g_keep_running = 1;
apps_data_t *g_apps_data_head = NULL;

void sig_handler(int signum)
{
    if(signum == SIGSEGV || signum == SIGTERM || signum == SIGINT)
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

int init_hailer_server_socket()
{
    int srvr_fd = -1, ret = -1;
    struct sockaddr_un srvr_addr;

    /* Create the server socket */
    if((srvr_fd = socket(AF_LOCAL, SOCK_STREAM, 0)) < 0)
    {
        HAILER_DBG_ERR("Failed to ceate the HAILER Server socket!! Exiting...");
        return HAILER_ERROR;
    }

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
        HAILER_DBG_ERR("Error Binding to Network listen socket. Exiting...");
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
        HAILER_DBG_INFO("Sent HAILER_MSG_APP_REG_SUCCESS event to AppId=%d", new_msg_hdr.rcvr_app_id);
    }
    return HAILER_SUCCESS;
}

int hailer_process_events()
{
    fd_set read_fds;
    int ret = HAILER_ERROR, i = 0;
    struct timeval tv = {1, 0};    /*Set select timeout = 1s*/
    hailer_msg_hdr msg_hdr;

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
                    ret = hailer_rcv_msg(fd, &msg_hdr);
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
            else
            {
                /* This msg is from an app within the node in which this hailer_server instance is running */
                ret = hailer_rcv_msg(i, &msg_hdr);
                if(strncmp(msg_hdr.rcvr_ip, "127.0.0.1", strlen("127.0.0.1")) != 0)
                {
                    /* This destination of this msg is an external node */
                    int len = msg_hdr.msg_len + sizeof(hailer_msg_hdr);
                    hailer_send_msg_to_netwk(&msg_hdr, len);
                }
                else
                {
                    /* Destination App is within this node*/
                    process_hailer_msgs(&msg_hdr);
                }
            }
        }
    }
    return HAILER_SUCCESS;
}

int main()
{
    /* Register the signal handler */
    signal(SIGSEGV, sig_handler);   /* Signum = 11  */
    signal(SIGINT, sig_handler);    /* Signum = 2  */
    signal(SIGTERM, sig_handler);   /* Signum = 15 */

    PRNT_GRN
    HAILER_DBG_INFO("Starting HAILER SERVER !");
    PRNT_RST

    /* Create a socket which will listen for msgs from other apps in this device */
    g_hailer_srvr_lstn_fd = init_hailer_server_socket();
    FD_SET(g_hailer_srvr_lstn_fd, &g_hailer_srvr_read_fds);
    UPDATE_MAXFD(g_max_fd, g_hailer_srvr_lstn_fd);

    /* Create a socket to listen to other nodes within the network */
    g_hailer_srvr_ntwrk_lstn_fd = init_hailer_server_network_socket();
    FD_SET(g_hailer_srvr_ntwrk_lstn_fd, &g_hailer_srvr_read_fds);
    UPDATE_MAXFD(g_max_fd, g_hailer_srvr_ntwrk_lstn_fd);

    while(g_keep_running)
    {
        hailer_process_events();
    }

    /* Make sure that we are removing the HAILER_SERVER_ADDRESS
     * file before exiting the HAILER. Also close the server sock
     */
    close(g_hailer_srvr_lstn_fd);
    close(g_hailer_srvr_ntwrk_lstn_fd);
    unlink(HAILER_SERVER_ADDRESS);
    PRNT_RED
    HAILER_DBG_INFO("HAILER Server Exiting!");
    PRNT_RST
    return 0;
}

