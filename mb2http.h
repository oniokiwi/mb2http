/*
 * Copyright © kiwipower 2017
 *
 * Header file to simulate battery charge/discharge profile
 */
#ifndef MB2HTTP_DOT_H
#define MB2HTTP_DOT_H

#include <stdint.h>
#include <modbus/modbus.h>

#define RealPowerSetPoint             1
#define ReadStateOfCharge             2

// proclet
int process_RealPowerSetPoint( uint8_t* );
int process_ReadStateOfCharge( uint8_t* );

void process_handler(uint8_t *, int code);
void set_module_parameters(modbus_t *context, modbus_mapping_t* mapping, const char* ipaddr, int port_number);
void tear_down();

#endif
