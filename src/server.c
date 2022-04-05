
#include <server.h>

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
    info = dc_application_info_create(&env, &err, "Settings Application");

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
    settings->filename = dc_setting_string_create(env, err);
    settings->server_ip = dc_setting_string_create(env, err);
    settings->server_udp_port = dc_setting_uint16_create(env, err);
    settings->server_tcp_port = dc_setting_uint16_create(env, err);

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
            {(struct dc_setting *)settings->filename,
                    dc_options_set_string,
                    "filename",
                    required_argument,
                    'f',
                    "FILENAME",
                    dc_string_from_string,
                    "filename",
                    dc_string_from_config,
                    "logfile.txt"},
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


static void process_game_state(server_info *serverInfo);

static uint16_t count_bullets(bullet_node *bulletNode);

static int destroy_settings(const struct dc_posix_env *env,
                            __attribute__((unused)) struct dc_error *err,
                            struct dc_application_settings **psettings)
{
    struct application_settings *app_settings;

    DC_TRACE(env);
    app_settings = (struct application_settings *)*psettings;
    dc_setting_string_destroy(env, &app_settings->filename);
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
    DC_TRACE(env);
    struct application_settings *app_settings;
    app_settings = (struct application_settings *)settings;


    //open file
    const char *filename;
    filename = dc_setting_string_get(env, app_settings->filename);
    printf("opening file \"%s\"\n", filename);
    //int logfile_fd = dc_open(env, err, filename, DC_O_WRONLY|DC_O_CREAT|DC_O_TRUNC, DC_S_IRUSR|DC_S_IWUSR|DC_S_IRGRP|DC_S_IROTH);


    // create admin server info for sockets and client_lists
    admin_server_info * adminServerInfo = dc_calloc(env, err, 1, sizeof(admin_server_info));
    if (dc_error_has_error(err)){
        error_reporter(err);
        exit(1);
    }

    //TCP ADMIN SERVER
    adminServerInfo->admin_server_socket = create_tcp_server(env, err, dc_setting_string_get(env, app_settings->server_ip), DEFAULT_TCP_PORT_ADMIN_SERVER, DEFAULT_IP_VERSION);
    if (dc_error_has_error(err) || adminServerInfo->admin_server_socket <= 0) {
        printf("could not create TCP server for admin\n");
        exit(1);
    }

    // create server info for sockets and client_lists
    server_info * serverInfo = dc_calloc(env, err, 1, sizeof(server_info));
    if (dc_error_has_error(err)){
        error_reporter(err);
        exit(1);
    }

    //TCP server
    int tcp_server_sd;
    tcp_server_sd = create_tcp_server(env, err, dc_setting_string_get(env, app_settings->server_ip), dc_setting_uint16_get(env, app_settings->server_tcp_port), DEFAULT_IP_VERSION);
    if (dc_error_has_error(err) || tcp_server_sd <= 0) {
        printf("could not create TCP server for admin\n");
        exit(1);
    }

    // Create UDP socket:
    int udp_server_sd = create_udp_server(env, err, dc_setting_string_get(env, app_settings->server_ip), dc_setting_uint16_get(env, app_settings->server_udp_port));


    fd_set readfds;

    uint64_t packet_no = 0;
    // 100 ticks/s
    const struct timeval tick_rate = {0, 10000};
    struct timeval timeout = tick_rate;

