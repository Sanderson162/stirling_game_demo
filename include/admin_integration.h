//
// Created by drep on 2022-04-01.
//

#ifndef STIRLING_GAME_DEMO_ADMIN_H
#define STIRLING_GAME_DEMO_ADMIN_H

#include <stdint.h>

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

#include "common.h";


#define ADMIN_HEADER_SIZE 6


// arbritrary number, enough for the whole class to be connected
#define MAX_ADMIN_CLIENTS 15

#define ADMIN_PROTOCOL_VERSION 1


typedef struct {
    uint8_t version;
    uint8_t command;
    uint16_t target_client_id;
    uint16_t message_length;
    uint8_t * message;
} admin_client_packet;


enum ADMIN_COMMANDS {
    STOP,
    COUNT,
    KICK,
    WARN,
    NOTICE
};

typedef struct {
    uint16_t id;
    int tcp_socket;
} admin_client;

typedef struct {
    int admin_server_socket;
    admin_client * adminClientList[MAX_ADMIN_CLIENTS];
} admin_server_info;

void
admin_readPacketFromSocket(const struct dc_posix_env *env, struct dc_error *err, admin_server_info *adminServerInfo,
                           uint16_t admin_id, admin_client_packet *adminClientPacket, int admin_socket);

void admin_receiveTcpPacket(const struct dc_posix_env *env, struct dc_error *err, admin_server_info *adminServerInfo, server_info *serverInfo, uint16_t admin_id);

void admin_acceptTCPConnection(const struct dc_posix_env *env, struct dc_error *err, admin_server_info *adminServerInfo);

void admin_addToClientList(const struct dc_posix_env *env, struct dc_error *err, admin_server_info *adminServerInfo, int admin_tcp_socket);
void admin_removeFromClientList(const struct dc_posix_env *env, struct dc_error *err, admin_server_info *adminServerInfo, uint16_t admin_id);
#endif //STIRLING_GAME_DEMO_ADMIN_H
