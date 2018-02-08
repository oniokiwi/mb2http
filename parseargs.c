#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include "parseargs.h"


extern const  uint16_t UT_REGISTERS_ADDRESS;
extern const  uint16_t UT_REGISTERS_NB;
extern const  uint16_t UT_REGISTERS_NB_MAX;
extern const  uint16_t UT_REGISTERS_TAB[];


const char* error_name(int error_id)
{
    switch ( error_id )
    {
    case OPTARG_SUCCESS:
        return "Success";

    case OPTARG_BAD_NUMBER_OUTPUT_DISCRETE_COILS:
        return "Bad number of output discrete coils given";

    case OPTARG_BAD_NUMBER_INPUT_DISCRETE_COILS:
        return "Bad number of input discrete coils given";

    case OPTARG_BAD_NUMBER_INPUT_REGISTER:
        return "Bad number of input registers given";

    case OPTARG_BAD_NUMBER_OUTPUT_REGISTER:
        return "Bad number of output registers given";

    default:
         /* intenitonally left blank" */
        break;
    }
    return NULL;
}

int usage(int argc, char** argv)
{
    printf("Usage:\n");
    printf("%s [option <value>] [option <value:number-bits>] [option <value:number_registers>] ...\n", argv[0]);
    printf("\nOptions:\n");
    printf(" -m | --modbus port\t\t\t # Set Modbus port to listen on for incoming requests (Default 1502)\n");
	printf(" -a | --HTTPDestinationIPAddress\t # The HTTP port destination address (Default %s)\n", HTTP_DEFAULT_DESTINATION_IP_ADDR);
	printf(" -p | --HTTPDestinationPort\t\t # The HTTP port destination number (Default %d)\n",HTTP_DEFAULT_DESTINATION_PORT);
    printf(" -h | --help\t\t\t\t # Print this help menu\n");
    printf("\nExamples:\n");
    printf("%s -m 1502 | --port 1502\t\t\t\t     # Change the listen port to 1502\n", argv[0]);
    printf("%s -a 192.168.1.66 | --HTTPDestinationIPAddress 192.168.1.66 # Send modbus target power to the given ip address.\n", argv[0]);
	printf("%s -p 8264 | --HTTPDestinationPort 8264 # Send modbus target power to the given port number.\n", argv[0]);
    exit(1);
}

int get_optarguments(int argc, char*argv[], optargs_t* args)
{
    int c;
    int retval = OPTARG_SUCCESS;

    while (1)
    {
        int this_option_optind = optind ? optind : 1;
        int option_index = 0;
        static struct option long_options[] =
        {
            {"port",                         required_argument, 0, 'm' },
            {"HTTPDestinationIPAddress     ",required_argument, 0, 'a' },
            {"HTTPDestinationIPPort        ",required_argument, 0, 'p' },
            {"help",                         required_argument, 0, 'h' },
            {0, 0, 0, 0 }
        };

        c = getopt_long(argc, argv, "m:a:p:h?", long_options, &option_index);
        if ((c == -1) || retval < 0)
            break;

        switch (c)
        {
            case 'm':
                args->port = atoi(optarg);
                break;

            case 'a':
            	strncpy(args->http_destination_ipaddress,optarg, MAX_IP_ADDR);
                break;

            case 'p':
                args->http_destination_port = atoi(optarg);
                break;

            case 'h':
                usage(argc, argv);
                break;

            case '?':
                break;

            default:
                printf("?? getopt returned character code 0%o ??\n", c);
        }
    }
    return retval;
}
