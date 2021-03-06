#include <string.h>
#include "debug.h"
#include "exec.h"
#include "graphics.h"
#include "units.hpp"

extern Config config;

int main(int argc, char **argv) {
    // Parse CLI arguments
    for (int i = 1; i < argc; i++) {
        char *arg = argv[i];
        if (!strcmp(arg, "-drawall")) {
            config.display_all_cubes = !config.display_all_cubes;
        } else if (!strcmp(arg, "-fps")) {
            config.show_fps = !config.show_fps;
        } else if (!strcmp(arg, "-full")) {
            config.full_screen = !config.full_screen;
        } else if (!strcmp(arg, "-testworld")) {
            config.test_world = !config.test_world;
        } else {
            puts("usage: a1 [-drawall] [-testworld] [-fps] [-full]");
            exit(1);
        }
    }

    // Initialize game
    log("loading map");
    pgm_init("ground.pgm");
    pgm_set_world_terrain();

    log("adding units");
    unit_init_all();

    log("starting game");
    start_game(&argc, argv);
    return 0;
}
