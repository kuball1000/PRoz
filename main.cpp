#include "consts.h"
#include "granny.h"
#include "student.h"
#include <mpi.h>
#include <iostream>
#include <cstdlib>
#include <ctime>

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (size != TOTAL_PROCESSES) {
        if (rank == 0) {
            std::cerr << "Error: This program requires exactly " << TOTAL_PROCESSES << " processes.\n";
        }
        MPI_Finalize();
        return 1;
    }

    std::srand(std::time(nullptr) + rank);

    if (rank < NUM_GRANNIES) {
        std::cout << "Proces " << rank << " to babcia\n";
        runGranny(rank);
    } else {
        std::cout << "Proces " << rank << " to studentka\n";
        runStudent(rank);
    }

    MPI_Finalize();
    return 0;
}
