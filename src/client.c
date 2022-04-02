
#include "client.h"

int main(int argc, char *argv[])
{
    dc_posix_tracer tracer;
    dc_error_reporter reporter;
    struct dc_posix_env env;
    struct dc_error err;
    struct dc_application_info *info;
    int ret_val;

    reporter = error_reporter;
    tracer = trace_reporter;
    tracer = NULL;
    dc_error_init(&err, reporter);
    dc_posix_env_init(&env, tracer);
    info = dc_application_info_create(&env, &err, "Game Client");

    struct sigaction sa;
    sa.sa_handler = &signal_handler;
    sa.sa_flags = 0;
    dc_sigaction(&env, &err, SIGINT, &sa, NULL);
    dc_sigaction(&env, &err, SIGTERM, &sa, NULL);


    ret_val = dc_application_run(&env, &err, info, create_settings, destroy_settings, run, dc_default_create_lifecycle, dc_default_destroy_lifecycle, NULL, argc, argv);
    dc_application_info_destroy(&env, &info);
    dc_error_reset(&err);

    return ret_val;
}

static struct dc_application_settings *create_settings(const struct dc_posix_env *env, struct dc_error *err)
{
    struct application_settings *settings;

    DC_TRACE(env);
    settings = dc_malloc(env, err, sizeof(struct application_settings));

    if(settings == NULL)
    {
        return NULL;
    }

    settings->opts.parent.config_path = dc_setting_path_create(env, err);
    settings->start_time = dc_setting_string_create(env, err);
    settings->server_ip = dc_setting_string_create(env, err);
    settings->server_udp_port = dc_setting_uint16_create(env, err);
    settings->server_tcp_port = dc_setting_uint16_create(env, err);
    settings->num_packets = dc_setting_uint16_create(env, err);
    settings->packet_size = dc_setting_uint16_create(env, err);
    settings->packet_delay = dc_setting_uint16_create(env, err);
    struct options opts[] = {
            {(struct dc_setting *)settings->opts.parent.config_path,
                    dc_options_set_path,
                    "config",
                    required_argument,
                    'c',
                    "CONFIG",
                    dc_string_from_string,
                    NULL,
                    dc_string_from_config,
                    NULL},
            {(struct dc_setting *)settings->start_time,
                    dc_options_set_string,
                    "start_time",
                    required_argument,
                    's',
                    "START_TIME",
                    dc_string_from_string,
                    "start_time",
                    dc_string_from_config,
                    NULL},
            {(struct dc_setting *)settings->server_ip,
                    dc_options_set_string,
                    "server_ip",
                    required_argument,
                    'i',
                    "SERVER_IP",
                    dc_string_from_string,
                    "server_ip",
                    dc_string_from_config,
                    DEFAULT_HOSTNAME},
            {(struct dc_setting *)settings->server_udp_port,
                    dc_options_set_uint16,
                    "server_udp_port",
                    required_argument,
                    'u',
                    "SERVER_UDP_PORT",
                    dc_uint16_from_string,
                    "server_udp_port",
                    dc_uint16_from_config,
                    dc_uint16_from_string(env, err, DEFAULT_UDP_PORT)},
                    {(struct dc_setting *)settings->server_tcp_port,
                    dc_options_set_uint16,
                    "server_tcp_port",
                    required_argument,
                    't',
                    "SERVER_TCP_PORT",
                    dc_uint16_from_string,
                    "server_tcp_port",
                    dc_uint16_from_config,
                    dc_uint16_from_string(env, err, DEFAULT_TCP_PORT)},
            {(struct dc_setting *)settings->num_packets,
                    dc_options_set_uint16,
                    "num_packets",
                    required_argument,
                    'n',
                    "NUM_PACKETS",
                    dc_uint16_from_string,
                    "num_packets",
                    dc_uint16_from_config,
                    dc_uint16_from_string(env, err, "100")},
            {(struct dc_setting *)settings->packet_size,
                    dc_options_set_uint16,
                    "packet_size",
                    required_argument,
                    's',
                    "PACKET_SIZE",
                    dc_uint16_from_string,
                    "packet_size",
                    dc_uint16_from_config,
                    dc_uint16_from_string(env, err, "100")},
            {(struct dc_setting *)settings->packet_delay,
                    dc_options_set_uint16,
                    "packet_delay",
                    required_argument,
                    'd',
                    "PACKET_DELAY",
                    dc_uint16_from_string,
                    "packet_delay",
                    dc_uint16_from_config,
                    dc_uint16_from_string(env, err, "1")}

    };

