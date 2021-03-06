#include <math.h>
#include "debug.h"
#include "exec.h"
#include "graphics.h"

extern Config config;
extern Laser lasers[];
extern Position player_pos;
extern View view;
extern World world_terrain;

static Coordinate pos_to_coord(Position pos) {
    return (Coordinate) {
        (int) pos.x, (int) pos.y, (int) pos.z
    };
}

static bool _has_collided(Coordinate coord) {
    int x, y, z;
    for (int x_offset = -1; x_offset < 2; x_offset++) {
        for (int y_offset = -1; y_offset < 2; y_offset++) {
            for (int z_offset = -1; z_offset < 2; z_offset++) {
                x = (coord.x * -1 + x_offset);
                y = (coord.y * -1 + y_offset);
                z = (coord.z * -1 + z_offset);
                // check if out of bounds
                if (x <= 0 || x >= WORLD_XZ) return true;
                if (z <= 0 || z >= WORLD_XZ) return true;
                if (y <= 0 || y >= WORLD_Y) return true;
                // check for cube
                if (world_terrain[x][y][z] != COLOUR_NONE) return true;
            }
        }
    }
    return false;
}

static void _calc_player_move(Direction direction) {
    Position player_pos_next = player_pos;
    static float accel_x = 0.0f;
    static float accel_y = 0.0f;
    static float accel_z = 0.0f;
    if (direction != DIRECTION_COAST) {
        float rot_x = (view.cam_x / 180.0f * PI);
        float rot_y = (view.cam_y / 180.0f * PI);
        // calculate coordinate
        switch (direction) {
            default:
                break;
            case DIRECTION_FORWARD:
                player_pos_next.x = player_pos.x - sinf(rot_y);
                if (!config.fly_control)
                    player_pos_next.y = player_pos.y + sinf(rot_x);
                player_pos_next.z = player_pos.z + cosf(rot_y);
                break;
            case DIRECTION_BACK:
                player_pos_next.x = player_pos.x + sinf(rot_y);
                if (!config.fly_control)
                    player_pos_next.y = player_pos.y - sinf(rot_x);
                player_pos_next.z = player_pos.z - cosf(rot_y);
                break;
            case DIRECTION_LEFT:
                player_pos_next.x = player_pos.x + cosf(rot_y);
                player_pos_next.z = player_pos.z + sinf(rot_y);
                break;
            case DIRECTION_RIGHT:
                player_pos_next.x = player_pos.x - cosf(rot_y);
                player_pos_next.z = player_pos.z - sinf(rot_y);
                break;
        }
        // recalculate acceleration velocity
        accel_x += (player_pos_next.x - player_pos.x) / 4;
        accel_y += (player_pos_next.y - player_pos.y) / 4;
        accel_z += (player_pos_next.z - player_pos.z) / 4;
    }
    // apply acceleration velocity
    player_pos_next.x += accel_x;
    player_pos_next.y += accel_y;
    player_pos_next.z += accel_z;
    if (_has_collided(pos_to_coord(player_pos_next))) {
        // stop acceleration
        accel_x = 0;
        accel_y = 0;
        accel_z = 0;
        return;
    }
    // decay acceleration
    float decay = config.traction ? 2 : 1.025f;
    accel_x /= decay;
    accel_y /= decay;
    accel_z /= decay;
    // idle_update position if changed
    player_pos = player_pos_next;
}

void glut_hook_default__draw_2d() {
    // note: layers overlay in the reverse order
    map_player_layer();  // e.g. player is drawn above terrain
    if (lasers[0].active) map_laser_layer();  // same with laser, etc...
    map_npc_layer();
    map_outline_layer();
    map_terrain_layer();
}

void glut_hook_default__idle_update() {
    static int laser_base = 0;
    static int timer_base = 0;
    static int frame = 0;
    // calculate time delta
    int time = glutGet(GLUT_ELAPSED_TIME);
    frame++;
    bool next_tick = time - timer_base > 100 / GAME_SPEED;
    // log profiling information
    if (next_tick && config.show_fps) log_fps(frame, time, timer_base);
    // reset lasers[0] cooldown
    bool laser_cooldown = time - laser_base > 350;
    if (!lasers[0].active) {
        laser_base = time;
    } else if (laser_cooldown) {
        lasers[0].active = false;
        laser_base = time;
    }
    // apply player movement
    _calc_player_move(DIRECTION_COAST);
    if (!next_tick && !config.timer_unlock) return;
    // reset time base
    timer_base = time;
    frame = 0;
    // trigger unit movement
    unit_cycle();
}

void glut_hook_default__keyboard(unsigned char key, int x, int y) {
    Direction direction = DIRECTION_COAST;
    switch (key) {
        case 'q':
        case 27:
        log("exiting");
            unit_rm_all();
            glutDestroyWindow(glutGetWindow());
            exit(0);
        case 'w':
            direction = DIRECTION_FORWARD;
            break;
        case 's':
            direction = DIRECTION_BACK;
            break;
        case 'a':
            direction = DIRECTION_LEFT;
            break;
        case 'd':
            direction = DIRECTION_RIGHT;
            break;
        case 'm':
            map_mode_toggle();
            break;
        case 'r':
            unit_reset_all();
            puts("resetting units");
            break;
        case 'f':
            config.fly_control = !config.fly_control;
            printf(
                "fly controls set to %s\n",
                config.fly_control ? "ON" : "OFF"
            );
            if (config.fly_control) {
                player_pos.y = -1 * WORLD_Y + MAP_CLEAR;
            }
            break;
        case 'o':
            config.overhead_view = !config.overhead_view;
            printf(
                "overhead view set to %s\n",
                config.fly_control ? "ON" : "OFF"
            );
            if (config.overhead_view) {
                player_pos.y = -1 * WORLD_Y + MAP_CLEAR * 2;
            }
            break;

        case 't':
            config.traction = !config.traction;
            printf(
                "traction set to %s\n",
                config.traction ? "ON" : "OFF"
            );
            break;
        case 'u':
            config.timer_unlock = !config.timer_unlock;
            printf(
                "timer unlock set to %s\n",
                config.fly_control ? "ON" : "OFF"
            );
            break;
        case 'p':
            config.pause_units = !config.pause_units;
            printf(
                "pause units set to %s\n",
                config.fly_control ? "ON" : "OFF"
            );
            break;
        case ' ':
            lasers[0].active = true;  // in class prof. said to activate w/ space
            break;
        default:
            break;
    }
    _calc_player_move(direction);  // applies drifting
}

void glut_hook_default__motion(int x, int y) {
    // render camera position
    view.cam_x += y - view.old_y;
    view.cam_y += x - view.old_x;
    view.old_x = x;
    view.old_y = y;
}

void glut_hook_default__mouse(int button, int state, int x, int y) {
    if (button != 0 || state != 0) return;
    lasers[0].active = true;  // spec says to use mouse so that's used too
}

void glut_hook_default__passive_motion(int x, int y) {
    // render camera position
    view.cam_x += y - view.old_y;
    view.cam_y += x - view.old_x;
    view.old_x = x;
    view.old_y = y;
}

void glut_hook_default__reshape(int w, int h) {
    glViewport(0, 0, (GLsizei) w, (GLsizei) h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0, (GLfloat) w / (GLfloat) h, 0.1, WORLD_XZ * 4);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    config.screen_width = w;
    config.screen_height = h;
    map_pos_update();
}
