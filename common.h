#pragma once
#include <vector>
#include <mutex>
#include <mpi.h>

struct LamportMessage {
    int timestamp;
    int sender;
};

struct LamportRequest {
    int timestamp;
    int rank;

    bool operator<(const LamportRequest& other) const {
        return timestamp < other.timestamp || (timestamp == other.timestamp && rank < other.rank);
    }
};

extern int lamportClock;
extern std::mutex clockMutex;
extern std::vector<LamportRequest> jarQueue;
extern std::vector<LamportRequest> jamQueue;

void incrementClock();
void updateClock(int received);
int getClock();

void addToQueue(std::vector<LamportRequest>& queue, int timestamp, int rank);
bool removeFromQueue(std::vector<LamportRequest>& queue, int rank);
bool isFirstInQueue(const std::vector<LamportRequest>& queue, int rank);
void printQueue(const std::vector<LamportRequest>& queue, int rank);