    // note the trick here - we use calloc and add 1 to ensure the last line is all 0/NULL
    settings->opts.opts_count = (sizeof(opts) / sizeof(struct options)) + 1;
    settings->opts.opts_size = sizeof(struct options);
    settings->opts.opts = dc_calloc(env, err, settings->opts.opts_count, settings->opts.opts_size);
    dc_memcpy(env, settings->opts.opts, opts, sizeof(opts));
    settings->opts.flags = "m:";
    settings->opts.env_prefix = "DC_EXAMPLE_";

    return (struct dc_application_settings *)settings;
}

static int destroy_settings(const struct dc_posix_env *env,
                            __attribute__((unused)) struct dc_error *err,
                            struct dc_application_settings **psettings)
{
    struct application_settings *app_settings;

    DC_TRACE(env);
    app_settings = (struct application_settings *)*psettings;
    dc_setting_string_destroy(env, &app_settings->start_time);
    dc_setting_string_destroy(env, &app_settings->server_ip);
    dc_free(env, app_settings->opts.opts, app_settings->opts.opts_count);
    dc_free(env, *psettings, sizeof(struct application_settings));

    if(env->null_free)
    {
        *psettings = NULL;
    }

    return 0;
}


static int run(const struct dc_posix_env *env, struct dc_error *err, struct dc_application_settings *settings)
{
    struct application_settings *app_settings;

    DC_TRACE(env);

    app_settings = (struct application_settings *)settings;

    //TCP
    int tcp_server_socket = connect_to_tcp_server(env, err, dc_setting_string_get(env, app_settings->server_ip), dc_setting_uint16_get(env, app_settings->server_tcp_port), DEFAULT_IP_VERSION);
    if (dc_error_has_error(err) || tcp_server_socket <= 0) {
        printf("could not connect to TCP socket\n");
        exit(1);
    }

    // UDP:
    struct sockaddr_in udp_server_addr;
    socklen_t server_struct_length = sizeof(struct sockaddr_in);
    const int udp_server_sd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    if(udp_server_sd < 0){
        printf("Error while creating socket\n");
        return -1;
    }
    printf("Socket created successfully\n");
    // Set port and IP:
    udp_server_addr.sin_family = AF_INET;
    udp_server_addr.sin_port = htons(dc_setting_uint16_get(env, app_settings->server_udp_port));
    udp_server_addr.sin_addr.s_addr = inet_addr(dc_setting_string_get(env, app_settings->server_ip));


    // Clean buffers:
    char server_message[2000], client_message[2000];
    memset(server_message, '\0', sizeof(server_message));
    memset(client_message, '\0', sizeof(client_message));
    bool exitFlag = false;


    ssize_t count = dc_read(env, err, tcp_server_socket, server_message, sizeof(server_message));
    if (count <= 0) {
        printf("could not get ID\n");
        exit(1);
    }
    // get id from server
    //char * tempID = strdup("001");
    uint16_t client_id = (uint16_t) (server_message[0] | (uint16_t) server_message[1] << 8);
    printf("client id %hu\n", client_id);

    const uint16_t delay = dc_setting_uint16_get(env, app_settings->packet_delay);
    // 100 ticks/s
    const struct timeval tick_rate = {0, 10000};
    struct timeval timeout;
    timeout.tv_sec = tick_rate.tv_sec;
    timeout.tv_usec = tick_rate.tv_usec;

    client_info * clientInfo = calloc(1, sizeof(client_info));

    clientInfo->client_id = client_id;
    clientInfo->udp_socket = udp_server_sd;
    clientInfo->tcp_socket = tcp_server_socket;
    clientInfo->udp_address = &udp_server_addr;
    clientInfo->udp_adr_len = server_struct_length;
    clientInfo->has_client_entity = true;
    clientInfo->client_entity = calloc(1, sizeof (client));


    pthread_t thread_id;
    printf("Starting display thread\n");
    pthread_create(&thread_id, NULL, (void *(*)(void *)) ncurses_thread, clientInfo);
    //pthread_join(thread_id, NULL);


    fd_set readfds;
    while (!exit_flag) {

        if (timeout.tv_usec == 0) {
            timeout.tv_usec = tick_rate.tv_usec;
            //printf("putting out packet %lu\n", clientInfo->last_packet_sent + 1);
            send_game_state(clientInfo, udp_server_sd);
        }


        FD_ZERO(&readfds);
        FD_SET(udp_server_sd, &readfds);
        FD_SET(tcp_server_socket, &readfds);
        //FD_SET(STDIN_FILENO, &readfds);

        int maxfd = udp_server_sd;
        if (tcp_server_socket > udp_server_sd) {
            maxfd = udp_server_sd;
        }
        //printf("select\n");
        // the big select statement
        if(select(maxfd + 1, &readfds, NULL, NULL, &timeout) > 0){

//            if(FD_ISSET(STDIN_FILENO, &readfds)) {
//                exitFlag = true;
//            }

            // check for udp messages
            if(FD_ISSET(udp_server_sd, &readfds)) {
                receive_udp_packet(env, err, clientInfo);
            }

            // check for new client connections
            if (FD_ISSET(tcp_server_socket, &readfds))
            {
                exit_flag = true;
                //receive_tcp_packet(env, err, clientInfo, tcp_server_socket);
            }

        } else {
            //printf("select timed out\n");
        }
    }



    close(udp_server_sd);
    close(tcp_server_socket);

    return EXIT_SUCCESS;
}

