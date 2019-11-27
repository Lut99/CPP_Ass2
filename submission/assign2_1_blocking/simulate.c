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

    // Iterate over the timesteps
    for (t = 0; t < t_max; t++) {
        // First, make sure that current_array is up-to-date
        MPI_Status status;
        int number;

        // All processes but the first send and listen to the left
        if (process_rank != 0) {
            MPI_Send(&current_array[start_i], 1, MPI_DOUBLE, process_rank - 1, TAG_PREV_ITEM, MPI_COMM_WORLD);
            MPI_Recv(&current_array[start_i - 1], 1, MPI_DOUBLE, process_rank - 1, TAG_NEXT_ITEM, MPI_COMM_WORLD, &status);
        }

        // All processes but the last send and listen to the right
        if (process_rank != n_processes - 1) {
            MPI_Send(&current_array[stop_i - 1], 1, MPI_DOUBLE, process_rank + 1, TAG_NEXT_ITEM, MPI_COMM_WORLD);
            MPI_Recv(&current_array[stop_i], 1, MPI_DOUBLE, process_rank + 1, TAG_PREV_ITEM, MPI_COMM_WORLD, &status);
        }

        // Do the threads own computations
        for (i = start_i; i < stop_i; i++) {
            next_array[i] = 2 * current_array[i] - old_array[i] + c * 
                            (current_array[i - 1] - (2 * current_array[i] - 
                             current_array[i + 1]));
        }

        // At the end, swap local arrays around
        temp = old_array;
        old_array = current_array;
        current_array = next_array;
        next_array = temp;
    }

    // Main process: join together work from all other processes
    if (process_rank == 0) {
        // Receive from each process
        for (i = 1; i < n_processes; i++) {
            // Compute properties for this process
            process_start_i = i * process_share + 1;
            if (i == n_processes - 1) {
                // Take into account that the last process (potentially) does
                //   more work
                process_share = (i_max - 1) - process_start_i;
            }
            MPI_Recv(&current_array[process_start_i], process_share, MPI_DOUBLE, i, TAG_FINALIZE, MPI_COMM_WORLD, NULL);
        }
        MPI_Finalize();
        return current_array;
    } else {
        // Non-main process: Send the current array to the main so it can write
        MPI_Send(&current_array[start_i], stop_i - start_i, MPI_DOUBLE, 0, TAG_FINALIZE, MPI_COMM_WORLD);
        
        MPI_Finalize();
        // Exit program without returning so only the main process return 
        // the end-result
        exit(0);
    }
}
