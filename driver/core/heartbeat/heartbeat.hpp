#pragma once

typedef struct {
    unsigned long long counter;
} HEARTBEAT;

extern HEARTBEAT heartbeat_counter;

HEARTBEAT get_heartbeat();
void add_heartbeat_counter();