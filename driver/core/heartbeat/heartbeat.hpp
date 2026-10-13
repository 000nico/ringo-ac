#pragma once

typedef struct {
    unsigned long long counter;
} Heartbeat;

extern Heartbeat heartbeat_counter;

Heartbeat get_heartbeat();
void add_heartbeat_counter();