//
// Created by drep on 2022-04-01.
//

#ifndef STIRLING_GAME_DEMO_SERVER_H
#define STIRLING_GAME_DEMO_SERVER_H
#include <dc_application/command_line.h>
#include <dc_application/config.h>
#include <dc_application/defaults.h>
#include <dc_application/environment.h>
#include <dc_application/options.h>
#include <dc_posix/dc_stdlib.h>
#include <dc_posix/dc_string.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
//includes from tutorial
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <sys/select.h>
#include <fcntl.h>
#include <dc_posix/dc_netdb.h>
#include <dc_posix/sys/dc_socket.h>
#include <dc_posix/dc_signal.h>
#include <dc_posix/dc_unistd.h>
#include <time.h>
#include <dc_posix/dc_fcntl.h>

#include "default_config.h"
#include <common.h>
#include "network_util.h"
#include "admin_integration.h"


struct application_settings
{
    struct dc_opt_settings opts;
    struct dc_setting_string *filename;
    struct dc_setting_string *server_ip;
    struct dc_setting_uint16 *server_udp_port;
    struct dc_setting_uint16 *server_tcp_port;
};

void signal_handler(__attribute__ ((unused)) int signnum);

static struct dc_application_settings *create_settings(const struct dc_posix_env *env, struct dc_error *err);
static int destroy_settings(const struct dc_posix_env *env,
                            struct dc_error *err,
                            struct dc_application_settings **psettings);

static void quit_handler(int sig_num);
static int run(const struct dc_posix_env *env, struct dc_error *err, struct dc_application_settings *settings);
static void error_reporter(const struct dc_error *err);
static void trace_reporter(const struct dc_posix_env *env,
                           const char *file_name,
                           const char *function_name,
                           size_t line_number);


static void addToClientList(server_info *serverInfo, int client_tcp_socket);

static void
receive_udp_packet(const struct dc_posix_env *env, struct dc_error *err, server_info *serverInfo, int udp_server_sd);

static void acceptTCPConnection(const struct dc_posix_env *env, struct dc_error *err, server_info *serverInfo, int tcp_server_sd);

//static int create_tcp_server(const struct dc_posix_env *env, struct dc_error *err, const char *hostname, in_port_t port);

static int create_udp_server(const struct dc_posix_env *env, struct dc_error *err, const char *hostname, in_port_t port);

static void
receive_tcp_packet(const struct dc_posix_env *env, struct dc_error *err, server_info *serverInfo, uint16_t client_id);

static void removeFromClientList(server_info *serverInfo, uint16_t client_id);
static bool compare_udp_sockets(struct sockaddr_in *original, struct sockaddr_in *received);

static void send_game_state(server_info *serverInfo, int udp_socket);

static connection_node * get_active_clients(server_info *serverInfo, uint16_t *num_entities);

static volatile sig_atomic_t exit_flag;

#endif //STIRLING_GAME_DEMO_SERVER_H
