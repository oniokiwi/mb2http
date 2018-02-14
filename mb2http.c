#include <stdio.h>
#include "mb2http.h"
#include "typedefs.h"
#include <modbus/modbus.h>
#include <string.h>
#include <time.h>
#include <curl/curl.h>

#define MAX_PATH 1024

// Private data
static CURL* curl;
static modbus_mapping_t *mb_mapping;
static modbus_t *ctx;

static const int msbAddressIndex = 8;
static const int lsbAddressIndex = 9;
static const int msbDataIndex = 10;
static const int lsbDataIndex = 11;

static const char soc_log_path[]   = "/workspace/mb2http/soc.log";
static int stateOfCharge           = 500; // default state of charge set to 50%

static char url[128] = {0}; // "http://192.168.1.66:1880/powerToDeliverkW";
static char filename[MAX_PATH];
static char ipaddress[16];
static int  port;

//
// Lookup table for process functions
//
const process_table_t process_table[] =
{
    {RealPowerSetPoint,  process_RealPowerSetPoint},
    {ReadStateOfCharge,  process_ReadStateOfCharge},
    { 0,                                      NULL}
};

long long current_timestamp()
{
    struct timeval tv;
    gettimeofday(&tv, NULL); // get current time
    long long milliseconds = tv.tv_sec*1000LL + tv.tv_usec/1000; // caculate milliseconds
    return milliseconds;
}

int getStateOfCharge()
{
    FILE *fp;
    fp = fopen ( filename, "r");  // ../soc.log
    if ( fp )
    {
        fscanf(fp,"%d", &stateOfCharge);
        fclose(fp);
    }
    return stateOfCharge;
}

//
// Real power command in kW: range(-32768  to 32767)
//
int process_ReadStateOfCharge(uint8_t* pval)
{
    int getStateOfCharge();
    uint16_t *address;
    uint16_t address_offset;
    int retval = MODBUS_SUCCESS; // need to figure out what this constant is

    //printf("%s - %d\n", __PRETTY_FUNCTION__, getStateOfCharge() );
    address_offset = mb_mapping->start_registers + ReadStateOfCharge;
    address = mb_mapping->tab_registers + address_offset;
    if ( address < (mb_mapping->tab_registers + mb_mapping-> nb_registers) )
    {
        address[0] = getStateOfCharge();  // register 2 offset 0
    }
    return retval;
}


//
// process_RealPowerSetPoint
//
int process_RealPowerSetPoint(uint8_t* pval)
{
    struct curl_slist *headers = NULL;
    int retval = MODBUS_SUCCESS;
    char power_buf[32];
    const uint16_t sign_bit_mask = 0x8000;

    uint16_t power =  ((pval[msbDataIndex] << 8) + pval[lsbDataIndex]);  // convert bytes to word
    //printf("%s - %d\n", __PRETTY_FUNCTION__, power );
    if ( power & sign_bit_mask )
    {
        power = ((~power) + 1 );
        sprintf(power_buf,"-%d", power);
    }
    else
    {
        sprintf(power_buf,"%d", power);
    }
    headers = curl_slist_append(headers, "Content-Type: text/plain; charset=UTF-8");
    sprintf(url, "http://%s:%d/powerToDeliver/%s", ipaddress, port, power_buf); // "http://192.168.1.66:1880/powerToDeliver";
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");

    if (curl) {
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_perform(curl);
        curl_slist_free_all(headers);
    }
    //printf("%s\n", url);
    return retval;
}

/*
***************************************************************************************************************
 \fn      process_handler(uint8_t* pdata)
 \brief   processess all incoming commands

 Process all input commands. The Modbus function code 0x17 which is not standard seems to exhibit non standaard
 data structure seen not belows.

 \note
      ReadWriteOperation = 0x17 has the following data format which is different from single read or write operation
      -----------------------------------------
      | FC | RS | RQ | WS | WQ | WC | WR x nn |
      -----------------------------------------
      0    1    3    5    7    9    11

      FC = Funcction Code, RS = Read start address, RQ = Read quantity, WS = Write start address,
      WQ = Write quantity, WC = Write count, WR x nn = Write data n bytes
**************************************************************************************************************
*/
//
// processess all incoming commands
//
void process_handler(uint8_t* pdata, int code)
{
    uint16_t address =  ((pdata[msbAddressIndex] << 8) + pdata[lsbAddressIndex]);  // convert bytes to word

    for ( const process_table_t *p = process_table; p->handler != 0; p++ )
    {
        if ( p -> address == address )
        {
        	modbus_reply(ctx, pdata, code, mb_mapping);
            p->handler(pdata);
            return;
        }
    }
    modbus_reply_exception(ctx, pdata, MODBUS_EXCEPTION_ILLEGAL_DATA_ADDRESS);
}

//
// set_module_parameters
//
void set_module_parameters(modbus_t *context, modbus_mapping_t* mapping, const char* ipaddr, int port_number)
{
    FILE *fp;
    char path[MAX_PATH];

    fp = popen("echo $HOME", "r");
    if (fp != NULL)
    {
        while (fgets(path, PATH_MAX, fp) != NULL)
        ;
        path[strlen(path) - 1] = '\0';
        sprintf(filename,"%s%s", path, soc_log_path); // build the path to the soc file
        pclose(fp);
    }
    curl = curl_easy_init();
    strcpy(ipaddress, ipaddr);
    port = port_number;
    mb_mapping = mapping;
    ctx = context;
}

void close_module()
{
    if (curl)
    {
        curl_easy_cleanup(curl);
    }
}
