/*
 * Copyright © kiwipower 2017
 *
 * Header file to simulate battery charge/discharge profile
 */
#ifndef MBTOHTTP_DOT_H
#define MBTOHTTP_DOT_H

#include <stdint.h>
#include <modbus/modbus.h>
#include "typedefs.h"

#define StateOfCharge      1
#define PowerToDeliver     2

int   process_multiple_registers(uint16_t start_address, uint16_t quantity, uint8_t* pdata);
int   process_handler(uint16_t, uint16_t);
int   process_query(modbus_pdu_t*);
void *microhttpd_handler( void *ptr );

// proclet
int   process_getStateOfCharge ();
int   process_setPowerToDeliver (uint16_t );

#endif
