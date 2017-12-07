/*
 * Header files for typedefs
 *
*/
#ifndef TYPEDEFS_DOT_H
#define TYPEDEFS_DOT_H

#include <modbus/modbus.h>
#include <stdbool.h>

#define MODBUS_SUCCESS  0
#define MAX_IP_ADDR  16

// HTTP destination networking info
#define HTTP_DEFAULT_DESTINATION_PORT      1880
#define HTTP_DEFAULT_DESTINATION_IP_ADDR  "192.168.1.66"

// HTTP source networking info
#define HTTP_DEFAULT_SOURCE_PORT           1902


typedef struct optargs_struct
{
    unsigned int port;                                       // port number for modbus server to listen
    char         http_destination_ipaddress[MAX_IP_ADDR];    // HTTP destination ip address
    unsigned int http_destination_port;                      // HTTP destination port number
}optargs_t;


typedef struct thread_params_struct
{
    modbus_t *ctx;
    modbus_mapping_t* mb_mapping;
    unsigned char *terminate;
    int  port;
}thread_param_t;

typedef struct post_data
{
    bool status;
    char *buff;
    int len;
}post_data_t;

typedef struct process_table_struct
{
    uint16_t address;
    int (*handler)(uint8_t*);
}process_table_t;

typedef struct process_endpoint_struct
{
    const char* endpoint;
    void (*handler)(const char *);
} process_endpoint_t;

#endif
