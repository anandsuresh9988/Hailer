#ifndef HAILER_SERVER_H
#define HAILER_SERVER_H

#include "hailer_app_id.h"

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

#endif //HAILER_SERVER_H