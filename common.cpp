#include "common.h"
#include <algorithm>
#include <iostream>

int lamportClock = 0;
std::mutex clockMutex;
std::vector<LamportRequest> jarQueue;
std::vector<LamportRequest> jamQueue;

void incrementClock() {
    std::lock_guard<std::mutex> lock(clockMutex);
    lamportClock++;
}

void updateClock(int received) {
    std::lock_guard<std::mutex> lock(clockMutex);
    lamportClock = std::max(lamportClock, received) + 1;
}

int getClock() {
    std::lock_guard<std::mutex> lock(clockMutex);
    return lamportClock;
}

void addToQueue(std::vector<LamportRequest>& queue, int timestamp, int rank) {
    queue.push_back({timestamp, rank});
    std::sort(queue.begin(), queue.end());
}

bool removeFromQueue(std::vector<LamportRequest>& queue, int rank) {
    auto it = std::remove_if(queue.begin(), queue.end(),
        [rank](const LamportRequest& r) { return r.rank == rank; });
    bool removed = it != queue.end();
    queue.erase(it, queue.end());
    return removed;
}


bool isFirstInQueue(const std::vector<LamportRequest>& queue, int rank) {
    return !queue.empty() && queue.front().rank == rank;
}

void printQueue(const std::vector<LamportRequest>& queue, int rank) {
    std::cout << "[DEBUG " << rank << "] kolejka = ";
    for (const auto& r : queue) {
        std::cout << "(" << r.timestamp << "," << r.rank << ") ";
    }
    std::cout << "\n";
}
