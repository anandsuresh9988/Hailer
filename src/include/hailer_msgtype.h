#ifndef HAILER_MSGTYPE_H
#define HAILER_MSGTYPE_H
/**
 * @file   hailer_msgtype.h
 * @author Anand S
 * @date   30 Jan 2022
 * @version 0.1
 * @brief  header file to store the unique MSG TYPES used by hailer applications
 *
 */

typedef enum msg_type
{
    HAILER_MSG_APP_LAUNCH = 0,
    HAILER_MSG_APP_REG_SUCCESS,
    HAILER_CLI_LOGLEVEL_GET,
    HAILER_CLI_LOGLEVEL_SET,
    HAILER_SRVR_RQST_RPLY_SUCCESS,
    HAILER_SRVR_RQST_RPLY_ERROR,
    HAILER_MSG_1,
    HAILER_MSG_2,

    HAILER_MSG_TYPE_MAX, /* Don't put naything after this value*/
} msg_type_t;

#endif //HAILER_MSGTYPE_H