    while (!exit_flag) {

        if (timeout.tv_usec == 0) {
            timeout.tv_usec = tick_rate.tv_usec;
            //printf("putting out packet %lu\n", packet_no);
            packet_no++;
            process_game_state(serverInfo);
            send_game_state(serverInfo, udp_server_sd);
        }


        FD_ZERO(&readfds);
        FD_SET(udp_server_sd, &readfds);
        FD_SET(tcp_server_sd, &readfds);
        FD_SET(adminServerInfo->admin_server_socket, &readfds);
        FD_SET(STDIN_FILENO, &readfds);

        int maxfd = udp_server_sd;
        if (tcp_server_sd > maxfd) {
            maxfd = tcp_server_sd;
        }

        if (adminServerInfo->admin_server_socket > maxfd) {
            maxfd = adminServerInfo->admin_server_socket;
        }

        for (uint16_t client_id = 0 ; client_id < MAX_TCP_CLIENTS ; client_id++)
        {
            //if valid socket descriptor then add to read list
            if(serverInfo->connections[client_id])
            {
                //printf("polling connected client %hu socket: %d\n", client_id, serverInfo->connections[client_id]->tcp_socket);
                FD_SET(serverInfo->connections[client_id]->tcp_socket, &readfds);
                //highest file descriptor number, need it for the select function
                if(serverInfo->connections[client_id]->tcp_socket > maxfd) {
                    maxfd = serverInfo->connections[client_id]->tcp_socket;
                }
            }


        }

        for (uint16_t client_id = 0 ; client_id < MAX_ADMIN_CLIENTS ; client_id++)
        {
            //if valid socket descriptor then add to read list
            if(adminServerInfo->adminClientList[client_id])
            {
                //printf("polling connected client %hu socket: %d\n", client_id, serverInfo->connections[client_id]->tcp_socket);
                FD_SET(adminServerInfo->adminClientList[client_id]->tcp_socket, &readfds);
                //highest file descriptor number, need it for the select function
                if(adminServerInfo->adminClientList[client_id]->tcp_socket > maxfd) {
                    maxfd = adminServerInfo->adminClientList[client_id]->tcp_socket;
                }
            }


        }


        // the big select statement
        if(select(maxfd + 1, &readfds, NULL, NULL, &timeout) > 0){

            if(FD_ISSET(STDIN_FILENO, &readfds)) {
                exit_flag = true;
            }

            // loop through list of clients and check for select event
            for (uint16_t client_id = 0; client_id < MAX_TCP_CLIENTS; client_id++)
            {
                if (serverInfo->connections[client_id] && FD_ISSET(serverInfo->connections[client_id]->tcp_socket, &readfds))
                {
                    receive_tcp_packet(env, err, serverInfo, client_id);
                }
            }

            // loop through list of clients and check for select event
            for (uint16_t client_id = 0; client_id < MAX_ADMIN_CLIENTS; client_id++)
            {
                if (adminServerInfo->adminClientList[client_id] && FD_ISSET(adminServerInfo->adminClientList[client_id]->tcp_socket, &readfds))
                {
                    admin_receiveTcpPacket(env, err, adminServerInfo, serverInfo, client_id);
                }
            }

            // check for udp messages
            if(FD_ISSET(udp_server_sd, &readfds)) {
                receive_udp_packet(env, err, serverInfo, udp_server_sd);
            }

            // check for new client connections
            if (FD_ISSET(tcp_server_sd, &readfds))
            {
                acceptTCPConnection(env, err, serverInfo, tcp_server_sd);
            }

            // check for new client connections
            if (FD_ISSET(adminServerInfo->admin_server_socket, &readfds))
            {
                admin_acceptTCPConnection(env, err, adminServerInfo);
            }

        } else {
            //printf("select timed out\n");
        }
    }

    // Close the sockets:
    close(udp_server_sd);
    close(tcp_server_sd);
    close(adminServerInfo->admin_server_socket);

    return EXIT_SUCCESS;
}

