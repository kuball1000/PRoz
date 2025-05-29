#include "common.h"
#include "consts.h"
#include "student.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include <mutex>
#include <condition_variable>

extern int lamportClock;
extern std::vector<LamportRequest> jamQueue;

void runStudent(int rank) {
    std::atomic<int> jamAvailable(0);
    std::atomic<int> ackCount(0);
    std::mutex mtx;
    std::condition_variable cv;

    std::thread listener([&]() {
        while (true) {
            LamportMessage msg;
            MPI_Status status;
            MPI_Recv(&msg, 2, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
            updateClock(msg.timestamp);

            std::unique_lock<std::mutex> lock(mtx);

            switch (status.MPI_TAG) {
                case MSG_NEW_JAM:
                    jamAvailable++;
                    std::cout << "[Studentka " << rank << "] - otrzymała info o konfiturze - (clock=" << getClock() << ")\n";
                    break;
                case MSG_REL_JAM:
                    jamAvailable--;
                    removeFromQueue(jamQueue, msg.sender);
                    std::cout << "[Studentka " << rank << "] - usunęła z kolejki: " << msg.sender <<" - (clock="<< getClock()<<")\n";
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

            cv.notify_all();
        }
    });

    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1500 + rand() % 3000));

        incrementClock();
        int timestamp = getClock();

        {
            std::lock_guard<std::mutex> lock(mtx);
            addToQueue(jamQueue, timestamp, rank);
        
            std::cout << "[Studentka " << rank << "] - chce konfiturę - (clock=" << timestamp << ")\n";

            LamportMessage req = { timestamp, rank };
            for (int i = NUM_GRANNIES; i < TOTAL_PROCESSES; ++i) {
                if (i == rank) continue;
                MPI_Send(&req, 2, MPI_INT, i, MSG_REQ_JAM, MPI_COMM_WORLD);
            }
        }

        ackCount = 0;

        {
            std::unique_lock<std::mutex> lock(mtx);
            cv.wait(lock, [&]() {
                return ackCount.load() >= NUM_STUDENTS - 1;
            });
        }

        {
            std::unique_lock<std::mutex> lock(mtx);
            cv.wait(lock, [&]() {
                int position = getPositionInQueue(jamQueue, rank);
                return position < jamAvailable && jamAvailable > 0;
            });

            int position = getPositionInQueue(jamQueue, rank);
            std::cout << "[Studentka " << rank << "] - jest na pozycji " << position << " w kolejce" << " - (clock=" << getClock() << ")\n";
        }

        std::cout << "[Studentka " << rank << "] - zjada konfiturę (clock=" << getClock() << ")\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(1500));

        LamportMessage rel = { getClock(), rank };

        for (int i = NUM_GRANNIES; i < TOTAL_PROCESSES; ++i) {
            MPI_Send(&rel, 2, MPI_INT, i, MSG_REL_JAM, MPI_COMM_WORLD); // usunięcie z kolejki
        }

        for (int i = 0; i < NUM_GRANNIES; ++i) {
            MPI_Send(&rel, 2, MPI_INT, i, MSG_REL_JAM, MPI_COMM_WORLD); // zwrot słoika
        }
    }

    listener.join();
}
