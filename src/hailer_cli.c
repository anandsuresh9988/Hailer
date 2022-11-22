#include <stdio.h>
#include <stdlib.h>

#include "include/hailer.h"

/* Print all the devices and it's info present in the device shmList */
void hailer_cli_print_peer_list(hailerShmlist_t *shmList )
{
    int             i = 0;
    int visited_nodes = 0;
    nodeList_t *curr_node = shmList->shmlistHead;

    hailer_shmList_lock(shmList->sem_lock_id);

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

int main(void)
{
    hailerShmlist_t *shmList = NULL;

    shmList = hailer_client_shmlist_init();
    if(shmList == NULL)
    {
        HAILER_DBG_ERR("hailer_client_shmlist_init() failed! \n");
        exit(-1);
    }

    hailer_cli_print_peer_list(shmList);
    exit(0);
}
