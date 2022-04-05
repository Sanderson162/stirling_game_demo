//
// Created by drep on 2022-04-03.
//

#ifndef STIRLING_GAME_DEMO_CHAT_SERVER_H
#define STIRLING_GAME_DEMO_CHAT_SERVER_H
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

// local imports
#include <common.h>


#define CHAT_DEFAULT_HOSTNAME "localhost"


void *run_chat_server(const char *parent_socket_path);

#endif //STIRLING_GAME_DEMO_CHAT_SERVER_H
