/**
 * @file    hailer_cli.c
 * @author  Anand S
 * @date    27 Nov 2022
 * @version 0.1
 * @brief   CLI toll for hailer
 *
 */

/* General headers */
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

/* Hailer specific headers  */
#include "include/hailer.h"

/* Print all the devices and it's info present in the device shmList */
void hailer_cli_print_peer_list( )
{
    int             i = 0;
    int visited_nodes = 0;
    hailerShmlist_t *shmList = NULL;
    nodeList_t *curr_node = NULL;

    shmList = hailer_client_shmlist_init();
    if(shmList == NULL)
    {
        HAILER_DBG_ERR("hailer_client_shmlist_init() failed! \n");
        exit(-1);
    }

    hailer_shmList_lock(shmList->sem_lock_id);
    curr_node = shmList->shmlistHead;
    for(i = 0; (i < HAILER_MAX_PEERS && visited_nodes < shmList->activePeerCount); i++, curr_node++)
    {
        if(curr_node->client.isUsed == TRUE)
        {
            printf("\n");
            printf("IP : %s\n", inet_ntoa(curr_node->client.ipAddr));
            printf("Last seen time : %ld\n", curr_node->client.lastSeenTimestamp);
            printf("Last seen time : %Le\n", curr_node->client.uptime);
            printf("MAC : %s\n", curr_node->client.mac);
            printf("\n");
            visited_nodes++;
        }
    }
    hailer_shmList_unlock(shmList->sem_lock_id);
}

void usage(void)
{
    PRNT_GRN
    printf("\n    HAILER - Inter/intra node communication  Library \n\n");
    PRNT_RST
    printf("Usage: hailer_cli -s or --show\n");
    printf("       hailer_cli -h or --help\n");
}

/* Options supported by hailer cli */
const struct option options[] =
{
    {"show", no_argument, 0, 's'},
    {"help", no_argument, 0, 'h'},
    {NULL, 0, 0, '\0'}
};

int main(int argc, char *argv[])
{
    int opt;
    if(argc < 2)
    {
        usage();
        exit(0);
    }

    while ((opt = getopt_long(argc, argv, "sh", options, NULL))!= -1)
    {
        switch(opt)
        {
            case 's':
                /* Dump all peer's info in the network with hailer installed */
                hailer_cli_print_peer_list();
                break;

            case 'h':
            default :
                /* Not the right way of using halier CLI. Display Hailer usage info */
                usage();
                break;
        }
    }
    exit(0);
}