static void process_game_state(server_info *serverInfo) {


    //update player locations
    for (uint16_t client_id = 0; client_id < MAX_CLIENTS; ++client_id) {
        if (serverInfo->connections[client_id] && serverInfo->connections[client_id]->has_client_entity) {
            client * current_client;
            current_client = serverInfo->connections[client_id]->client_entity;
            uint16_t new_x = current_client->position_x;
            uint16_t new_y = current_client->position_y;
            // move player based off input state;
            if (current_client->inputState.move_up) {
                new_x--;
            }
            if (current_client->inputState.move_down) {
                new_x++;
            }
            if (current_client->inputState.move_left) {
                new_y--;
            }
            if (current_client->inputState.move_right) {
                new_y++;
            }
            //validate new position
            if (validate_user_position(new_x, new_y)) {
                current_client->position_x = new_x;
                current_client->position_y = new_y;
            } else if (validate_user_position(new_x, current_client->position_y)) {
                current_client->position_x = new_x;
            } else if (validate_user_position(current_client->position_x, new_y)) {
                current_client->position_y = new_y;
            } else if (!validate_user_position(current_client->position_x, current_client->position_y)) {
                current_client->position_x = 0;
                current_client->position_y = 0;
            }
            if (current_client->inputState.shoot_up || current_client->inputState.shoot_down || current_client->inputState.shoot_left || current_client->inputState.shoot_right) {
                // player will shoot a bullet this tick
                bullet * new_bullet;
                new_bullet = calloc(1, sizeof(bullet));
                new_bullet->shooters_id = client_id;
                new_bullet->position_x = current_client->position_x;
                new_bullet->position_y = current_client->position_y;
                if (current_client->inputState.shoot_up) {
                    new_bullet->direction_x--;
                }
                if (current_client->inputState.shoot_down) {
                    new_bullet->direction_x++;
                }
                if (current_client->inputState.shoot_left) {
                    new_bullet->direction_y--;
                }
                if (current_client->inputState.shoot_right) {
                    new_bullet->direction_y++;
                }
                if (new_bullet->direction_x == 0 && new_bullet->direction_y == 0) {
                    free(new_bullet);
                } else {
                    bullet_node * new_bullet_node;
                    new_bullet_node = calloc(1, sizeof(bullet));
                    new_bullet_node->bullet = new_bullet;
                    new_bullet_node->next = serverInfo->bulletList;
                    serverInfo->bulletList = new_bullet_node;
                }
            }


            memset(&current_client->inputState, 0, sizeof(client_input_state));
        }
    }

    //update bullet position
    bullet_node * bulletNode_prev = NULL;
    bullet_node * bulletNode = serverInfo->bulletList;

    while (bulletNode) {
        bulletNode->bullet->position_x += bulletNode->bullet->direction_x;
        bulletNode->bullet->position_y += bulletNode->bullet->direction_y;
        if (!validate_user_position(bulletNode->bullet->position_x, bulletNode->bullet->position_y)) {
            if (bulletNode_prev == NULL) {
                serverInfo->bulletList = bulletNode->next;
            } else {
                bulletNode_prev->next = bulletNode->next;
            }
            free(bulletNode->bullet);
            bullet_node * temp_node = bulletNode;
            bulletNode = bulletNode->next;
            free(temp_node);
        } else {
            bulletNode_prev = bulletNode;
            bulletNode = bulletNode->next;
        }

    }


    // check if any bullets collided with players
    for (uint16_t client_id = 0; client_id < MAX_CLIENTS; ++client_id) {
        if (serverInfo->connections[client_id] && serverInfo->connections[client_id]->has_client_entity) {
            client *current_client;
            current_client = serverInfo->connections[client_id]->client_entity;
            uint16_t x = current_client->position_x;
            uint16_t y = current_client->position_y;
            bulletNode_prev = NULL;
            bulletNode = serverInfo->bulletList;
            while (bulletNode) {
                if (bulletNode->bullet->position_x == x && bulletNode->bullet->position_y == y) {
                    if (bulletNode->bullet->shooters_id != client_id) {
                        current_client->position_x = 0;
                        current_client->position_y = 0;
                    }
                    if (bulletNode_prev == NULL) {
                        serverInfo->bulletList = bulletNode->next;
                    } else {
                        bulletNode_prev->next = bulletNode->next;
                    }
                    free(bulletNode->bullet);
                    bullet_node * temp_node = bulletNode;
                    bulletNode = bulletNode->next;
                    free(temp_node);
                } else {
                    bulletNode_prev = bulletNode;
                    bulletNode = bulletNode->next;
                }

            }
        }
    }

}


