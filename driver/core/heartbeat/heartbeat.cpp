#include "heartbeat.hpp"

HEARTBEAT heartbeat_counter = {};

HEARTBEAT get_heartbeat() {
    return heartbeat_counter;
}

void add_heartbeat_counter() {
    heartbeat_counter.counter++;
}