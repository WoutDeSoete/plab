//
// Created by wout on 12/5/25.
//

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <pthread.h>
#include <inttypes.h>
#include <string.h>
#include "sbuffer.h"

#define SENSOR_FILE "sensor_data"
#define OUTPUT_CSV  "sensor_data_out.csv"
#define READER_COUNT 2

typedef struct {
    sbuffer_t *buf;
    FILE *out_csv;
    pthread_mutex_t *file_lock;
    int thread_num;
} reader_args_t;

typedef struct {
    sbuffer_t *buf;
    const char *input_path;
	pthread_mutex_t *file_lock;
} writer_args_t;

typedef struct {
    sensor_id_t sid;
    sensor_value_t temp;
    sensor_ts_t ts;
} sensor_record_t;


void *writer_thread(void *arg) {
    writer_args_t *writer = arg;
    sbuffer_t *buf = writer->buf;
    const char *path = writer->input_path;
	pthread_mutex_t *lock = writer->file_lock;
    FILE *sensor_data = fopen(path, "rb");
    if (!sensor_data) {
        perror("fopen sensor_data");
        //EOS so reader terminate
        sensor_data_t eos = {0, 0.0, 0};
        sbuffer_insert(buf, &eos);
        return NULL;
    }


    while (1) {
		sensor_id_t sid;
        sensor_value_t temp;
        sensor_ts_t ts;

        size_t r1 = fread(&sid, sizeof(sensor_id_t), 1, sensor_data);
        size_t r2 = fread(&temp, sizeof(sensor_value_t), 1, sensor_data);
        size_t r3 = fread(&ts, sizeof(sensor_ts_t), 1, sensor_data);
        if (r1 != 1 || r2 != 1 || r3 != 1)
        {
            if (feof(sensor_data)) break;
            else
			{
                perror("fread");
                break;
            }
        }

        sensor_data_t sd;
		sd.id = sid;
		sd.value = temp;
		sd.ts = ts;

		pthread_mutex_lock(lock);
        if (sbuffer_insert(buf, &sd) != 0) {
            fprintf(stderr, "Failed to insert into sbuffer\n");
            break;
        }
		pthread_mutex_unlock(lock);
        //insert one measurement every 10 ms
        usleep(10000);
    }

    fclose(sensor_data);

    // Insert EOS  per reader
    sensor_data_t eos = {0, 0.0, 0};
    for (int i = 0; i < READER_COUNT; ++i) {
        sbuffer_insert(buf, &eos);
    }
    return NULL;
}

void *reader_thread(void *arg) {
    reader_args_t *reader = arg;
    sbuffer_t *buf = reader->buf;
    FILE *out = reader->out_csv;
    pthread_mutex_t *lock = reader->file_lock;

    while (1) {
        sensor_data_t data;
        int r = sbuffer_remove(buf, &data);
        if (r != 0|| data.id == 0) {
            // EOS or no more data
            break;
        }

        // lock for writing in csv
        pthread_mutex_lock(lock);
        // header is writen in main
        fprintf(out, "%" PRIu32 ",%.6f,%" PRIu64 "\n",
                data.id, data.value, data.ts);
        fflush(out);
        pthread_mutex_unlock(lock);

        // Wait 25 ms after reading each measurement
        usleep(25000);
    }
    return NULL;
}

int main(void) {
    time_t start = time(NULL);
    sbuffer_t *buf;
	sbuffer_init(&buf);
    if (!buf) {
        fprintf(stderr, "Failed to init sbuffer\n");
        return 1;
    }

    // Open CSV for append and write header
    FILE *csv = fopen(OUTPUT_CSV, "a");
    if (!csv) {
        perror("fopen output csv");
        sbuffer_free(&buf);
        return 1;
    }

    pthread_t writer;
    pthread_t readers[READER_COUNT];

    pthread_mutex_t file_lock;
    pthread_mutex_init(&file_lock, NULL);

    writer_args_t wa = { .buf = buf, .input_path = SENSOR_FILE , .file_lock = &file_lock };
    if (pthread_create(&writer, NULL, writer_thread, &wa) != 0) {
        perror("pthread_create writer");
        fclose(csv);
        sbuffer_free(&buf);
        return 1;
    }

    reader_args_t readerarg[READER_COUNT];
    for (int i = 0; i < READER_COUNT; ++i) {
        readerarg[i].buf = buf;
        readerarg[i].out_csv = csv;
        readerarg[i].file_lock = &file_lock;
        readerarg[i].thread_num = i+1;
        if (pthread_create(&readers[i], NULL, reader_thread, &readerarg[i]) != 0) {
            perror("pthread_create reader");

            // signal EOS so others finish
            sensor_data_t eos = {0,0,0};
            for (int j=0;j<READER_COUNT;j++) sbuffer_insert(buf,&eos);
            break;
        }
    }

    // Wait for threads
    pthread_join(writer, NULL);
    for (int i = 0; i < READER_COUNT; ++i) pthread_join(readers[i], NULL);

    // cleanup
    pthread_mutex_destroy(&file_lock);
    fclose(csv);
    sbuffer_free(&buf);

    time_t end = time(NULL);
    printf("full time of program: %ld\n", end - start);
    return 0;
}
