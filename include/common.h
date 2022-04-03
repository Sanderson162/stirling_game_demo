#ifndef TEMPLATE_COMMON_H
#define TEMPLATE_COMMON_H

/*
 * This file is part of dc_dump.
 *
 *  dc_dump is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Foobar is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with dc_dump.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <bits/stdint-uintn.h>
#include <netinet/in.h>
#include <stdbool.h>

#define DEFAULT_TCP_PORT "7521"
#define DEFAULT_UDP_PORT "4981"

#define DEFAULT_TCP_PORT_ADMIN_SERVER 8521

#define DEFAULT_HOSTNAME "127.0.0.1"
#define MAX_TCP_CLIENTS 100
#define MAX_CLIENTS 1000
typedef struct {
    bool move_up;
    bool move_down;
    bool move_left;
    bool move_right;

    bool shoot_up;
    bool shoot_down;
    bool shoot_left;
    bool shoot_right;
} client_input_state;

typedef struct {
    uint16_t client_id;
    uint16_t position_x;
    uint16_t position_y;
    client_input_state inputState;
} client;

typedef struct {
    uint16_t shooters_id;
    uint16_t position_x;
    uint16_t position_y;
    short direction_x;
    short direction_y;
} bullet;

typedef struct bullet_node {
    bullet * bullet;
    struct bullet_node * next;
} bullet_node;


typedef struct {
    uint16_t client_id;
    uint64_t last_packet_no;
    int tcp_socket;
    struct sockaddr_in * udp_address;
    socklen_t udp_adr_len;
    bool has_client_entity;
    client * client_entity;
} connection;

typedef struct connection_node {
    uint16_t client_id;
    struct connection_node * next;
} connection_node;

typedef struct client_node {
    client * client_entity;
    struct client_node * next;
} client_node;

typedef struct {
    bullet_node * bulletList;
    connection * connections[MAX_CLIENTS];
    uint64_t last_packet_no;
} server_info;

typedef struct {
    client_node * client_entities;
    uint16_t client_id;
    uint64_t last_packet_sent;
    uint64_t last_packet_received;
    int tcp_socket;
    int udp_socket;
    struct sockaddr_in * udp_address;
    socklen_t udp_adr_len;
    bool has_client_entity;
    client * client_entity;
    client_input_state clientInputState;
    bullet_node * bulletList;
} client_info;


#define EXIT_KEY '0'
#define MOVE_UP 'w'
#define MOVE_LEFT 'a'
#define MOVE_DOWN 's'
#define MOVE_RIGHT 'd'
#define PLAYER_ICON '0'

#endif // TEMPLATE_COMMON_H
