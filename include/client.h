//
// Created by drep on 2022-04-01.
//

#ifndef STIRLING_GAME_DEMO_CLIENT_H
#define STIRLING_GAME_DEMO_CLIENT_H

#include <dc_application/command_line.h>
#include <dc_application/config.h>
#include <dc_application/defaults.h>
#include <dc_application/environment.h>
#include <dc_application/options.h>
#include <dc_posix/dc_stdlib.h>
#include <dc_posix/dc_string.h>
#include <dc_posix/dc_signal.h>
#include <dc_posix/sys/dc_socket.h>
#include <dc_posix/dc_unistd.h>

#include <arpa/inet.h>
#include <errno.h>
#include <getopt.h>
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/un.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>

//local imports
#include <common.h>
#include "network_util.h"
#include "types.h"
#include "default_config.h"

typedef struct sockaddr_in sockaddr_in;


struct application_settings
{
    struct dc_opt_settings opts;
    struct dc_setting_string *start_time;
    struct dc_setting_string *server_ip;
    struct dc_setting_uint16 *server_udp_port;
    struct dc_setting_uint16 *server_tcp_port;
    struct dc_setting_uint16 *num_packets;
    struct dc_setting_uint16 *packet_size;
    struct dc_setting_uint16 *packet_delay;
};

void signal_handler(__attribute__ ((unused)) int signnum);

static volatile sig_atomic_t exit_flag;

static struct dc_application_settings *create_settings(const struct dc_posix_env *env, struct dc_error *err);
static int destroy_settings(const struct dc_posix_env *env,
                            struct dc_error *err,
                            struct dc_application_settings **psettings);
static int run(const struct dc_posix_env *env, struct dc_error *err, struct dc_application_settings *settings);
static void error_reporter(const struct dc_error *err);
static void trace_reporter(const struct dc_posix_env *env,
                           const char *file_name,
                           const char *function_name,
                           size_t line_number);


size_t get_time_difference(int future_hour, int future_minute, int current_hour, int current_min, int current_seconds);

static void send_game_state(client_info *clientInfo, int udp_socket);

static void receive_udp_packet(const struct dc_posix_env *env, struct dc_error *err, client_info *clientInfo);

static void free_client_entities_list(client_node **head);

static void fill_entities_list(client_node **entities_list, const uint8_t *entity_buffer, uint16_t num_entities);

static void update_player_position(client_info *clientInfo);

void * ncurses_thread(client_info *clientInfo);

void move_player(client *client_entity, int direction_x, int direction_y);

void draw_game(client_info *clientInfo);
static void free_bullet_list(bullet_node **head);
static void fill_bullet_list(bullet_node **bullet_list, const uint8_t *entity_buffer, uint16_t num_bullets);
#endif //STIRLING_GAME_DEMO_CLIENT_H
