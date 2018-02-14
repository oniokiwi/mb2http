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
#include <stdbool.h>
#include <sys/socket.h>
#include "mb2http.h"
#include <pthread.h>
#include "main.h"
#include "typedefs.h"
#include "parseargs.h"

// Modbus networking info
#define MODBUS_DEFAULT_PORT 1502
#define TCP                 0


static uint8_t query[MODBUS_TCP_MAX_ADU_LENGTH];

int main(int argc, char*argv[])
{
	void close_module();
    modbus_t *ctx;
    modbus_mapping_t *mb_mapping;
    int s = -1;
    int rc;
    bool initialised = FALSE;
    bool done = FALSE;
    setvbuf(stdout, NULL, _IONBF, 0);                          // disable stdout buffering

    optargs_t args =
    {
        MODBUS_DEFAULT_PORT,
        HTTP_DEFAULT_DESTINATION_IP_ADDR,
		HTTP_DEFAULT_DESTINATION_PORT
    };

    setvbuf(stdout, NULL, _IONBF, 0);                          // disable stdout buffering
    rc = get_optarguments(argc, argv, &args);
    if ( rc < OPTARG_SUCCESS )
    {
        usage(argc, argv);
    }
    printf("mb2http arguments...\n");
    printf("port:%d HTTP destination IP address %s HTTP destination Port %d\n",
                    args.port, args.http_destination_ipaddress,   args.http_destination_port);

    for (;;)
    {
        ctx = modbus_new_tcp(NULL, args.port);
        modbus_set_debug(ctx, TRUE);

        if ( initialised == FALSE )
        {
            mb_mapping = modbus_mapping_new_start_address(
                0, 0,                     // BITS_ADDRESS, BITS_NB
                0, 0,                     // INPUT_BITS_ADDRESS, INPUT_BITS_NB
                0, 100,                   // REGISTERS_ADDRESS, REGISTERS_NB,
                0, 0);                    // INPUT_REGISTERS_ADDRESS, INPUT_REGISTERS_NB

            if (mb_mapping == NULL)
            {
                printf("Failed to allocate the mapping: %s\n", modbus_strerror(errno));
                modbus_free(ctx);
                return -1;
            }
            initialised = TRUE;
        }
        set_module_parameters(ctx, mb_mapping, args.http_destination_ipaddress, args.http_destination_port );
        printf("modbus listen TCP \n");

        s = modbus_tcp_listen(ctx, 1);
        modbus_tcp_accept(ctx, &s);
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
                close_module();
                ctx = NULL;
                done = TRUE;
                break;

            case 0:
                // No data received
                break;

            default:
            	process_handler(query, rc);
                continue;
            }
        }
    } // for (;;)
    modbus_mapping_free(mb_mapping);     // out of the loop to maintain register values
    return 0;
}