bool validate_user_position(uint16_t x, uint16_t y) {
    if (x > 50 || y > 50) {
        return false;
    }
    return true;
}

static void send_game_state(server_info *serverInfo, int udp_socket) {
    uint64_t packet_no = ++serverInfo->last_packet_no;



    uint16_t num_entities;
    connection_node * clients_with_entities = get_active_clients(serverInfo, &num_entities);

    uint16_t num_bullets;
    num_bullets = count_bullets(serverInfo->bulletList);

    uint8_t * header = calloc(12, 1);


    // FROM STACK OVERFLOW: https://stackoverflow.com/a/35153234
    // USER https://stackoverflow.com/users/3482801/straw1239
    for(int i = 0; i < 8; i++) {
        header[i] = (uint8_t) ((packet_no >> 8*(7 - i)) & 0xFF);
    }

    header[8] = num_entities & 0xFF;
    header[9] = num_entities >> 8;

    header[10] = num_bullets & 0xFF;
    header[11] = num_bullets >> 8;

    uint8_t * entity_list = calloc(num_entities, 6);

    uint16_t entities_added = 0;
    size_t shift = 0;
    connection_node * node = clients_with_entities;
    while (node) {
        client * current_client = serverInfo->connections[node->client_id]->client_entity;
        if (current_client) {
            *(entity_list + shift++) = current_client->client_id & 0xFF;
            *(entity_list + shift++) = current_client->client_id >> 8;
            *(entity_list + shift++) = current_client->position_x & 0xFF;
            *(entity_list + shift++) = current_client->position_x >> 8;
            *(entity_list + shift++) = current_client->position_y & 0xFF;
            *(entity_list + shift++) = current_client->position_y >> 8;
             entities_added++;
        }

        node = node->next;
    }


    uint8_t * bullet_list = calloc(num_bullets, 4);
    uint16_t bullets_added = 0;
    shift = 0;
    bullet_node * bulletNode = serverInfo->bulletList;
    while (bulletNode) {
        *(bullet_list + shift++) = bulletNode->bullet->position_x & 0xFF;
        *(bullet_list + shift++) = bulletNode->bullet->position_x >> 8;
        *(bullet_list + shift++) = bulletNode->bullet->position_y & 0xFF;
        *(bullet_list + shift++) = bulletNode->bullet->position_y >> 8;
        bullets_added++;
        bulletNode = bulletNode->next;
    }

    size_t buffer_size = entities_added * 6 + bullets_added * 4 + 12;
    uint8_t * buffer = calloc(buffer_size, 1);
    memcpy(buffer, header, 12);
    memcpy((buffer + 12), entity_list, entities_added * 6);
    memcpy((buffer + 12 + entities_added * 6), bullet_list, bullets_added * 4);
    free(header);
    free(entity_list);
    free(bullet_list);

    node = clients_with_entities;
    while (node) {
        connection * current_client_conn = serverInfo->connections[node->client_id];
        if (current_client_conn->udp_address) {
            //printf("sending packete with length: %zu\n", buffer_size);
            sendto(udp_socket, buffer, buffer_size, 0, (const struct sockaddr *) current_client_conn->udp_address, current_client_conn->udp_adr_len);
        }

        node = node->next;
    }

    free(buffer);
}

static uint16_t count_bullets(bullet_node *bulletNode) {
    uint16_t count = 0;
    while (bulletNode){
        count++;
        bulletNode = bulletNode->next;
    }
    return count;
}

static connection_node * get_active_clients(server_info *serverInfo, uint16_t *num_entities) {
    *num_entities = 0;
    connection_node * head = NULL;

    for (uint16_t client_id = 0; client_id < MAX_CLIENTS; ++client_id) {
        if (serverInfo->connections[client_id] && serverInfo->connections[client_id]->has_client_entity) {
            connection_node * temp = calloc(1, sizeof (connection_node));
            temp->client_id = client_id;
            temp->next = head;
            head = temp;
            (*num_entities)++;
        }
    }

    return head;
}

