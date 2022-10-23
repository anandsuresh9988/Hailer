#ifndef HAILER_APP_ID_H
#define HAILER_APP_ID_H
/**
 * @file   hailer_app_id.h
 * @author Anand S
 * @date   30 Jan 2022
 * @version 0.1
 * @brief  header file to store the unique App ID of each application the network using the hailer
 *
 */

typedef enum app_id
{
    APP_ID_HAILER_SERVER = 0,
    APP_ID_APP_A,
    APP_ID_APP_B,

    HAILER_APP_ID_MAX, /* Don't put naything after this value*/
    HAILER_INVALID_APP_ID
} app_id_t;

#endif //HAILER_APPID_H