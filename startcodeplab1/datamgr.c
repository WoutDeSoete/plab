#include <stdio.h>
#include "datamgr.h"

#include <assert.h>

#include "dplist.c"
#include <stdint.h>
#include <inttypes.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "lib/dplist.h"


dplist_t *list;

typedef uint16_t sensor_id_t, room_id_t;
typedef double sensor_value_t;

//internal data structures
typedef struct sensor_node {
    sensor_id_t sensor_id;
    room_id_t room_id;
    sensor_value_t buffer[RUN_AVG_LENGTH];
    int buf_index;
    int buf_count;
    double sum;
    time_t last_modified;
} sensor_node_t;

// Head of linked list
static sensor_node_t *head = NULL;

// helper functions

//Create a new sensor node
static sensor_node_t *create_node(sensor_id_t sid, uint16_t rid) {
    sensor_node_t *node = calloc(1, sizeof(sensor_node_t));
    ERROR_HANDLER(node == NULL, "Memory allocation failed");
    node->sensor_id = sid;
    node->room_id = rid;
    node->buf_index = 0;
    node->buf_count = 0;
    node->sum = 0.0;
    node->last_modified = 0;
    return node;
}

static sensor_node_t *find_sensor(uint16_t sensor_id) {
    int length = dpl_size(list);
    for (int i = 0; i < length; i++) {
        sensor_node_t *node = (sensor_node_t *)dpl_get_element_at_index(list, i);
        if (node->sensor_id == sensor_id) return node;
    }
    return NULL;
}

// update buffer with new value
static void update_buffer(sensor_node_t *n, sensor_value_t value, time_t ts) {
    if (n->buf_count < RUN_AVG_LENGTH) {
        n->buffer[n->buf_index] = value;
        n->sum += value;
        n->buf_count++;
    } else {
        /* Circular buffer replace oldest */
        n->sum -= n->buffer[n->buf_index];
        n->buffer[n->buf_index] = value;
        n->sum += value;
    }
    n->buf_index = (n->buf_index + 1) % RUN_AVG_LENGTH;
    n->last_modified = ts;
}

// Compute running average
static double compute_avg(sensor_node_t *n) {
    if (n->buf_count < RUN_AVG_LENGTH)
        return 0.0;
    return n->sum / (double)RUN_AVG_LENGTH;
}


static void *element_copy(void *src_element) {
    ERROR_HANDLER(src_element == NULL, "Null pointer passed to element_copy");
    sensor_node_t *src = (sensor_node_t *)src_element;
    sensor_node_t *dst = malloc(sizeof(sensor_node_t));
    ERROR_HANDLER(dst == NULL, "Memory allocation failed in element_copy");
    memcpy(dst, src, sizeof(sensor_node_t));
    return dst;
}

// Compare callback: compare sensor IDs
static int element_compare(void *x, void *y) {
    sensor_node_t *a = (sensor_node_t *)x;
    sensor_node_t *b = (sensor_node_t *)y;
    return (int)a->sensor_id - (int)b->sensor_id;
}

//Main functions


void datamgr_parse_sensor_files(FILE *fp_sensor_map, FILE *fp_sensor_data)
{
    mapping = dpl_create(element_copy, NULL, element_compare);
    list = dpl_create(element_copy, NULL, element_compare);

    ERROR_HANDLER(fp_sensor_map == NULL, "Map file pointer is NULL");
    ERROR_HANDLER(fp_sensor_data == NULL, "Sensor data file pointer is NULL");

    uint16_t room_id, sensor_id;
    while (fscanf(fp_sensor_map, "%hu %hu", &room_id, &sensor_id) == 2)
    {
        if (find_sensor(sensor_id) != NULL) {
            fprintf(stderr, "Duplicate sensor ID %u found in map file, ignoring\n", sensor_id);
            continue;
        }
        sensor_node_t *node = create_node(sensor_id, room_id);
        dpl_insert_at_index(list, node, 0, true);
    }


    while (1) {
        sensor_id_t sid;
        sensor_value_t temp;
        sensor_ts_t ts;

        size_t r1 = fread(&sid, sizeof(sensor_id_t), 1, fp_sensor_data);
        size_t r2 = fread(&temp, sizeof(sensor_value_t), 1, fp_sensor_data);
        size_t r3 = fread(&ts, sizeof(sensor_ts_t), 1, fp_sensor_data);
        if (r1 != 1 || r2 != 1 || r3 != 1) break;

        sensor_node_t *node = find_sensor(sid);
        if (node == NULL) {
            fprintf(stderr, "Warning: Received data for unknown sensor ID %u\n", sid);
            continue;
        }

        update_buffer(node, temp, ts);
        double avg = compute_avg(node);
        if (node->buf_count >= RUN_AVG_LENGTH) {
            if (avg > SET_MAX_TEMP)
                fprintf(stderr, "Room %" PRIu16 " is too hot (avg = %.2f°C)\n", node->room_id, avg);
            else if (avg < SET_MIN_TEMP)
                fprintf(stderr, "Room %" PRIu16 " is too cold (avg = %.2f°C)\n", node->room_id, avg);
        }
    }

}
