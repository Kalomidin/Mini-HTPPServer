/***
 * 
 * This file includes helper functions for `server.c`
 * 
 * **/

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

using namespace std;
#define BUFSIZE 1024
#define THREADS 1

void error(string msg);
void clientError(char *str, char *cause, const char *err, 
	    const char *shortmsg, const char *longmsg);
void set_http_body(char *str, const char *code, const char*status, size_t content_length);
unordered_map<string, string> load_contents();
void t_start(vector<thread> &threads, std::function<void()>&& job);
int read_mes(int fd);

bool set_email_psw(char * buf, string &email, string &psw) {
    string temp = buf;
    size_t email_pos = temp.find("email");
    if(email_pos == string::npos)
        return false;
    email_pos += strlen("email") + 1;
    size_t psw_pos = temp.find("psw", email_pos);
    if(psw_pos == string::npos)
        return false;
    psw_pos += strlen("psw") + 1;
    email = temp.substr(email_pos, psw_pos - 5 - email_pos).c_str();
    psw = temp.substr(psw_pos, temp.length() - psw_pos).c_str();
    printf("Email: %s, psw: %s\n", email.c_str(), psw.c_str());
    return true;
}

/* Prints Error message */
void error(string msg) {
    perror(msg.c_str());
    exit(1);
}

/*
 * clientError - returns an error message to the client
 */
void clientError(char *str, char *cause, const char *err, 
	    const char *shortmsg, const char *longmsg) {
    string content = "<html><title>Mine Error</title>\n";
    content = content + "<body bgcolor=""ffffff"">\n";
    content = content + "<h1>" + err +": " + shortmsg + "</h1>\n";
    content = content + "<p>" + longmsg + ": " + cause + "\n";
    content = content + "</body></html>\n";
    set_http_body(str, err, shortmsg, content.length());
    sprintf(str, "%s%s", str, content.c_str());
}

/* Sets the http body*/
void set_http_body(char *str, const char *code, const char*status, size_t content_length) {
    sprintf(str, "HTTP/1.1 %s %s\n", code, status);
    sprintf(str, "%sServer: Mine Web \nContent-Length: %ld\n", str, content_length);
    sprintf(str, "%sContent-type: text/html\n", str);
    sprintf(str, "%sConnection: Closed\n\n", str);
}

unordered_map<string, string> load_contents() {
    unordered_map<string, string> pages;
    string buf;
    vector<string> files;
    files.push_back("index.html");
    files.push_back("Mine.html");
    files.push_back("failure.html");
    for(auto &p: files) {
        if(!strstr(p.c_str(), ".html")) {
            continue;
        }

        ifstream file("./pages/" + p);
        if(file.is_open()) {
            string res;
            while(getline(file, buf)) {
                res = res + buf;
            }
            file.close();            
            pages[p] = res;   
        }
    }
    return pages;
}

/* ***********************************/
/* Not used but helpful functions */
/* ***********************************/

/*Launch a task in the thread pool*/
void t_start(vector<thread> &threads, std::function<void()>&& job) {
    // find an ended thread
    for(auto&& thread: threads)
    {
        if(thread.joinable()) // still running or waiting to join
            continue;

        thread = std::thread(job);
        return;
    }

    // if not wait for one
    for(auto&& thread: threads)
    {
        if(!thread.joinable()) // dead thread (not run or already joined)
            continue;

        thread.join();
        thread = std::thread(job);
        return;
    }
}

int read_mes(int fd) {
    int i = 0;
    char buf[BUFSIZE];
    while((i = read(fd, buf, BUFSIZE)) > 0) {
        printf("%s", buf);
    }
    if (i < 0) {
        error("Failed to read message");
        return -1;
    }
    return 0;
}