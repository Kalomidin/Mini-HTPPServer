#include <sys/socket.h>
#include <iostream>
#include <netinet/in.h>
#include <string.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/stat.h>
#include <unordered_map>
#include <filesystem>
#include <fstream>
#include <vector>
#include <threads.h>
#include <thread>
#include <mutex>
#include <functional>
#include <poll.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include "helper.cpp"

bool make_socket_unblocking(int socketfd);
bool new_connection(int parentfd, struct epoll_event &event, int epollfd);

bool make_socket_unblocking(int socketfd) {
    int flags = fcntl(socketfd, F_GETFL, 0);

    if (flags == -1) {
        error("fcntl returned invalid flags for the socketfd\n");
        return false;
    }
    flags = flags | O_NONBLOCK;
    if(fcntl(socketfd, F_SETFL, flags) == -1) {
        error("fcntl returned invalid flags for the socketfd\n");
        return false;
    }

    return true;
}


bool new_connection(int parentfd, struct epoll_event &event, int epollfd) {
    struct sockaddr clientaddr;
    socklen_t clientlen = sizeof(clientaddr);
    int chilfd = accept(parentfd, &clientaddr, &clientlen);
    if (chilfd == -1)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK) // Done processing incoming connections
        {
            return false;
        }
        else
        {
            error("Failed while accepting new client\n");
            return false;
        }
    }

    /* Make new created socket nonblocking*/
    if(!make_socket_unblocking(chilfd)) {
        error("Failed to make new accepted socket ublocking\n");
        return false;
    }

    /* Add new socket to the events*/
    event.data.fd = chilfd;
    event.events = EPOLLIN | EPOLLET;
    if(epoll_ctl(epollfd, EPOLL_CTL_ADD, chilfd, &event) == -1) {
        error("Failed to add even\n");
    }
    return true;
}