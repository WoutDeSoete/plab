//
// Created by wout on 11/19/25.
//
#include "sensor_db.h"
#include "config.h"

FILE * open_db(char * filename, bool append)
{
    if (filename == NULL) return NULL;

    FILE *f = fopen(filename, append? "a":"w");
    return f;
}

int insert_sensor(FILE * f, sensor_id_t id, sensor_value_t value, sensor_ts_t ts)
{
    if (f == NULL)return -1;

    if (fprintf(f,"%d, %f, %ld\n", id, value, ts) <0)return -1;
    fflush(f);
    return 0;
}

int close_db(FILE * f)
{
    return fclose(f);
}