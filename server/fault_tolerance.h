/**
 *
 * @file fault_tolerance.h
 * @author Terraz Théophile
 * @date 2017-30-06
 *
 **/

#ifndef FAULT_TOLERANCE_H
#define FAULT_TOLERANCE_H

#include <stdio.h>
#include <stdlib.h>
#include <melissa_utils.h>
#include <vector.h>

//struct field_status_s
//{
//    char                   name[MAX_FIELD_NAME];
//    int32_t               *time_steps;
//    struct field_status_s *next;
//};

typedef struct field_status_s field_status_t;

struct melissa_simulation_s
{
    int id;
//    field_status_t *field_status;
//    int32_t *time_steps;
    int status;
    int timeout;
    int last_message;

};

typedef struct melissa_simulation_s melissa_simulation_t;

melissa_simulation_t* add_simulation(int id, int nb_time_steps);

simu_push_to(vector_t *v,
             int       pos,
             int       nb_time_steps);

void free_simu_vector(vector_t v);

int check_timeouts (int *simu_state, int *simu_timeouts, double *last_message_simu, int nb_simu);

void send_timeouts (int       detected_timeouts,
                    int *simu_timeouts,
                    int       nb_simu,
                    char*     txt_buffer,
                    void     *python_requester);

#endif // FAULT_TOLERANCE_H