static void receive_udp_packet(const struct dc_posix_env *env, struct dc_error *err, client_info *clientInfo) {
    uint8_t header[10] = {0};
    //printf("recv udp packet\n");
    ssize_t count;
    count = recvfrom(clientInfo->udp_socket, header, 10, MSG_PEEK | MSG_WAITALL, NULL, NULL);
    //printf("count %zd\n", count);
    if (count < 10) {
        //printf("packet not fully received\n");
        return;
    }
    uint64_t packet_no = (uint64_t) ((uint64_t) header[7] | (uint64_t) header[6] << 8 | (uint64_t) header[5] << 16 | (uint64_t) header[4] << 24 | (uint64_t) header[3] << 32 | (uint64_t) header[2] << 40 | (uint64_t) header[1] << 48 | (uint64_t) header[0] << 56);
    uint16_t num_entities = (uint16_t) (header[8] | (uint16_t) header[9] << 8);
    ssize_t buffer_size = num_entities * 6 + 10;
    uint8_t * buffer = calloc(1, (size_t) buffer_size);
    count = recvfrom(clientInfo->udp_socket, buffer, (size_t) buffer_size, MSG_WAITALL, NULL, NULL);
    if (count < buffer_size) {
        free(buffer);
        printf("count not full %d < %d", count, buffer_size);
        return;
    }

    //printf("received packet %lu from the server with entities %hu\n", packet_no, num_entities);
    if (packet_no <= clientInfo->last_packet_received) {
        //printf("old packet, discard\n");
        free(buffer);
        return;
    }

    free_client_entities_list(&clientInfo->client_entities);
    fill_entities_list(&clientInfo->client_entities, buffer + 10, num_entities);
    free(buffer);
    //update_player_position(clientInfo);
    draw_game(clientInfo);

}

static void update_player_position(client_info *clientInfo) {
    client_node * node = clientInfo->client_entities;
//    while (node) {
//        if (clientInfo->client_id == node->client_entity->client_id) {
//            clientInfo->client_entity = node->client_entity;
//        }
//        node = node->next;
//    }
}

static void fill_entities_list(client_node **entities_list, const uint8_t *entity_buffer, uint16_t num_entities) {
    size_t shift = 0;
    for (uint16_t entity_no = 0; entity_no < num_entities; ++entity_no) {
        client * new_entity = calloc(1, sizeof (client));
        new_entity->client_id = (uint16_t) (entity_buffer[0 + shift] | (uint16_t) entity_buffer[1 + shift] << 8);
        new_entity->position_x = (uint16_t) (entity_buffer[2 + shift] | (uint16_t) entity_buffer[3 + shift] << 8);
        new_entity->position_y = (uint16_t) (entity_buffer[4 + shift] | (uint16_t) entity_buffer[5 + shift] << 8);
        shift += 6;
        client_node * clientNode = calloc(1, sizeof (client_node));
        clientNode->client_entity = new_entity;
        clientNode->next = *entities_list;
        *entities_list = clientNode;
    }
}

