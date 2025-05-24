#pragma once

#define NUM_GRANNIES 2
#define NUM_STUDENTS 2
#define TOTAL_PROCESSES (NUM_GRANNIES + NUM_STUDENTS)
#define NUM_JARS 3

enum MessageType {
    MSG_REQ_JAR,
    MSG_ACK_JAR,
    MSG_REL_JAR,
    MSG_REQ_JAM,
    MSG_ACK_JAM,
    MSG_REL_JAM,
    MSG_NEW_JAM
};
