/**
 * @file    hailer_cli.c
 * @author  Anand S
 * @date    27 Nov 2022
 * @version 0.1
 * @brief   CLI tool for hailer
 *
 */

/* General headers */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <time.h>

/* Hailer specific headers */
#include "include/hailer.h"

#define OPTIONAL_ARGUMENT_IS_PRESENT \
    ((optarg == NULL && optind < argc && argv[optind][0] != '-') \
     ? (uint8_t) (optarg = argv[optind++]) \
     : (optarg != NULL))

/* Covert secs to a string in the format HH:MM:SS */
static int hailer_secs_to_time_string(long double tiem_in_secs, char* time_str, int len)
{

    int secs = 0, mins = 0, hrs = 0;

    hrs  = (tiem_in_secs / 3600);
    mins = (tiem_in_secs - (hrs *3600)) / 60;
    secs = (tiem_in_secs - (hrs * 3600) - (mins *60));

    if(hrs != 0)
    {
        snprintf(time_str, len, "%d hrs, %d mins, %d secs", hrs, mins, secs);
    }
    else if (mins != 0)
    {
        snprintf(time_str, len, "%d mins, %d secs", mins, secs);
    }
    else
    {
        snprintf(time_str, len, "%d secs", secs);
    }

    return HAILER_SUCCESS;
}

/* Print all the devices and it's info present in the device shmList */
static void hailer_clli_print_peer_list(void)
{
    int i                        = 0;
    int visited_nodes            = 0;
    char timebuf[MAX_SIZE_80]    = {0};
    char uptime_str[MAX_SIZE_80] = {0};
    struct tm ts;
    hailerShmlist_t *shmList   = NULL;
    nodeList_t *curr_node      = NULL;

    shmList = hailer_client_shmlist_init();
    if(shmList == NULL)
    {
        printf("hailer_client_shmlist_init() failed! \n");
        exit(-1);
    }

    hailer_shmList_lock(shmList->sem_lock_id);
    curr_node = shmList->shmlistHead;
    for(i = 0; (i < HAILER_MAX_PEERS && visited_nodes < shmList->activePeerCount); i++, curr_node++)
    {
        if(curr_node->client.isUsed == TRUE)
        {
            /* Convert times into human readabale form */
            ts = *localtime(&(curr_node->client.lastSeenTimestamp));
            strftime(timebuf, sizeof(timebuf), "%a %Y-%m-%d %H:%M:%S %Z", &ts);

            hailer_secs_to_time_string(curr_node->client.uptime, uptime_str, MAX_SIZE_80);

            printf("\n");
            printf("IP             : %s\n", inet_ntoa(curr_node->client.ipAddr));
            printf("Last seen time : %s\n", timebuf);
            printf("Peer Uptime    : %s\n", uptime_str);
            printf("MAC            : %s\n", curr_node->client.mac);
            printf("\n");
            visited_nodes++;
        }
    }
    hailer_shmList_unlock(shmList->sem_lock_id);
}

/* Register hailer cli with server app to send and recieve messages */
static int hailer_cli_app_intialise(hailer_msg_handle *msg_handle)
{
    if(hailer_app_register(msg_handle, APP_ID_HAILER_CLI) != HAILER_SUCCESS)
    {
        printf("hailer_app_register failed !! \n");
        return HAILER_ERROR;
    }
    return HAILER_SUCCESS;
}

/* Send a msg to hailer server and wait for the reply */
static int hailer_cli_send_and_get_reply(hailer_msg_hdr *msghdr, hailer_msg_hdr *rply_msg_hdr,  hailer_msg_handle *msg_handle)
{
    hailer_send_msg(msghdr, msg_handle->comm_fd);
    hailer_rcv_msg(msg_handle->comm_fd, rply_msg_hdr);

    return HAILER_SUCCESS;
}