static void free_client_entities_list(client_node **head) {
    client_node * node = *head;
    while (node) {
        free(node->client_entity);
        client_node * temp = node->next;
        free(node);
        node = temp;
    }
    *head = NULL;
}

static void send_game_state(client_info *clientInfo, const int udp_socket) {
    uint8_t packet[14];
    memset(packet, 0, 14);
    uint64_t packet_no = ++clientInfo->last_packet_sent;
    // FROM STACK OVERFLOW: https://stackoverflow.com/a/35153234
    // USER https://stackoverflow.com/users/3482801/straw1239
    for(int i = 0; i < 8; i++) {
        packet[i] = (uint8_t) ((packet_no >> 8*(7 - i)) & 0xFF);
    }
    packet[8] = clientInfo->client_id & 0xFF;
    packet[9] = clientInfo->client_id >> 8;


    if(clientInfo->client_entity) {
        packet[10] = clientInfo->client_entity->position_x & 0xFF;
        packet[11] = clientInfo->client_entity->position_x >> 8;
        packet[12] = clientInfo->client_entity->position_y & 0xFF;
        packet[13] = clientInfo->client_entity->position_y >> 8;
    }


    if(sendto(udp_socket, packet, 14, 0, (const struct sockaddr *) clientInfo->udp_address, clientInfo->udp_adr_len) < 0){
        printf("Unable to send message\n");
        exit(1);
    }
}

static void error_reporter(const struct dc_error *err)
{
    fprintf(stderr, "ERROR: %s : %s : @ %zu : %d\n", err->file_name, err->function_name, err->line_number, 0);
    fprintf(stderr, "ERROR: %s\n", err->message);
}

static void trace_reporter(__attribute__((unused)) const struct dc_posix_env *env,
                           const char *file_name,
                           const char *function_name,
                           size_t line_number)
{
    fprintf(stdout, "TRACE: %s : %s : @ %zu\n", file_name, function_name, line_number);
}

int connectToServer(struct dc_error *err, const struct dc_posix_env *env, const char *host_name, in_port_t port) {
    struct addrinfo hints;
    struct addrinfo *result;

    dc_memset(env, &hints, 0, sizeof(hints));
    hints.ai_family =  AF_INET; // AF_INET6;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_CANONNAME;
    dc_getaddrinfo(env, err, host_name, NULL, &hints, &result);

    if(dc_error_has_no_error(err))
    {
        int socket_fd;

        socket_fd = dc_socket(env, err, result->ai_family, result->ai_socktype, result->ai_protocol);

        if(dc_error_has_no_error(err))
        {
            struct sockaddr *sockaddr;
            in_port_t converted_port;
            socklen_t sockaddr_size;

            sockaddr = result->ai_addr;
            converted_port = htons(port);

            if(sockaddr->sa_family == AF_INET)
            {
                struct sockaddr_in *addr_in;

                addr_in = (struct sockaddr_in *)sockaddr;
                addr_in->sin_port = converted_port;
                sockaddr_size = sizeof(struct sockaddr_in);
            }
            else
            {
                if(sockaddr->sa_family == AF_INET6)
                {
                    struct sockaddr_in6 *addr_in;

                    addr_in = (struct sockaddr_in6 *)sockaddr;
                    addr_in->sin6_port = converted_port;
                    sockaddr_size = sizeof(struct sockaddr_in6);
                }
                else
                {
                    DC_ERROR_RAISE_USER(err, "sockaddr->sa_family is invalid", -1);
                    sockaddr_size = 0;
                }
            }

            if(dc_error_has_no_error(err))
            {
                dc_connect(env, err, socket_fd, sockaddr, sockaddr_size);

                if(dc_error_has_no_error(err))
                {
                    return socket_fd;
                }
            }
        }
    }
    if (dc_error_has_error(err)) {
        error_reporter(err);
        exit(1);
    }
    return -1;
}


