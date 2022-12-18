#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include "hailer.h"

/* Set this to 1 to test msg transfer between 2 nodes
 * in a network. Set it to 0 to test the IPC functionality
 * of HAILER
 */
#define TEST_COMM_BTWN_NODES 1

int g_keep_running = 1;

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

int main()
{
    int ret = -1;
    hailer_msg_handle msg_handle;
    fd_set read_fds;
    struct timeval tv = {5, 0};
    hailer_msg_hdr msg_hdr;

    /* Register the signal handler */
    signal(SIGKILL, sig_handler);   /* Signum = 9  */
    signal(SIGINT, sig_handler);    /* Signum = 2  */
    signal(SIGTERM, sig_handler);   /* Signum = 15 */

    if(hailer_app_register(&msg_handle,  APP_ID_APP_B) != HAILER_SUCCESS)
    {
        printf("hailer_app_register failed !! \n");
        return -1;
    }

    while(g_keep_running)
    {
        FD_SET(msg_handle.comm_fd, &read_fds);
        ret = select(msg_handle.comm_fd+1, &read_fds, NULL, NULL, &tv);

        if(ret > 0)
        {
            if (FD_ISSET(msg_handle.comm_fd, &read_fds))
            {
               ret = hailer_rcv_msg(msg_handle.comm_fd, &msg_hdr);
               if(ret > 0)
               {
                    if(msg_hdr.msg_type == HAILER_MSG_APP_REG_SUCCESS)
                    {
                        printf("Recieved HAILER_MSG_APP_REG_SUCCESS event from HAILER Server. App successfully registered with the HAILER server\n");
                    }
                    else if( msg_hdr.msg_type < HAILER_MSG_TYPE_MAX)
                    {
                        printf("App B Recieved msg_type=%d from Appid=%d RcvrIp=%s\n", msg_hdr.msg_type, msg_hdr.sndr_app_id, msg_hdr.rcvr_ip);
                    }
                    else
                    {
                        printf("Error in Registering with HAILER Server\n");
                    }
                    if(TEST_COMM_BTWN_NODES)
                    {
                        /* Set the below hardcoded IP to the IP address of the Node in which AppA is running
                         * TODO: Convert this and the TEST_COMM_BTWN_NODES option as command line arguments
                         */
                        strncpy(&msg_hdr.rcvr_ip, "192.168.1.162", strlen("192.168.1.162") + 1);
                    }
                    msg_hdr.rcvr_app_id =  (msg_handle.app_id == APP_ID_APP_A) ? APP_ID_APP_B : APP_ID_APP_A;
                    msg_hdr.sndr_app_id = msg_handle.app_id;
                    msg_hdr.msg_type = (msg_handle.app_id == APP_ID_APP_A ) ? HAILER_MSG_1 : HAILER_MSG_2;
                    hailer_send_msg(&msg_hdr, msg_handle.comm_fd);
                    sleep(5);
               }
            }
        }
    }

    if(hailer_app_unregister(&msg_handle) != HAILER_SUCCESS)
    {
        printf("hailer_app_unregister failed !! \n");
        return -1;
    }
    else
    {
        printf("TestApp B exiting !! \n");
    }
    return 0;
}
