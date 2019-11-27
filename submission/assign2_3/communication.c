
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

/*
*   Implements a REAL modulo function
**/
int mod(int x, int N) {
    // Source: https://stackoverflow.com/questions/11720656/modulo-operation-with-negative-numbers
    return (x % N + N) %N;
}

/*
*   Bcast optimized for ring topology
**/
int MYMPI_Bcast(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm communicator) {
    int i, n_processes, prank, prev_neighbour, next_neighbour, shortest_path, dist1, distL, distR;

    // Get information about process
    MPI_Comm_size(communicator, &n_processes);
    MPI_Comm_rank(communicator, &prank);

    // Calculate the neighbours of the root
    prev_neighbour = mod(root - 1, n_processes);
    next_neighbour = mod(root + 1, n_processes);

    if (prank == root) {
        /* root process */

        // send the message to both neighbours
        MPI_Send(buffer, count, datatype, prev_neighbour, 69, communicator);
        MPI_Send(buffer, count, datatype, next_neighbour, 69, communicator);
    } else {
        /* non-root processes */

        // calculate the distance between root and the current process
        dist1 = abs(prank - root);
        // Determine which direction dist1 points in
        if (prank < root) {
            distL = dist1;
            distR = n_processes - distL;
        } else {
            distR = dist1;
            distL = n_processes - distR;
        }

        // Choose the shortest path
        if (distL < distR) {
            // Listen to the right neighbour
            MPI_Recv(buffer, count, datatype, mod(prank + 1, n_processes), 69, communicator, NULL);
            // Send the message through to the left neighbour
            MPI_Send(buffer, count, datatype, mod(prank - 1, n_processes), 69, communicator);
        } else {
            // Listen to the left neighbour
            MPI_Recv(buffer, count, datatype, mod(prank - 1, n_processes), 69, communicator, NULL);
            // Send the message through to the right neighbour
            MPI_Send(buffer, count, datatype, mod(prank + 1, n_processes), 69, communicator);
        }
    }
}


/*
*   Sends a message to everyone in the network. Works for the circular
*   topology 
**/
int MYMPI_Bcast_simple(void * buffer, int count, MPI_Datatype datatype, int root, MPI_Comm communicator) {
    int i, n_processes, prank;

    // Get information about process
    MPI_Comm_size(communicator, &n_processes);
    MPI_Comm_rank(communicator, &prank);

    if (prank == root) {
        /* The root */
        
        // send the contents of the buffer to each process
        for (i = 0; i < n_processes; i++) {
            if (i != root) {
                MPI_Send(buffer, count, datatype, i, 69, communicator);
            }
        }
    } else {
        /* The non-root: receive into the buffer pointer */
        
        // recieve from the root and store in buffer
        MPI_Recv(buffer, count, datatype, root, 69, communicator, NULL); 
    }
}

int main(int argc, char** argv) {
    MPI_Init(NULL, NULL);
    if (argc != 2) {
        printf("usage: communication <text_to_print>\n");
        MPI_Finalize();
        exit(0);
    }
    
    char buffer[256];
    char *msg = argv[1];
    char *p;
    int prank;
    int root = 2;
    MPI_Comm_rank(MPI_COMM_WORLD, &prank);

    // Give root and other nodes their own buffer
    if (prank == root) {
        p = msg;
    } else {
        p = (char*) &buffer;
    }

    // Send and recieve broadcast
    MYMPI_Bcast(p, 256, MPI_CHAR, root, MPI_COMM_WORLD);

    // Print sent and recieved messages
    printf("process %i received: \"%s\"\n", prank, p);

    MPI_Finalize();
    return 0;
}