size_t get_time_difference(int future_hour, int future_minute, int current_hour, int current_min, int current_seconds) {
    int seconds_future = future_hour * 3600 + future_minute * 60;
    int seconds_current = current_hour * 3600 + current_min * 60 + current_seconds;
    int seconds_difference =  (seconds_future - seconds_current);
    if (seconds_difference < 0) {
        seconds_difference += 86400;
    }
    return (size_t) seconds_difference;
}

void * ncurses_thread(client_info *clientInfo) {
    printf("thread started\n");
    char key = 0;


    // THIS CODE IS MODIFIED FROM LIAM'S DEMO
    initscr();            /* initialize ncurses screen */
    noecho();             /* disable rendering text on input */
    keypad(stdscr, TRUE); /* allow input from special keys */
    curs_set(0);          /* make cursor invisible */
    cbreak();             /* enables intant input */
    draw_game(clientInfo);

    printw("\n");
    //mvaddch(1, 1, '0');
    //timeout(50);
    while (!exit_flag && (key = (char)getch()) != EXIT_KEY) {
        clear();
        mvprintw(0, 20, "key: %c x: %d y: %d", key, clientInfo->client_entity->position_x, clientInfo->client_entity->position_y);

        switch (key) {
            case MOVE_UP:
                //printf("move_up");
                move_player(clientInfo->client_entity, -1, 0);
                break;
            case MOVE_LEFT:
                move_player(clientInfo->client_entity, 0, -1);
                break;
            case MOVE_DOWN:
                move_player(clientInfo->client_entity, 1, 0);
                break;
            case MOVE_RIGHT:
                move_player(clientInfo->client_entity, 0, 1);
                break;
            default:
                break;
        }
        draw_game(clientInfo);





        //mvaddch(clientInfo->client_entity->position_x, clientInfo->client_entity->position_y, '0');
        //printw("\n");

    }

    endwin();





//    const struct timeval tick_rate = {0, 10000};
//    struct timeval t1, t2;
//    long elapsed_us = 0;
//    struct timespec sleepTime;
//    while(true) {
//        // start timer
//        gettimeofday(&t1, NULL);
//
//
//
//
//
//        // stop timer
//        gettimeofday(&t2, NULL);
//        elapsed_us = (t2.tv_usec - t1.tv_usec);   // us to ms
//        if (elapsed_us > tick_rate.tv_usec) {
//            // were late
//        } else {
//            sleepTime.tv_nsec = (tick_rate.tv_usec - elapsed_us) * 1000;
//            nanosleep(&sleepTime, NULL);
//        }
//    }
    return 0;
}

void draw_game(client_info *clientInfo) {
    clear();

    // draw the entities if there are any
    if (clientInfo->client_entities) {
        client_node * clientNode = clientInfo->client_entities;

        while (clientNode) {
            uint16_t row = clientNode->client_entity->position_x;
            uint16_t col = clientNode->client_entity->position_y;
            //printf("entity @ %hu %hu", row, col);
            //move(clientNode->client_entity->position_y, clientNode->client_entity->position_x);
            if (clientNode->client_entity->client_id == clientInfo->client_id) {
                attron(COLOR_PAIR(COLOR_GREEN));
                mvaddch(row, col, '0');
                attroff(COLOR_PAIR(COLOR_GREEN));
            } else {
                attron(COLOR_PAIR(COLOR_RED));
                mvaddch(row, col, '0');
                attroff(COLOR_PAIR(COLOR_RED));
            }

            clientNode = clientNode->next;
        }
    }
    refresh();
}

void move_player(client *client_entity, int direction_x, int direction_y) {
    if ((client_entity->position_x == 0 && direction_x < 0) || (client_entity->position_x == 50 && direction_x > 0) || (client_entity->position_y == 0 && direction_y < 0) || (client_entity->position_y == 50 && direction_y > 0)) {
        return;
    }
    client_entity->position_x += direction_x;
    client_entity->position_y += direction_y;

}

void signal_handler(__attribute__ ((unused)) int signnum) {
    printf("\nexit flag set\n");
    exit_flag = 1;
    //exit(0);
}










