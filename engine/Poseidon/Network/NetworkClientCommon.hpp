#pragma once

#include <Poseidon/Network/NetworkImpl.hpp>
#include <Poseidon/Network/NetworkConfig.hpp>

// Shared between networkClient.cpp and networkClientActions.cpp

// Single item of respawn queue (units waiting for respawn)
struct RespawnQueueItem
{
    OLink<Person> person;
    Vector3 position;
    Poseidon::Foundation::Time time;
    bool player; // unit belongs to a human player
};

