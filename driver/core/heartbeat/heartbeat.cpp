#include "heartbeat.hpp"

Heartbeat heartbeat_counter = {};

Heartbeat get_heartbeat() {
    return heartbeat_counter;
}

void add_heartbeat_counter() {
    heartbeat_counter.counter++;
}