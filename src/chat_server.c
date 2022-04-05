//
// Created by drep on 2022-04-03.
//

#include <chat_server.h>
typedef struct {
    int id;
    int client_socket;
    socklen_t client_socklen;
    struct sockaddr_un client_sockaddr;
} client_sock_un;
void * run_chat_server(const char * parent_socket_path) {
//    printf("client: starting child thread\n");
//
//
//
//    char buf[256];
//
//
//
//    client_sock_un * clientSockUn;
//
//    clientSockUn = connect_to_unix_socket(parent_socket_path);
//
//
//    fd_set fdSet;
//
//    //
//    const struct timeval tick_rate = {0, 500000};
//    struct timeval timeout = tick_rate;
//
//    int exit_flag = 0;
//
//    while (!exit_flag) {
//
//        if (timeout.tv_usec == 0) {
//            timeout.tv_usec = tick_rate.tv_usec;
//            const char * message = "##### client to server";
//            char * buffer = strdup(message);
//            size_t buffer_size = strlen(buffer);
//            send_data_to_server(clientSockUn, buffer, buffer_size);
//            free(buffer);
//        }
//
//
//        FD_ZERO(&fdSet);
//        FD_SET(clientSockUn->client_socket, &fdSet);
//        FD_SET(STDIN_FILENO, &fdSet);
//
//        int maxfd = clientSockUn->client_socket;
//
//
//        // the big select statement
//        if(select(maxfd + 1, &fdSet, NULL, NULL, &timeout) > 0){
//
//            if(FD_ISSET(STDIN_FILENO, &fdSet)) {
//                exit_flag = 1;
//            }
//
//            // check for connection on IPC
//            if (FD_ISSET(clientSockUn->client_socket, &fdSet))
//            {
//                read_server_message(clientSockUn);
//            }
//
//
//        } else {
//            //printf("select timed out\n");
//        }
//    }
//
//    /******************************/
//    /* Close the socket and exit. */
//    /******************************/
//    close(clientSockUn->client_socket);
}
