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
#include "epoll_funcs.cpp"
#include <mutex>

using namespace std;

#define MAXEVENTS 10000

/* Mutex for synchronization with multiple threads `epoll` */
// mutex mtx;

std::array<struct epoll_event, MAXEVENTS> events;

/* Signed up Users*/

unordered_map<string, string> users;



void process_request(int childfd, unordered_map<string, string> pages, vector<string> restricted_pages);

void simple_test(int parentfd, struct epoll_event &event, int epollfd) {
    while(new_connection(parentfd, event, epollfd)) {}
}

int run_server(unsigned short portno) {
    int optval;
    int parentfd;
    struct sockaddr_in serveraddr;
    int childfd;
    unordered_map<string, string> pages;
    vector<string> restricted_pages;
    restricted_pages.push_back("Mine.html");

    struct stat sbuf;

    /* Load the web contents */
    pages = load_contents();

    /* open the socket descriptor*/
    parentfd = socket(AF_INET, SOCK_STREAM, 0);

    if (parentfd < 0) {
        error("ERROR opening the socket");
    }

    optval = 1;
    setsockopt(parentfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval, sizeof(int));

    /*Bind the port to the socket*/
    bzero((char *) &serveraddr, sizeof(serveraddr)); // Why
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons(portno);

    if (bind(parentfd, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0) {
        error("Failed to bind");
    }

    make_socket_unblocking(parentfd);

    if (listen(parentfd, 100000) < 0) {
        error("Failed to listen");
    }

    /* Create a thread pool*/
    vector<thread> threads(THREADS);

    /* Create epoll */
    int epollfd = epoll_create1(0);
    if(epollfd == -1) {
        error("Failed to create epollfd\n");
    }
    struct epoll_event event;
    event.data.fd = parentfd;
    event.events = EPOLLIN | EPOLLET;

    if(epoll_ctl(epollfd, EPOLL_CTL_ADD, parentfd, &event) == -1) {
        error("Failed to add parentfd to the epoll queue\n");
    }

    printf("Server is running \n");

    while(1) {

        /* Get an readable events */
        int cnt_readable_events = epoll_wait(epollfd, events.data(), MAXEVENTS, -1);

        for(int i =0; i < cnt_readable_events; i++) {
            if(events[i].events & EPOLLERR || events[i].events & EPOLLHUP || !(events[i].events & EPOLLIN)) {
                error("Epoll error: Invalid event in the events\n");
            }
            else if(parentfd == events[i].data.fd) {
                // t_start(threads, [=]{simple_test(parentfd, events[i], epollfd);});
                while(new_connection(parentfd, event, epollfd)) {}
            } else {
                process_request(events[i].data.fd, pages, restricted_pages);
                // t_start(threads, [=]{process_request(events[i].data.fd, pages);});
            }
        }
    }


    for(auto &t: threads) {
        if (t.joinable())
            t.join();
    }
    return 1;
}

void process_request(int childfd, unordered_map<string, string> pages, vector<string> restricted_pages) {
    char buf[BUFSIZE];
    char method[BUFSIZE];  /* request method */
    char uri[BUFSIZE];     /* request uri */
    char version[BUFSIZE]; /* request method */
    read(childfd, buf, BUFSIZE);
    sscanf(buf, "%s %s %s\n", method, uri, version);
    
    /* Return if the method is not 'GET' and 'POST' */
    if (strcasecmp(method, "GET") && strcasecmp(method, "POST")) {
        clientError(buf, method, "501", "Not Implemented", 
            "Mine Server does not implement this method");
        write(childfd, buf, BUFSIZE);
        close(childfd);
    } else {

        /* Execute the 'GET' request */
        if (strstr(method, "GET")) {
            char filename[BUFSIZE];

            if (uri[strlen(uri) - 1] == '/')
                strcpy(filename, "index.html"); 
            else
                strcpy(filename, uri + 1);

            /* make sure the file exists */
            if (pages.find(filename) == pages.end()) {
                clientError(buf, filename, "404", "Not found", 
                    "Mine Server couldn't find this file");
                write(childfd, buf, BUFSIZE);
                close(childfd);
                return;
            }

            /* Make sure it is not restricted page*/
            for(auto &page: restricted_pages) {
                if (strstr(filename, page.c_str())) {
                    clientError(buf, filename, "403", "Forbidden", 
                        "The client does not have access rights to the content; that is, \
                        it is unauthorized, so the server is refusing to give the requested resource");
                    write(childfd, buf, BUFSIZE);
                    close(childfd);
                    return;
                }
            }

            size_t length = pages[filename].length();
            set_http_body(buf, "200", "OK", length);
            string response = buf + pages[filename];
            write(childfd, response.c_str(), response.length());
            close(childfd);
        } 
        /* Execute the 'POST' request */
        else {
            string email, password;
            if(!set_email_psw(buf, email, password)) {
                clientError(buf, uri, "400", "Bad Request", 
                    "The server could not understand the request due to invalid syntax");
                write(childfd, buf, BUFSIZE);
                close(childfd);
                return;
            }
            if (strstr(uri, "/login")) {
                unordered_map<std::string, std::string>::iterator val = users.find(email);
                if(val != users.end() && val->first == password) {
                    /* Return `restricted pages at index 0` */
                    size_t length = pages[restricted_pages[0]].length();
                    set_http_body(buf, "200", "OK", length);
                    string response = buf + pages[restricted_pages[0]];
                    write(childfd, response.c_str(), response.length());
                    close(childfd);
                } else {
                string filename = "failure.html";
                size_t length = pages[filename].length();
                set_http_body(buf, "200", "OK", length);
                string response = buf + pages[filename];
                // printf("Response: %s\n", response.c_str());
                write(childfd, response.c_str(), response.length());
                close(childfd);
                }
            } else if (strstr(uri, "/signup")) {
                users[email] = password;
                string filename = "index.html";
                size_t length = pages[filename].length();
                set_http_body(buf, "200", "OK", length);
                string response = buf + pages[filename];
                // printf("Response: %s\n", response.c_str());
                write(childfd, response.c_str(), response.length());
                close(childfd);
            } else {
                clientError(buf, uri, "404", "Not found", 
                    "Mine Server couldn't find this file");
                write(childfd, buf, BUFSIZE);
                close(childfd);
                return;
            }
        }            
    }
}