//
// Created by drep on 2022-04-01.
//

#ifndef STIRLING_GAME_DEMO_NETWORK_UTIL_H
#define STIRLING_GAME_DEMO_NETWORK_UTIL_H

#include <arpa/inet.h>
#include <assert.h>
#include <dc_application/command_line.h>
#include <dc_application/config.h>
#include <dc_application/options.h>
#include <dc_network/common.h>
#include <dc_network/options.h>
#include <dc_network/server.h>
#include <dc_posix/sys/dc_msg.h>
#include <dc_posix/dc_netdb.h>
#include <dc_posix/dc_signal.h>
#include <dc_posix/dc_stdlib.h>
#include <dc_posix/dc_string.h>
#include <dc_posix/dc_unistd.h>
#include <dc_posix/sys/dc_socket.h>
#include <dc_posix/sys/dc_msg.h>
#include <dc_util/dump.h>
#include <dc_util/streams.h>
#include <dc_util/types.h>
#include <getopt.h>
#include <inttypes.h>
#include <sys/fcntl.h>
#include <signal.h>
#include <sys/select.h>
#include <unistd.h>

int create_tcp_server(const struct dc_posix_env *env, struct dc_error *err, const char *hostname, in_port_t port, const char * ip_version);
int connect_to_tcp_server(const struct dc_posix_env *env, struct dc_error *err, const char *hostname, in_port_t port, const char * ip_version);


#endif //STIRLING_GAME_DEMO_NETWORK_UTIL_H