static int create_udp_server(const struct dc_posix_env *env, struct dc_error *err, const char *hostname, in_port_t port) {
    int udp_server_sd;
    struct sockaddr_in udp_server_addr;
    udp_server_sd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    if(udp_server_sd < 0){
        printf("Error while creating socket\n");
        return -1;
    }
    //printf("Socket created successfully\n");

    // Set port and IP:
    udp_server_addr.sin_family = AF_INET;
    udp_server_addr.sin_port = htons(port);
    udp_server_addr.sin_addr.s_addr = inet_addr(hostname);

    // Bind to the set port and IP:
    if(bind(udp_server_sd, (struct sockaddr*)&udp_server_addr, sizeof(udp_server_addr)) < 0){
        printf("Couldn't bind to the port\n");
        return -1;
    }
    //printf("Done with binding\n");
    printf("Listening for incoming udp messages on port %d\n", port);
    return udp_server_sd;
}

static void acceptTCPConnection(const struct dc_posix_env *env, struct dc_error *err, server_info *serverInfo, int tcp_server_sd) {
    //accept new tcp connectionasd
    printf("new TCP connection request\n");
    int client_tcp_socket = dc_accept(env, err, tcp_server_sd, NULL, NULL);
    if (dc_error_has_no_error(err)) {
        printf("connected client\n");
        addToClientList(serverInfo, client_tcp_socket);
    } else {
        if(err->type == DC_ERROR_ERRNO && err->errno_code == EINTR) {
            dc_error_reset(err);
        }
    }
}

static void
receive_tcp_packet(const struct dc_posix_env *env, struct dc_error *err, server_info *serverInfo, uint16_t client_id) {


    char *data;
    ssize_t count;
    size_t size = 1024;
    data = dc_malloc(env, err, size);

    int client_socket = serverInfo->connections[client_id]->tcp_socket;
    printf("new tcp event from user: %d socket %d\n", client_id, client_socket);

    bool readComplete = false;
    while(!readComplete && !(exit_flag) && (count = dc_read(env, err, client_socket, data, size)) > 0 && dc_error_has_no_error(err))
    {
        readComplete = true;

    }
    if (count <= 0) {
        //Somebody disconnected , get his details and print
        printf("client disconnected id: %d sd %d\n" , client_id, client_socket);
        removeFromClientList(serverInfo, client_id);
        //Close the socket and mark as 0 in list for reuse
        readComplete = true;
    }
    free(data);
}

