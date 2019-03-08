#include "debug.h"
#include "units.hpp"

using namespace std;


// External Variable Declarations ----------------------------------------------

extern World world_terrain;


// Constructor Definitions -----------------------------------------------------

Human::Human(int x, int y, int z) : Unit(x, y, z, "human") {
    layout[{+0, -1, +0}] = COLOUR_GREEN;
    layout[{+0, +0, +0}] = COLOUR_RED;
    layout[{+0, +1, +0}] = COLOUR_ORANGE;
    terrain_height = 2;
    for (; y > 2; y--) {
        if (world_terrain[x][y][z]) {
            terrain_height += y;
            break;
        }
    }
    origin.y = target.y = terrain_height;
}

Human::Human(Coordinate coordinate) :
    Human(coordinate.x, coordinate.y, coordinate.z) {}

Human::Human() : Human(calc_random_coordinate()) {}


// Public Method Definitions ---------------------------------------------------

void Human::ai() {
    switch (state) {
        case SETTLED:
            if (fall_height >= MAP_CLEAR) {
                state = KILLED;
            }
            break;
        case FALLING:
            if (origin.y == terrain_height) {
                state = SETTLED;
            } else {
                ++fall_height;
                --target.y;
                assert_gte(origin.y, 0, "out of bounds");
            }
            break;
        case FLOATING:
            assert_lte(target.y - 2, WORLD_Y, "out of bounds");
            break;
        case KILLED:
            remove();
            return;
    }
    Unit::ai();
}

void Human::render() {
    if (state == FLOATING) {
        Layout next_layout;
        next_layout[{+0, -1, +0}] = layout[{+0, +0, +0}];
        next_layout[{+0, +0, +0}] = layout[{+0, +1, +0}];
        next_layout[{+0, +1, +0}] = layout[{+0, -1, +0}];
        layout.swap(next_layout);
    }
    Unit::render();
}

void Human::shoot() {
    available = false;
    state = KILLED;
    Unit::shoot();
}

void Human::action_lift() {
    ++target.y;
    available = false;
    state = FLOATING;
}

void Human::action_drop() {
    log("%s dropped", as_str.c_str());
    fall_height = 0;
    available = true;
    state = FALLING;
}

void Human::action_capture() {
    log("%s captured", as_str.c_str());
    available = false;
    state = KILLED;
}
