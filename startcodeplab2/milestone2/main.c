#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <time.h>

#include <sys/types.h>

#include "sensor_db.h"
#include "logger.h"
#include "config.h"

int main()
{
    create_log_process();
    FILE *f = open_db("sensor_db.csv", true);
    write_to_log_process("Data file opened.\n");

    sleep(1);
    sensor_id_t id = 1;
    sensor_value_t v = 0.001;
    sensor_ts_t ts = time(NULL);
    insert_sensor(f, id, v, ts);
    write_to_log_process("sensor 1 inserted.\n");

    id = 2;
    v = 0.002;
    ts = time(NULL);
    insert_sensor(f, id, v, ts);
    write_to_log_process("sensor 2 inserted.\n");
    id = 3;
    v = 0.003;
    ts = time(NULL);
    insert_sensor(f, id, v, ts);
    write_to_log_process("sensor 3 inserted.\n");
    sleep(5);
    insert_sensor(f, 4, v, ts);
    write_to_log_process("sensor 4 inserted.\n");

    close_db(f);
    write_to_log_process("Data file closed.\n");
    end_log_process();
    return 0;
}
