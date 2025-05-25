// granny.cpp
#include "common.h"
#include "consts.h"
#include "granny.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>

void runGranny(int rank) {
    std::atomic<int> jarAcks(0);
    std::atomic<int> availableJars(NUM_JARS);
    int jamCount = 0;

    std::thread listener([&]() {
        while (true) {
            LamportMessage recvMsg;
            MPI_Status status;
            MPI_Recv(&recvMsg, 2, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
            updateClock(recvMsg.timestamp);

            switch (status.MPI_TAG) {
                case MSG_ACK_JAR:
                    jarAcks++;
                    break;
                case MSG_REQ_JAR:
                    addToQueue(jarQueue, recvMsg.timestamp, recvMsg.sender);
                    {
                        LamportMessage ack = { getClock(), rank };
                        MPI_Send(&ack, 2, MPI_INT, recvMsg.sender, MSG_ACK_JAR, MPI_COMM_WORLD);
                    }
                    break;
                case MSG_REL_JAR:
                    availableJars--;
                    removeFromQueue(jarQueue, recvMsg.sender);
                    std::cout << "[Babcia " << rank << "] widzi, że Babcia " << recvMsg.sender << " skończyła robić konfiturę\n";
                    break;
                case MSG_REL_JAM:
                    availableJars++;
                    std::cout << "[Babcia " << rank << "] odzyskała słoik od Studentki " << recvMsg.sender << "\n";
                    break;
            }
        }
    });

    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000 + rand() % 3000));

        incrementClock();
        int timestamp = getClock();
        addToQueue(jarQueue, timestamp, rank);
        jarAcks = 0;

        LamportMessage msg = { timestamp, rank };
        for (int i = 0; i < NUM_GRANNIES; ++i) {
            if (i == rank) continue;
            MPI_Send(&msg, 2, MPI_INT, i, MSG_REQ_JAR, MPI_COMM_WORLD);
        }

        while (jarAcks.load() < NUM_GRANNIES - 1) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        std::cout << "[Babcia " << rank << "] kolejka: ";
        printQueue(jarQueue, rank);
        std::cout << "[Babcia " << rank << "] dostępne słoiki: " << availableJars.load() << "\n";

        int position = getPositionInQueue(jarQueue, rank);
        if (position != -1) {
            std::cout << "[Babcia " << rank << "] jest na pozycji " << position << " w kolejce\n";
        } else {
            std::cout << "[Babcia " << rank << "] nie ma w kolejce\n";
        }

        while (!(position < availableJars.load() && availableJars.load() > 0)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        std::cout << "[Babcia " << rank << "] robi konfiturę (clock=" << getClock() << ")\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(2000));
        jamCount++;
        // availableJars--;

        LamportMessage relMsg = { getClock(), rank };
        for (int i = 0; i < NUM_GRANNIES; ++i) {
            // if (i == rank) continue;
            MPI_Send(&relMsg, 2, MPI_INT, i, MSG_REL_JAR, MPI_COMM_WORLD);
        }
        // removeFromQueue(jarQueue, rank);

        for (int i = NUM_GRANNIES; i < TOTAL_PROCESSES; ++i) {
            MPI_Send(&relMsg, 2, MPI_INT, i, MSG_NEW_JAM, MPI_COMM_WORLD);
        }
    }

    listener.join();
}
