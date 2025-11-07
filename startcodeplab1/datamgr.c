#include <stdio.h>
#include "datamgr.h"
#include "dplist.c"
#include <stdint.h>
#include <inttypes.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>

#include "lib/dplist.h"


dpl_list_t mapping;
dpl_list_t list;


void datamgr_parse_sensor_files(FILE *fp_sensor_map, FILE *fp_sensor_data)
{
    mapping = dpl_create();

}