/* Routine to get the current loglevel of hailer server */
static int hailer_cli_get_logelevel(int *logelevel)
{
    hailer_msg_handle msg_handle;
    hailer_msg_hdr msg_hdr;
    hailer_msg_hdr *rply_msg_hdr = NULL;
    int log = 0;
    char buf[1024] = {0};
    rply_msg_hdr = (hailer_msg_hdr *)buf;

    INITIALISE_HAILER_MSG_HDR(msg_hdr);
    /* Register with hailer server */
    hailer_cli_app_intialise(&msg_handle);

    msg_hdr.sndr_app_id = msg_handle.app_id;
    msg_hdr.rcvr_app_id = APP_ID_HAILER_SERVER;
    msg_hdr.msg_type    = HAILER_CLI_LOGLEVEL_GET;

    /* Send the request to hailer server and wait for reply */
    hailer_cli_send_and_get_reply(&msg_hdr, rply_msg_hdr, &msg_handle);
    /* We have got the info from the server. De-register the app */
    hailer_app_unregister(&msg_handle);
    /* Extract the loglevel info from the reply msg */
    log = *((char*)(rply_msg_hdr +1));
    *logelevel = log;

    return HAILER_SUCCESS;
}

/* Routine to set the hailer server loglevel */
static int hailer_cli_set_logelevel(int loglevel)
{
    hailer_msg_handle msg_handle;
    hailer_msg_hdr *msg_hdr = NULL;
    hailer_msg_hdr rply_msg_hdr;
    char buf[1024] = {0};
    msg_hdr = (hailer_msg_hdr *)buf;

    INITIALISE_HAILER_MSG_HDR_PTR(msg_hdr);
    hailer_cli_app_intialise(&msg_handle);

    msg_hdr->msg_type    = HAILER_CLI_LOGLEVEL_SET;
    msg_hdr->sndr_app_id = msg_handle.app_id;
    msg_hdr->rcvr_app_id = APP_ID_HAILER_SERVER;
    msg_hdr->msg_len     = sizeof(int);
    memcpy((char*)(msg_hdr + 1) , &loglevel, msg_hdr->msg_len);
    hailer_cli_send_and_get_reply(msg_hdr, &rply_msg_hdr, &msg_handle);

    hailer_app_unregister(&msg_handle);
    if(rply_msg_hdr.msg_type != HAILER_SRVR_RQST_RPLY_SUCCESS)
    {
        return HAILER_ERROR;
    }
    return HAILER_SUCCESS;
}

/* Display Hailer CLI usage info */
static void usage(void)
{
    printf("Usage: hailer_cli -s or --show                -  Dump all available info about the peers in the network\n");
    printf("       hailer_cli -l or --loglevel [loglevel] -  Get/Set hailer loglevel\n");
    printf("       hailer_cli -h or --help                -  Display hailer CLI help text\n");
}

/* Options supported by hailer cli */
const struct option options[] =
{
    {"show",      no_argument,       0, 's'},
    {"logelevel", optional_argument, 0, 'l'},
    {"help",      no_argument,       0, 'h'},
    {NULL,        0,                 0, '\0'}
};

int main(int argc, char *argv[])
{
    int opt;

    if(argc < 2)
    {
        usage();
        exit(0);
    }

    while ((opt = getopt_long(argc, argv, "sl::h", options, NULL))!= -1)
    {
        PRNT_GRN
        printf("\n    HAILER - Inter/intra node communication  Library\n\n");
        PRNT_RST

        switch(opt)
        {
            case 's':
            {
                /* Dump all peer's info in the network with hailer installed */
                hailer_clli_print_peer_list();
                break;
            }

            case 'l':
            {
                if(OPTIONAL_ARGUMENT_IS_PRESENT)
                {
                    /* Optional argument is preset. So we need to set the server loglevel */
                    int loglevel = atoi(optarg);
                    if((loglevel < HAILER_LOGLEVEL_NONE) || (loglevel > HAILER_LOGLEVEL_DBG))
                    {
                        PRNT_RED
                        printf("Invalid loglevel!\n");
                        PRNT_RST
                    }
                    else
                    {
                        /* Send the new logelevl to the hailer server and get reply*/
                        if(hailer_cli_set_logelevel(loglevel) == HAILER_SUCCESS)
                        {
                            PRNT_GRN
                            printf("Hailer server loglevel set!\n");
                            PRNT_RST
                        }
                        else
                        {
                            PRNT_RED
                            printf("Loglevel set failed!\n");
                            PRNT_RST
                        }
                    }
                }
                else
                {
                    /* Print Hailer server loglevel */
                    int loglevel = -1;
                    hailer_cli_get_logelevel(&loglevel);
                    printf("Hailer server loglevel = %d\n", loglevel);
                }
                break;
            }

            case 'h':
            default :
            {
                /* Not the right way of using halier CLI. Display Hailer usage info */
                usage();
                break;
            }
        }
    }
    exit(0);
}
