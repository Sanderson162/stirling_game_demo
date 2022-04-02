//
// Created by drep on 2022-04-01.
//
#include "network_util.h"

int create_tcp_server(const struct dc_posix_env *env, struct dc_error *err, const char *hostname, in_port_t port, const char * ip_version) {
    int tcp_server_sd;
    struct addrinfo hints;
    struct addrinfo *result;
    dc_memset(env, &hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_CANONNAME;

    if (dc_strcmp(env, ip_version, "IPv4") == 0) {
        hints.ai_family = AF_INET;
    } else {
        if (dc_strcmp(env, ip_version, "IPv6") == 0) {
            hints.ai_family = AF_INET6;
        } else {
            DC_ERROR_RAISE_USER(err, "Invalid ip_version", -1);
            hints.ai_family = 0;
        }
    }
    dc_getaddrinfo(env, err, hostname, NULL, &hints, &result);
    if (dc_error_has_error(err)){
        return -1;
    }

    tcp_server_sd = dc_socket(env, err, result->ai_family, result->ai_socktype, result->ai_protocol);
    if (dc_error_has_error(err)){
        return -1;
    }

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

    if (dc_error_has_error(err)){
        return -1;
    }
    dc_bind(env, err, tcp_server_sd, sockaddr, sockaddr_size);

    if (dc_error_has_error(err)){
        return -1;
    }

    int backlog;

    backlog = 5;
    dc_listen(env, err, tcp_server_sd, backlog);

    if (dc_error_has_error(err)){
        return -1;
    }

    printf("Listening for incoming tcp messages on port %d\n", port);

    return tcp_server_sd;
}


int connect_to_tcp_server(const struct dc_posix_env *env, struct dc_error *err, const char *host_name, in_port_t port, const char * ip_version) {
    struct addrinfo hints;
    struct addrinfo *result;

    dc_memset(env, &hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_CANONNAME;

    if (dc_strcmp(env, ip_version, "IPv4") == 0) {
        hints.ai_family = AF_INET;
    } else {
        if (dc_strcmp(env, ip_version, "IPv6") == 0) {
            hints.ai_family = AF_INET6;
        } else {
            DC_ERROR_RAISE_USER(err, "Invalid ip_version", -1);
            hints.ai_family = 0;
        }
    }

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
        return -1;
    }
    return -1;
}
