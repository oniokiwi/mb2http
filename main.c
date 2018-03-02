/*
 * Copyright © 2008-2014 Stéphane Raimbault <stephane.raimbault@gmail.com>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <modbus/modbus.h>
#include <getopt.h>
#include <sys/socket.h>
#include <pthread.h>
#include <stdbool.h>
#include <sys/syscall.h>
#include "mb2http.h"
#include "curl_handler.h"
#include "typedefs.h"

#define POWER_TO_DELIVER_URL_DEFAULT "http://localhost:1880"
#define SUBMIT_READINGS_URL_DEFAULT  "http://localhost:1880/testpoint"

static char powerToDeliverURL[128];
static char submitReadingsURL[128];

const uint16_t UT_REGISTERS_NB = 0x07FF;
static char query[MODBUS_TCP_MAX_ADU_LENGTH];

#define MODBUS_DEFAULT_PORT 1502

static void init();
static pthread_t thread1,thread2;
static uint8_t terminate1, terminate2;
static modbus_mapping_t *mb_mapping;


static void usage(const char *app_name)
{
    printf("Usage:\n");
    printf("%s [option <value>] ...\n", app_name);
    printf("\nOptions:\n");
    printf(" -p \t\t # Set Modbus port to listen on for incoming requests (Default 1502)\n");
    printf(" -? \t\t # Print this help menu\n");
    printf("\nExamples:\n");
    printf("%s -p 1502  \t # Change the listen port to 1502\n", app_name);
    exit(1);
}

void init()
{
    mhttpd_thread_param_t* mhttpd_thread_param;
    curl_thread_param_t* curl_thread_param;

    mb_mapping = modbus_mapping_new_start_address(
       0, 0,
       0, 0,
       0, UT_REGISTERS_NB,
       0, 0);

    if (mb_mapping == NULL)
    {
        printf("Failed to allocate the mapping: %s\n", modbus_strerror(errno));
        exit(1); // all bets are off
    }
    setvbuf(stdout, NULL, _IONBF, 0);                          // disable stdout buffering
    terminate1 = FALSE;
    mhttpd_thread_param = malloc(sizeof (mhttpd_thread_param_t));
    mhttpd_thread_param -> mb_mapping = mb_mapping;
    mhttpd_thread_param -> terminate = &terminate1;
    pthread_create( &thread1, NULL, microhttpd_handler, mhttpd_thread_param);

    terminate2 = FALSE;
    curl_thread_param = malloc(sizeof (curl_thread_param_t));
    curl_thread_param -> terminate = &terminate2;
    strcpy(curl_thread_param->powerToDeliverURL, powerToDeliverURL);
    strcpy(curl_thread_param->submitReadingsURL, submitReadingsURL);
    pthread_create( &thread2, NULL, curl_handler, curl_thread_param);

}


int main(int argc, char*argv[])
{
    modbus_t *ctx;
    int rc, opt, s = -1, port = MODBUS_DEFAULT_PORT;
    bool done = FALSE;

    // setup some default URL's values
    strcpy(powerToDeliverURL, POWER_TO_DELIVER_URL_DEFAULT);
    strcpy(submitReadingsURL, SUBMIT_READINGS_URL_DEFAULT);

    while ((opt = getopt(argc, argv, "p:u:k:")) != -1)
    {
        switch (opt) {
        case 'p':
            port = atoi(optarg);
            break;
        case 'u':
        	strncpy(powerToDeliverURL, optarg, strlen(optarg));
            break;
        case 'k':
        	strncpy(submitReadingsURL, optarg, strlen(optarg));
            break;

        default:
            usage(*argv);
        }
    }
    init();

    printf("mb2http application - port (%d)\n", port);

    for (;;)
    {
        ctx = modbus_new_tcp(NULL, port);

        if ( ctx == NULL )
        {
            printf("Failed creating modbus context\n");
            return -1;
        }
        s = modbus_tcp_listen(ctx, 1);
        modbus_tcp_accept(ctx, &s);
        modbus_set_debug(ctx, TRUE);
        done = FALSE;
        while (!done)
        {
            rc = modbus_receive(ctx, query);
            switch (rc)
            {
            case -1:
                close(s); // close the socket
                modbus_close(ctx);
                modbus_free(ctx);
                ctx = NULL;
                done = TRUE;
                break;

            case 0:
                // No data received
                break;

            default:
                if (process_query((modbus_pdu_t*) query) == MODBUS_SUCCESS)
                    modbus_reply(ctx, query, rc, mb_mapping);
                else
                	modbus_reply_exception(ctx, query, rc);
                continue;
            }
        }
    } // for (;;)
    terminate1 = true;
    terminate2 = true;
    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);

    return 0;
}


