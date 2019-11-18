/*
 * simulate.c
 *
 * Implement your (parallel) simulation here!
 */

#include <stdio.h>
#include <stdlib.h>

#include "simulate.h"


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
 * num_threads: how many threads to use (excluding the main threads)
 * old_array: array of size i_max filled with data for t-1
 * current_array: array of size i_max filled with data for t
 * next_array: array of size i_max. You should fill this with t+1
 */
double *simulate(const int i_max, const int t_max, double *old_array,
        double *current_array, double *next_array)
{
    /* Simulate a fucking wave */
    int t, i;
    double *temp;

    for (t = 0; t < t_max; t++) {
        for (i = 1; i < i_max - 1; i++) {
            next_array[i] = 2 * current_array[i] - old_array[i] + c * 
                            (current_array[i - 1] - 
                            (2 * current_array[i] - current_array[i + 1]));
        }
        temp = old_array;
        old_array = current_array;
        current_array = next_array;
        next_array = temp;
    }

    /* You should return a pointer to the array with the final results. */
    return current_array;
}
