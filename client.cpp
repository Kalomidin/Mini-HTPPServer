#include <arpa/inet.h>
#include <sys/socket.h>
#include <iostream>
#include <netinet/in.h>
#include <string.h>
#include <netdb.h>
#include <unistd.h>
#include "server.cpp"
#include <future>
#include <mutex>
#include <thread>
#include <chrono>

#define BUF_SIZE 1024

enum MessageType {
    GET,
    POST,
};

void send_message(unsigned short port, const char *ip);
int write_get(int sockfd);
int read_file(int sockfd);


int main() {
    vector<future<int>> results;
    auto start = chrono::high_resolution_clock::now();
    /* Create a thread pool*/
    vector<thread> threads(THREADS);

    for(int i = 0; i < 1000; i++) {
        // send_message(8080, "127.0.0.1");
        // auto val = std::async(launch::async, send_message, 8080, "127.0.0.1");
        // val.get();
        // results.push_back(std::async(launch::async, send_message, 8080, "127.0.0.1"));
        t_start(threads, [=]{send_message(8080, "127.0.0.1");});
    }
    for(auto &t: threads) {
        if (t.joinable())
            t.join();
    }
    auto stop = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::microseconds>(stop - start);
    cout << "Time taken: "
         << duration.count() << " microseconds" << endl;
    // printf("\nSend message response: %d\n", response);
}


/* Returns -1 on fail and 0 if succeeded*/
void send_message(unsigned short port, const char *ip) {
    int sockfd = 0; char recv_buff[BUF_SIZE];
    struct sockaddr_in sock_addr;
    /* Initialize the receiving buffer*/
    memset(recv_buff, '0', sizeof(recv_buff));
    // printf("Send message is running2\n");

    /* Create the socket*/
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("Failed to create a socket");
        // return -1;
    }

    /* Initialize the server address*/
    bzero((char *) &sock_addr, sizeof(sock_addr)); // Why

    sock_addr.sin_family = AF_INET;
    sock_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, ip, &sock_addr.sin_addr) <= 0) {
        printf("Converting ip to binary failure");
        // return -1;
    }

    if(connect(sockfd, (sockaddr *) &sock_addr, sizeof(sock_addr)) < 0) {
        printf("Connection failed\n");
        // return -1;
    }
    // printf("Connected to the server\n");
    write_get(sockfd);
    read_mes(sockfd);
    close(sockfd);
}

int write_get(int sockfd) {
    string message = "GET / HTTP/1.1\n";
    write(sockfd, message.c_str(), message.length());
    return 1;
}