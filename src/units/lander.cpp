/**
 * lander.cpp
 *
 * Lander class derived from the Unit base class.
 */

#include <algorithm>
#include "debug.h"
#include "units.hpp"

using namespace std;


// External Variable Declarations ----------------------------------------------

extern World world_terrain;
extern World world_units;


// Constructor Definition ------------------------------------------------------

Lander::Lander(int x, int y, int z) : Unit(x, y, z, "lander") {
    layout[ {-2, -1, +0}] = COLOUR_GREEN;
    layout[ {+2, -1, +0}] = COLOUR_GREEN;
    layout[ {+0, -1, -2}] = COLOUR_GREEN;
    layout[ {+0, -1, +2}] = COLOUR_GREEN;
    layout[ {+0, +1, +0}] = COLOUR_GREEN;
    layout[ {-1, +1, +0}] = COLOUR_GREEN;
    layout[ {+1, +1, +0}] = COLOUR_GREEN;
    layout[ {+0, +2, +0}] = COLOUR_YELLOW;
    origin.y = min(origin.y,(int)calc_min_y());
    new_search_path();
}

Lander::Lander(Coordinate coordinate)
    : Lander(coordinate.x, coordinate.y, coordinate.z) {
}


Lander::Lander() : Lander(calc_random_coordinate()) {
}

Lander::~Lander() {
    if (captive) captive->action_drop();
}


// Private Method Definitions --------------------------------------------------

void Lander::new_search_path() {
    log("%s searching elsewhere", as_str.c_str());
    abandon_captive();
    target = calc_random_coordinate(true);  // along edge
    target.y = origin.y;
}

void Lander::set_captive(Human *human) {
    assert_ok(human, "unable to set captive");
    captive = human;
    captive->available = false;
}

void Lander::decide_next() {
    Human *human;
    if (can_exit()) {
        state = EXITED;
    } else if (is_colliding_ground) {
        state = HITTING_GROUND;
    } else if (is_colliding_unit) {
        state = HITTING_UNIT;
    } else if (can_escape()) {
        state = ESCAPING;
    } else if (can_capture()) {
        state = CAPTURING;
    } else if (can_pursue(&human)) {
        state = PURSUING;
        set_captive(human);
    } else {
        state = SEARCHING;
    }
}

// Actions
void Lander::action_search() {
    if (origin.x <= MAP_CLEAR || origin.x >= WORLD_XZ - MAP_CLEAR ||
            origin.z <= MAP_CLEAR || origin.z >= WORLD_XZ - MAP_CLEAR)
        new_search_path();
}

void Lander::action_bounce_ground() {
    origin.y++;
    target.y+=5;
}

void Lander::action_bounce_unit() {
    target.x = WORLD_XZ-target.x;
    target.z = WORLD_XZ-target.z;
    target.y++;
}

void Lander::action_pursue() {
    target.x = captive->origin.x;
    target.z = captive->origin.z;
    target.y = captive->origin.y + MAP_CLEAR;
}

void Lander::action_capture() {
    target.y = captive->origin.y + MAP_CLEAR;
}

void Lander::action_escape() {
    if (cycle % 10 != 0) return;
    ++target.y;
    captive->action_lift();
}

void Lander::action_exit() {
    log("%s escaped with %s", as_str.c_str(), captive->as_str.c_str());
    captive->action_capture();
    abandon_captive();
    delete this;
}

void Lander::action_attack() {
    log("%s attacking player", as_str.c_str());
}

void Lander::action_kill() {
    log("%s killed", as_str.c_str());
    delete this;
}

// Deciders
bool Lander::can_escape() {
    int captive_distance = y_distance(captive);
    return captive && captive_distance > 0 && captive_distance < MAP_CLEAR * 2;
}

bool Lander::can_capture() {
    return y_distance(captive) > 0;
}

bool Lander::can_pursue(Human **human) {
    Coordinate idx1;  // coordinate from
    idx1.x = max(origin.x - LANDER_VISIBILITY, 0);
    idx1.z = max(origin.z - LANDER_VISIBILITY, 0);
    idx1.y = 0;
    Coordinate idx2;  // coordinate to
    idx2.x = min(origin.x + LANDER_VISIBILITY, WORLD_XZ - 1);
    idx2.z = min(origin.z + LANDER_VISIBILITY, WORLD_XZ - 1);
    idx2.y = origin.y;
    Coordinate idx;  // coordinate counter
    for (idx.x = idx1.x; idx.x < idx2.x; idx.x++) {
        for (idx.z = idx1.z; idx.z < idx2.z; idx.z++) {
            for (idx.y = idx1.y; idx.y < idx2.y; idx.y++) {
                if (world_units[idx.x][idx.y][idx.z]) {
                    *human = dynamic_cast<Human *>(find_unit(idx));
                    if (*human && (*human)->available) return true;
                }
            }
        }
    }
    return false;
}

bool Lander::can_shoot_player() {
    return false;
}

bool Lander::can_exit() {
    return captive && WORLD_Y - origin.y < 3;
}


// Public Method Definitions ---------------------------------------------------

void Lander::ai() {
    switch (state) {
        case SEARCHING:
            action_search();
            break;
        case HITTING_UNIT:
            log("%s hitting unit", as_str.c_str());
            action_bounce_unit();
            break;
        case HITTING_GROUND:
            log("%s hitting ground", as_str.c_str());
            action_bounce_ground();
            break;
        case PURSUING:
            action_pursue();
            break;
        case CAPTURING:
            action_capture();
            break;
        case ESCAPING:
            action_escape();
            break;
        case EXITED:
            action_exit();
            return;
        case ATTACKING:
            action_attack();
            break;
        case KILLED:
            action_kill();
            return;
    }
    decide_next();
    Unit::ai();
}

void Lander::render() {
    if (cycle % 2) {
        layout[ {+0, +0, +1}] = COLOUR_GREEN;
        layout[ {+0, +0, -1}] = COLOUR_GREEN;
        layout[ {-1, +0, +0}] = COLOUR_YELLOW;
        layout[ {+1, +0, +0}] = COLOUR_YELLOW;
    } else {
        layout[ {+0, +0, +1}] = COLOUR_YELLOW;
        layout[ {+0, +0, -1}] = COLOUR_YELLOW;
        layout[ {-1, +0, +0}] = COLOUR_GREEN;
        layout[ {+1, +0, +0}] = COLOUR_GREEN;
    }
    Unit::render();
}

void Lander::abandon_captive() {
    captive = nullptr;
}
