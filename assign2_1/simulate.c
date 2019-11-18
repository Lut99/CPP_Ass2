/*
 * simulate.c
 *
 * Implement your (parallel) simulation here!
 */

#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

#include "simulate.h"

#define TAG_PREV_ITEM 0
#define TAG_NEXT_ITEM 1
#define TAG_FINALIZE 2


/* Add any global variables you may need. */
const double c = 0.15;


/* Add any functions you may need (like a worker) here. */
inline double compute(int i, double *old_array, double *current_array) {
    return 2 * current_array[i] - old_array[i] + c * 
           (current_array[i - 1] - (2 * current_array[i] - 
            current_array[i + 1]));
}


/*
 * Executes the entire simulation.
 *
 * Implement your code here.
 *
 * i_max: how many data points are on a single wave
 * t_max: how many iterations the simulation should run
 * old_array: array of size i_max filled with data for t-1
 * current_array: array of size i_max filled with data for t
 * next_array: array of size i_max. You should fill this with t+1
 */
double *simulate(const int i_max, const int t_max, double *old_array,
        double *current_array, double *next_array)
{
    int t, i, n_processes, process_rank, process_start_i, start_i, stop_i, process_share;
    double *temp;
    MPI_Request send_L, send_R, recv_L, recv_R;
    MPI_Status dummy;

    MPI_Init(NULL, NULL);

    // Acquire information about the world
    MPI_Comm_size(MPI_COMM_WORLD, &n_processes);
    MPI_Comm_rank(MPI_COMM_WORLD, &process_rank);

    // Compute what most processes' share is
    process_share = (i_max - 2) / n_processes;
    // Compute the boundries of the to-be-computed area. Note: stop_i is
    //   exclusive.
    start_i = process_rank * process_share + 1;
    stop_i = start_i + process_share;
    // The last process should receive the overflowing items
    if (process_rank == n_processes - 1) {
        stop_i = i_max - 1;
    }

    // Now that we know this, do the timesteps
    for (t = 0; t < t_max; t++) {
        // First: fire-and-forget the sends and receives
        if (process_rank != 0) {
            // Send to left
            MPI_Isend(&current_array[start_i], 1, MPI_DOUBLE, process_rank - 1, TAG_PREV_ITEM, MPI_COMM_WORLD, &send_L);
            MPI_Irecv(&current_array[start_i - 1], 1, MPI_DOUBLE, process_rank - 1, TAG_NEXT_ITEM, MPI_COMM_WORLD, &recv_L);
            // printf("prank=%i, recv_from=%i\n", process_rank, process_rank - 1);
        }
        if (process_rank != n_processes - 1) {
            // Send to right
            MPI_Isend(&current_array[stop_i - 1], 1, MPI_DOUBLE, process_rank + 1, TAG_NEXT_ITEM, MPI_COMM_WORLD, &send_R);
            // printf("prank=%i, sent_to=%i\n", process_rank, process_rank + 1);
            MPI_Irecv(&current_array[stop_i], 1, MPI_DOUBLE, process_rank + 1, TAG_PREV_ITEM, MPI_COMM_WORLD, &recv_R);
        }
        // printf("prank = %i, fired network operations\n", process_rank);

        // Now we can just do the loop normally
        for (i = start_i + 1; i < stop_i - 1; i++) {
            next_array[i] = compute(i, old_array, current_array);
            //printf("prank = %i, t: %i, i: %i\n", process_rank, t, i);
        }

        // printf("prank = %i, waiting for sends\n", process_rank);

        // For certainty, wait until sends are completed
        MPI_Wait(&send_L, &dummy);
        MPI_Wait(&send_R, &dummy);

        // printf("prank = %i, done waiting for sends\n", process_rank);

        // Only continue once all receives have completed
        if (process_rank != 0) {
            // printf("prank = %i, before recv_L\n", process_rank);
            MPI_Wait(&recv_L, &dummy);
            // printf("prank = %i, after recv_L\n", process_rank);
        }
        next_array[start_i] = compute(start_i, old_array, current_array);
        // printf("prank=%i, after computle_l\n", process_rank);
        if (process_rank != n_processes - 1) {
            // printf("prank = %i, before recv_R\n", process_rank);
            MPI_Wait(&recv_R, &dummy);
            // printf("prank = %i, after recv_R\n", process_rank);
        }
        next_array[stop_i - 1] = compute(stop_i - 1, old_array, current_array);

        // At the end, swap local arrays around
        temp = old_array;
        old_array = current_array;
        current_array = next_array;
        next_array = temp;
    }
    // printf("prank = %i, done w/timesteps\n", process_rank);

    // Join up to main
    if (process_rank == 0) {
        // THE MAIN: Receive from each process
        for (i = 1; i < n_processes; i++) {
            // Compute properties for this process
            process_start_i = i * process_share + 1;
            if (i == n_processes - 1) {
                // Take into account that the last process (potentially) does
                //   more work
                process_share = (i_max - 1) - process_start_i;
            }
            MPI_Recv(&current_array[process_start_i], process_share, MPI_DOUBLE, i, TAG_FINALIZE, MPI_COMM_WORLD, &dummy);
        }
        MPI_Finalize();
        return current_array;
    } else {
        /* Send the current array to the main so it can write */
        MPI_Send(&current_array[start_i], stop_i - start_i, MPI_DOUBLE, 0, TAG_FINALIZE, MPI_COMM_WORLD);
        MPI_Finalize();
        // Exit program
        exit(0);
    }
}
