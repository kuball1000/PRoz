#include "common.h"
#include "consts.h"
#include "granny.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include <condition_variable>

void runGranny(int rank) {
    std::atomic<int> jarAcks(0);
    std::atomic<int> availableJars(NUM_JARS);
    std::mutex mtx;
    std::condition_variable cv;

    std::thread listener([&]() {
        while (true) {
            LamportMessage recvMsg;
            MPI_Status status;
            MPI_Recv(&recvMsg, 2, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
            updateClock(recvMsg.timestamp);

            std::unique_lock<std::mutex> lock(mtx);

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
                    std::cout << "[Babcia " << rank << "] - widzi, że Babcia " << recvMsg.sender << " skończyła robić konfiturę - (clock=" << getClock() << ")\n";
                    break;
                case MSG_REL_JAM:
                    availableJars++;
                    std::cout << "[Babcia " << rank << "] - odzyskała słoik od Studentki " << recvMsg.sender << "- (clock=" << getClock() << ")\n";
                    break;
            }

            cv.notify_all();
        }
    });

    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000 + rand() % 3000));

        incrementClock();
        int timestamp = getClock();
        addToQueue(jarQueue, timestamp, rank);
        jarAcks = 0;

        {
            std::unique_lock<std::mutex> lock(mtx);
            LamportMessage msg = { timestamp, rank };
            for (int i = 0; i < NUM_GRANNIES; ++i) {
                if (i == rank) continue;
                MPI_Send(&msg, 2, MPI_INT, i, MSG_REQ_JAR, MPI_COMM_WORLD);
            }
        }   

        // Czekaj aż przyjdą wszystkie ACK-i
        {
            std::unique_lock<std::mutex> lock(mtx);
            cv.wait(lock, [&]() {
                return jarAcks.load() >= NUM_GRANNIES - 1;
            });
        }

        std::cout << "[Babcia " << rank << "] - kolejka: ";
        printQueue(jarQueue, rank);
        std::cout << " - (clock=" << getClock() << ")\n";
        std::cout << "[Babcia " << rank << "] - dostępne słoiki: " << availableJars.load() << " - (clock=" << getClock() << ")\n";

        {
            std::unique_lock<std::mutex> lock(mtx);
            cv.wait(lock, [&]() {
                int pos = getPositionInQueue(jarQueue, rank);
                return pos < availableJars.load() && availableJars.load() > 0;
            });
        }

        std::cout << "[Babcia " << rank << "] - robi konfiturę - (clock=" << getClock() << ")\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(5000));

        LamportMessage relMsg = { getClock(), rank };
        for (int i = 0; i < NUM_GRANNIES; ++i) {
            MPI_Send(&relMsg, 2, MPI_INT, i, MSG_REL_JAR, MPI_COMM_WORLD);
        }

        for (int i = NUM_GRANNIES; i < TOTAL_PROCESSES; ++i) {
            MPI_Send(&relMsg, 2, MPI_INT, i, MSG_NEW_JAM, MPI_COMM_WORLD);
        }
    }

    listener.join();
}
