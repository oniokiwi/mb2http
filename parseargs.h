#ifndef PARSEARGS_DOT_H
#define PARSEARGS_DOT_H

#include "typedefs.h"

enum OPTARGS_ERROR
{
    OPTARG_SUCCESS = 0,
    OPTARG_BAD_NUMBER_OUTPUT_DISCRETE_COILS = -100,
    OPTARG_BAD_NUMBER_INPUT_DISCRETE_COILS,
    OPTARG_BAD_NUMBER_INPUT_REGISTER,
    OPTARG_BAD_NUMBER_OUTPUT_REGISTER,
};

const char* error_name(int error_id);
int usage(int argc, char** argv);
int get_optarguments(int argc, char*argv[], optargs_t* args);

#endif