static void
receive_udp_packet(const struct dc_posix_env *env, struct dc_error *err, server_info *serverInfo, int udp_server_sd) {
    uint8_t client_message[11];
    size_t client_message_len = 11;
    memset(client_message, '\0', client_message_len);
    struct sockaddr_in udp_client_addr;
    socklen_t client_struct_length = sizeof(udp_client_addr);
    ssize_t count;
    count = dc_recvfrom(env, err, udp_server_sd, client_message, client_message_len, 0, (struct sockaddr*) &udp_client_addr, &client_struct_length);
    //printf("count %zd\n", count);
    if (count < client_message_len) {
        printf("packet not fully received\n");
        return;
    }
    //client_message[count] = 0;
    uint64_t packet_no = (uint64_t) ((uint64_t) client_message[7] | (uint64_t) client_message[6] << 8 | (uint64_t) client_message[5] << 16 | (uint64_t) client_message[4] << 24 | (uint64_t) client_message[3] << 32 | (uint64_t) client_message[2] << 40 | (uint64_t) client_message[1] << 48 | (uint64_t) client_message[0] << 56);
    uint16_t client_id = (uint16_t) (client_message[8] | (uint16_t) client_message[9] << 8);
    //uint16_t position_x = (uint16_t) (client_message[10] | (uint16_t) client_message[11] << 8);
    //uint16_t position_y = (uint16_t) (client_message[12] | (uint16_t) client_message[13] << 8);

    client_input_state packet_inputs;
    packet_inputs.move_up = client_message[10] & 0x1;
    packet_inputs.move_down = client_message[10] & 0x2;
    packet_inputs.move_left = client_message[10] & 0x4;
    packet_inputs.move_right = client_message[10] & 0x8;
    packet_inputs.shoot_up = client_message[10] & 0x10;
    packet_inputs.shoot_down = client_message[10] & 0x20;
    packet_inputs.shoot_left = client_message[10] & 0x40;
    packet_inputs.shoot_right = client_message[10] & 0x80;

    //printf("client packet with no %lu id %d x %d y %d\n", packet_no, client_id, position_x, position_y);
    connection * client_conn = serverInfo->connections[client_id];
    // check if client exists
    if (!client_conn) {
        printf("client does not exist\n");
        return;
    }
    if (client_conn->last_packet_no >= packet_no) {
        // disregard old message;
        return;
    }
    client_conn->last_packet_no = packet_no;
    // check if has udp address attached
    if (!client_conn->udp_address) {
        client_conn->udp_address = calloc(1, client_struct_length);
        memcpy(client_conn->udp_address, &udp_client_addr, client_struct_length);
        client_conn->udp_adr_len = client_struct_length;
    } else if (!compare_udp_sockets(client_conn->udp_address, &udp_client_addr)){
        // TODO:  address changed but id remained the same
        return;
    }

    if (!client_conn->has_client_entity) {
        client_conn->has_client_entity = true;
        client_conn->client_entity = calloc(1, sizeof(client));
    }
    memcpy(&client_conn->client_entity->inputState, &packet_inputs, sizeof(client_input_state));

//    client_conn->client_entity->position_x = position_x;
//    client_conn->client_entity->position_y = position_y;


//    printf("Received udp message from IP: %s and port: %i\n", inet_ntoa((udp_client_addr).sin_addr), ntohs((udp_client_addr).sin_port));
//    printf("%s\n", client_message);
//    const int header_length = 12;
//    client_message[header_length] = '\n';
//    dc_write(env, err, logfile_fd, "UDP ", dc_strlen(env, "UDP "));
//    dc_write(env, err, logfile_fd, client_message, header_length + 1);
}

static bool compare_udp_sockets(struct sockaddr_in *original, struct sockaddr_in *received) {
    return original->sin_port == received->sin_port && original->sin_addr.s_addr == received->sin_addr.s_addr;
}

void removeFromClientList(server_info *serverInfo, uint16_t client_id) {

    connection * client_conn = serverInfo->connections[client_id];
    if (!client_conn) {
        printf("client does not exist to be removed\n");
        return;
    }

    if (client_conn->has_client_entity) {
        free(client_conn->client_entity);
    }

    if (client_conn->udp_address) {
        free(client_conn->udp_address);
    }
    close(client_conn->tcp_socket);
    free(client_conn);
    serverInfo->connections[client_id] = NULL;

}

static void addToClientList(server_info *serverInfo, int client_tcp_socket) {

    for (uint16_t client_id = 0; client_id < MAX_CLIENTS; ++client_id) {
        if (serverInfo->connections[client_id] == NULL) {
            printf("Adding to list of sockets as %d with no %d\n" , client_id, client_tcp_socket);
            serverInfo->connections[client_id] = calloc(1, sizeof(connection));
            serverInfo->connections[client_id]->client_id = client_id;
            serverInfo->connections[client_id]->tcp_socket = client_tcp_socket;
            // everything else is null since calloc
            uint8_t buffer[2] = {client_id & 0xFF, client_id >> 8};
            write(client_tcp_socket, buffer, 2);
            return;
        }
    }
    close(client_tcp_socket);
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


void signal_handler(__attribute__ ((unused)) int signnum) {
    printf("\nServer Terminated\n");
    exit_flag = 1;
    //exit(0);
}
