#include "common.h"
#include "consts.h"
#include "student.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>

extern int lamportClock;
extern std::vector<LamportRequest> jamQueue;

void runStudent(int rank) {
    int jamAcks = 0;
    std::atomic<int> jamAvailable(0);

    std::atomic<int> ackCount(0);

std::thread listener([&]() {
    while (true) {
        LamportMessage msg;
        MPI_Status status;
        MPI_Recv(&msg, 2, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        updateClock(msg.timestamp);

        switch (status.MPI_TAG) {
            case MSG_NEW_JAM:
                jamAvailable++;
                std::cout << "[Studentka " << rank << "] otrzymała info o konfiturze (clock=" << getClock() << ")\n";
                break;
            case MSG_REL_JAM:
                jamAvailable--;
                removeFromQueue(jamQueue, msg.sender);
                std::cout << "[Studentka " << rank << "] usunęła z kolejki: " << msg.sender << "\n";
                break;
            case MSG_REQ_JAM:
                addToQueue(jamQueue, msg.timestamp, msg.sender);
                {
                    LamportMessage ack = { getClock(), rank };
                    MPI_Send(&ack, 2, MPI_INT, msg.sender, MSG_ACK_JAM, MPI_COMM_WORLD);
                }
                break;
            case MSG_ACK_JAM:
                ackCount++;
                break;
        }
    }
});


    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1500 + rand() % 3000));

        if (jamAvailable <= 0) {
            std::cout << "[Studentka " << rank << "] czeka (clock=" << getClock() << ")\n";
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            continue;
        }

        incrementClock();
        int timestamp = getClock();
        addToQueue(jamQueue, timestamp, rank);
        std::cout << "[Studentka " << rank << "] chce konfiturę (clock=" << timestamp << ")\n";
        jamAcks = 0;

        LamportMessage req = { timestamp, rank };
        for (int i = NUM_GRANNIES; i < TOTAL_PROCESSES; ++i) {
            if (i == rank) continue;
            MPI_Send(&req, 2, MPI_INT, i, MSG_REQ_JAM, MPI_COMM_WORLD);
        }

        ackCount = 0;

    while (ackCount.load() < NUM_STUDENTS - 1) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

        while (true) {
            int position = getPositionInQueue(jamQueue, rank);
            if (position != -1) {
                std::cout << "[Studentka " << rank << "] jest na pozycji " << position << " w kolejce\n";
            } else {
                std::cout << "[Studentka  " << rank << "] nie ma w kolejce\n";
            }
            if (position < jamAvailable && jamAvailable > 0) break;
            std::cout << "[Studentka " << rank << "] NIE wchodzi – first=" << position << ", jamAvailable=" << jamAvailable << "\n";
            printQueue(jamQueue, rank);
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }

        std::cout << "[Studentka " << rank << "] zjada konfiturę (clock=" << getClock() << ")\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(1500));
        // jamAvailable--;

        LamportMessage rel = { getClock(), rank };

        for (int i = NUM_GRANNIES; i < TOTAL_PROCESSES; ++i) {
            // if (i == rank) continue;
            MPI_Send(&rel, 2, MPI_INT, i, MSG_REL_JAM, MPI_COMM_WORLD);//usuniecia z kolejki
        }

        for (int i = 0; i < NUM_GRANNIES; ++i) {
            MPI_Send(&rel, 2, MPI_INT, i, MSG_REL_JAM, MPI_COMM_WORLD); //zwrot sloika
        }



        // removeFromQueue(jamQueue, rank);
    }

    listener.join();
}