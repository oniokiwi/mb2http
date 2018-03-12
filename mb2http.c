/*
     This file is part of libmicrohttpd
     Copyright (C) 2007 Christian Grothoff (and other contributing authors)

     This library is free software; you can redistribute it and/or
     modify it under the terms of the GNU Lesser General Public
     License as published by the Free Software Foundation; either
     version 2.1 of the License, or (at your option) any later version.

     This library is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
     Lesser General Public License for more details.

     You should have received a copy of the GNU Lesser General Public
     License along with this library; if not, write to the Free Software
     Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <microhttpd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <json.h>
#include <sys/syscall.h>
#include "typedefs.h"
#include "mb2http.h"
#include "curl_handler.h"

#define MAX_PATH 1024

// Private data
static modbus_mapping_t *mb_mapping;
static modbus_t* ctx;
static unsigned short stateOfCharge;

int process_DebugEnable(uint16_t data)
{
	bool val =  data & 0x0001;
	printf("%s - %s\n", __PRETTY_FUNCTION__, val?"TRUE":"FALSE");
	modbus_set_debug(ctx, val);
}

int process_getStateOfCharge ()
{
    uint16_t *address;
    uint16_t address_offset;
    int retval = MODBUS_SUCCESS; // need to figure out what this constant is

    address_offset = mb_mapping->start_registers + StateOfCharge;
    address = mb_mapping->tab_registers + address_offset;
    *address =  stateOfCharge;

    return retval;
}

int process_setPowerToDeliver (uint16_t data )
{
    int retval = MODBUS_SUCCESS;

    curl_sendPowerToDeliver(data);
    return retval;
}


void remove_character(char *buffer, int character)
{
    char *s,*d;

    d = buffer;

    for ( s = buffer; *s != '\0'; s++)
    {
        if ( *s != character )
        {
            *d++ = *s;
        }
    }
    *d = '\0';
}

void parse_json(const char* str)
{
    struct json_object *object, *tmp, *jobj;
    int length;
    char buf[128];
    float p,s;
    long int t;

    jobj = json_tokener_parse(str);

    // key and val don't exist outside of this bloc
    json_object_object_foreach(jobj, key, val)
    {
        switch (json_object_get_type(val))
        {
            case json_type_array:
                length = json_object_array_length(val);
                tmp = json_object_array_get_idx(val, length -1);
                strcpy(buf, json_object_to_json_string(tmp));
                remove_character(buf,' ');
                sscanf(buf,"{ \"timestamp\": %ld, \"powerDeliveredkW\": %f, \"stateOfCharge\": %f }", &t, &p, &s);
                stateOfCharge = (uint16_t)s;
                break;
        }
    }
    json_object_put(jobj);
}

static int ahc_echo(void * cls,
            struct MHD_Connection * connection,
            const char * url,
            const char * method,
            const char * version,
            const char * upload_data,
            size_t * upload_data_size,
            void ** ptr)
{
    const char * page = cls;
    struct MHD_Response * response;
    int reply_status;
    int ret;
    post_data_t *post = NULL;

    if (0 != strcmp(method, "PUT"))
    {
        return MHD_NO; /* unexpected method */
    }
    post = (post_data_t*)*ptr;
    if(post == NULL)
    {
        post = malloc(sizeof(post_data_t));
        post->status = false;
        *ptr = post;
    }
    if(!post->status)
    {
        post->status = true;
        return MHD_YES;
    }
    else
    {
        static int length = 0;
        if(*upload_data_size != 0)
        {
            length = *upload_data_size + 1;         // add space for null character
            *upload_data_size = 0;
            post->buff = malloc(length);
            post->buff[length] = '\0';             // ensure null termination
            snprintf(post->buff, length,"%s",upload_data);
            return MHD_YES;
        }
        else
        {
            parse_json((const char*)post->buff);
            curl_sendReadings((const char*)post->buff, length);
            free(post->buff);
        }
    }
    if(post != NULL)
    {
        free(post);
    }
    response = MHD_create_response_from_buffer (0, NULL,MHD_RESPMEM_PERSISTENT);
    ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
    MHD_destroy_response(response);
    return ret;
}

int process_write_multiple_addresses(uint16_t start_address, uint16_t quantity, uint8_t* pdata)
{
    return 0;
}


