#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <pthread.h>
#include "hailer.h"

/* Set this to 1 to test msg transfer between 2 nodes
 * in a network. Set it to 0 to test the IPC functionality
 * of HAILER
 */

int g_keep_running          = 1;
hailer_msg_handle g_msg_handle;
char myIpAddress[64]        = {0};
char OtherNodeIpAddress[64] = {0};
pthread_t testapp_sender_thread_id = 1;

void sig_handler(int signum)
{
    if(signum == SIGKILL || signum == SIGTERM || signum == SIGINT)
    {
        g_keep_running = 0;
        PRNT_RED
        printf("Recieved signal=%d. Stopping HAILER Server!", signum);
        PRNT_RST
    }
}

void usage(void)
{
    printf("usage: <testapp_name>  <Ip address of the other node> \n");
}

void *hailer_testapp_send_packets()
{
    hailer_msg_hdr msg_hdr;

    while(g_keep_running)
    {
        {
            strncpy(msg_hdr.rcvr_ip, OtherNodeIpAddress, strlen(OtherNodeIpAddress) + 1);
            //printf("rcvr_ip = %s\n", msg_hdr.rcvr_ip);
            msg_hdr.rcvr_app_id =  (g_msg_handle.app_id == APP_ID_APP_A) ? APP_ID_APP_B : APP_ID_APP_A;
            msg_hdr.sndr_app_id = g_msg_handle.app_id;
            msg_hdr.msg_type = (g_msg_handle.app_id == APP_ID_APP_A ) ? HAILER_MSG_1 : HAILER_MSG_2;
            hailer_send_msg(&msg_hdr, g_msg_handle.comm_fd);
            sleep(5);
        }
    }
}

int main(int argc, char *argv[])
{
    int ret = -1;
    fd_set read_fds;
    struct timeval tv = {5, 0};
    hailer_msg_hdr msg_hdr;

    if(argc < 2)
    {
        usage();
    }
    else
    {
        strncpy(OtherNodeIpAddress, argv[1], strlen(argv[1]));
        printf("OtherNodeIpAddress = %s\n", OtherNodeIpAddress);
    }
    /* Register the signal handler */
    signal(SIGKILL, sig_handler);   /* Signum = 9  */
    signal(SIGINT, sig_handler);    /* Signum = 2  */
    signal(SIGTERM, sig_handler);   /* Signum = 15 */

    if(hailer_app_register(&g_msg_handle,  APP_ID_APP_B) != HAILER_SUCCESS)
    {
        printf("hailer_app_register failed !! \n");
        return -1;
    }

    pthread_create(&testapp_sender_thread_id, NULL, hailer_testapp_send_packets, NULL);

    while(g_keep_running)
    {
        FD_SET(g_msg_handle.comm_fd, &read_fds);
        ret = select(g_msg_handle.comm_fd+1, &read_fds, NULL, NULL, &tv);

        if(ret > 0)
        {
            if (FD_ISSET(g_msg_handle.comm_fd, &read_fds))
            {
               ret = hailer_rcv_msg(g_msg_handle.comm_fd, &msg_hdr);
               if(ret > 0)
               {
                    if( msg_hdr.msg_type < HAILER_MSG_TYPE_MAX)
                    {
                        printf("App B Recieved msg_type=%d from Appid=%d RcvrIp=%s\n", msg_hdr.msg_type, msg_hdr.sndr_app_id, msg_hdr.rcvr_ip);
                    }
               }
            }
        }
    }

    if(hailer_app_unregister(&g_msg_handle) != HAILER_SUCCESS)
    {
        printf("hailer_app_unregister failed !! \n");
        return -1;
    }
    else
    {
        printf("TestApp A exiting !! \n");
    }
    return 0;
}
