#ifndef HAILER_SERVER_H
#define HAILER_SERVER_H

/* Generic headers */
#include <string.h>

/* Hailer specific headers */
#include "hailer_app_id.h"
#include "hailer.h"

/* PORT used by HAILER Server to listen to msgs from other devices
 * MAke sure that we are using a PORT which is not used by other apps
 */
#define HAILER_SERVER_PORT (6773)

#define UPDATE_MAXFD(maxFd, fd) (maxFd = (fd > maxFd) ? fd : maxFd)

typedef struct _appsData
{
    app_id_t app_id;
    int fd;
    struct _appsData *next;
} apps_data_t;

#if (HAILER_DBG_ENABLE == 1)

#undef HAILER_DBG_ERR
#undef HAILER_DBG_INFO
#undef HAILER_DBG_DEBUG

    #define HAILER_DBG_ERR(fmt, args...) \
        do { \
                if(g_hailer_srver_loglvl >= HAILER_LOGLEVEL_ERR) \
                printf("HAILER_ERR: %s %d %s : " fmt "\n", __FILE__, __LINE__, __FUNCTION__, ##args); \
        } while(0)

    #define HAILER_DBG_INFO(fmt, args...) \
        do { \
                if(g_hailer_srver_loglvl >= HAILER_LOGLEVEL_INFO) \
                printf("HAILER_INFO: %s %d %s : " fmt "\n", __FILE__, __LINE__, __FUNCTION__, ##args); \
        } while(0)

     #define HAILER_DBG_DEBUG(fmt, args...) \
        do { \
                if(g_hailer_srver_loglvl >= HAILER_LOGLEVEL_DBG) \
                printf("HAILER_INFO: %s %d %s : " fmt "\n", __FILE__, __LINE__, __FUNCTION__, ##args); \
        } while(0)

#else

    #define HAILER_DBG_ERR(fmt, args...) while(0);
    #define HAILER_DBG_INFO(fmt, args...) while(0);

#endif //HAILER_DBG_ENABLE

hailerShmlist_t *hailer_srvr_shmlist_init(void);

#endif //HAILER_SERVER_H