int  process_handler(uint16_t address, uint16_t data)
{
    switch (address)
    {
    case DebugEnable:
    	process_DebugEnable(data);
    	break;

    case StateOfCharge:
        process_getStateOfCharge();
        break;

    case PowerToDeliver:
        process_setPowerToDeliver(data);
        break;
    }

    return 0;
}
/*
***************************************************************************************************************
 \fn      process_handler(uint8_t* pdata)
 \brief   processess all incoming commands

 Process all input commands. The Modbus function code 0x17 which is not standard seems to exhibit non standaard
 data structure seen not belows.

 \note

      MODBUS_FC_READ_HOLDING_REGISTERS
      MODBUS_FC_WRITE_SINGLE_REGISTER - has the following data format
      ------------------------------------------------
      | TID | PID | LEN | UID | FC | [W|R]S | [W|R]Q |
      ------------------------------------------------
      0     1     3     5     7    8        11       13

      MODBUS_FC_WRITE_MULTIPLE_REGISTERS - has the following data format operation
      -------------------------------------------------------
      | TID | PID | LEN | UID | FC | WS | WQ | WC | WR x nn |
      -------------------------------------------------------
      0     1     3     5     7    8    11   13   14

      MODBUS_FC_WRITE_AND_READ_REGISTERS - has the following data format
      -----------------------------------------------------------------
      | TID | PID | LEN | UID | FC | RS | RQ | WS | WQ | WC | WR x nn |
      -----------------------------------------------------------------
      0     1     3     5     7    8    11   13   15   17   18

      TID = Transaction Id, PID = Protocol Id, LEN = Total length of message, UID = unit Id
      FC = Function Code, RS = Read start address, RQ = Read quantity, WS = Write start address,
      WQ = Write quantity, WC = Write count, WR = Write Register. nn => WQ x 2 bytes
**************************************************************************************************************
*/

int process_query(modbus_pdu_t* mb)
{
    const int convert_bytes2word_value = 256;
    int i,j,retval = MODBUS_EXCEPTION_ILLEGAL_DATA_ADDRESS;
    uint16_t address,value,count;
    int len = __bswap_16(mb->mbap.length) - 2; // len - fc - unit_id
    uint8_t fc;

    fc = mb->fcode;
    switch ( fc ){
    case MODBUS_FC_READ_HOLDING_REGISTERS:
        address = (mb->data[i++] * convert_bytes2word_value) + mb->data[i++]; // address
        value   = (mb->data[i++] * convert_bytes2word_value) + mb->data[i++]; // data
        retval  = process_handler(address, value);
        break;

    case MODBUS_FC_WRITE_SINGLE_REGISTER:
        address = (mb->data[i++] * convert_bytes2word_value) + mb->data[i++]; // address
        value   = (mb->data[i++] * convert_bytes2word_value) + mb->data[i++]; // data
        retval  = process_handler(address, value);
        break;

    case MODBUS_FC_WRITE_MULTIPLE_REGISTERS:
        address = (mb->data[i++] * convert_bytes2word_value) + mb->data[i++]; // address
        count = (mb->data[i++] * convert_bytes2word_value) + mb->data[i++];   // register count
        i++;                                             // skip over byte count
        retval = process_write_multiple_addresses(address, count, &mb->data[i]);
        i += (count*2);
        break;

    case MODBUS_FC_WRITE_AND_READ_REGISTERS:
        address = (mb->data[i++] * convert_bytes2word_value) + mb->data[i++]; // address
        value   = (mb->data[i++] * convert_bytes2word_value) + mb->data[i++]; // data
        retval  = process_handler(address, value);
        address = (mb->data[i++] * convert_bytes2word_value) + mb->data[i++]; // address
        count = (mb->data[i++] * convert_bytes2word_value) + mb->data[i++];   // register count
        i++;                                             // skip over byte count
        retval = process_write_multiple_addresses(address, count, &mb->data[i]);
        i += (count*2);
        break;

    default:
        retval = MODBUS_EXCEPTION_ILLEGAL_FUNCTION;
        break;
    }
    return retval;
}


void *microhttpd_handler( void *ptr )
{
    char *terminate;
    mhttpd_thread_param_t* param = (mhttpd_thread_param_t*) ptr;
    mb_mapping = param->mb_mapping;
    terminate = param->terminate;
    free(param);
    struct MHD_Daemon *d;

    d = MHD_start_daemon (MHD_USE_AUTO | MHD_USE_INTERNAL_POLLING_THREAD ,
                          8888,
                          NULL, NULL, &ahc_echo, NULL,
                          MHD_OPTION_CONNECTION_TIMEOUT, (unsigned int) 120,
                          MHD_OPTION_STRICT_FOR_CLIENT, (int) 1,
                          MHD_OPTION_END);

    if (NULL == daemon)
    {
        printf("unable to start microhttpd server\n");
        exit(1);
    }

    while (*terminate == false)
    ;
    MHD_stop_daemon (d);
    return 0;
}

void set_modbus_context(modbus_t* pctx)
{
	ctx = pctx;
}


