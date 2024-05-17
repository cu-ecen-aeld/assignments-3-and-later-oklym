#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>

#include <unistd.h> //sleep


#define PORT "9000"
#define OK_RESULT 0
#define ERR_RESULT -1

static const char* FILE_NAME = "/var/tmp/aesdsocketdata";

int sock_fd = ERR_RESULT;
int conn_fd = ERR_RESULT;

int data_file = ERR_RESULT;


static void printSysLog(int level, char* msg) {
  printf("%s: %s\n", level == LOG_ERR ? "ERROR" : "INFO", msg);
  openlog("assignment-5-oklym", LOG_PID, LOG_USER);
  syslog(level, "%s", msg);
  closelog();
}


void go_exit(int exit_code) {

    if (data_file != ERR_RESULT) {
        close(data_file);
    }

    if (sock_fd != ERR_RESULT) {
        close(sock_fd);
    }

    if (conn_fd != ERR_RESULT) {
        close(conn_fd);
    }

    if (remove(FILE_NAME) == ERR_RESULT) {
	printSysLog(LOG_DEBUG, "Failed to remove data file");
        exit(EXIT_FAILURE);
    }

    //printf("\n\n>>>>> exit: %d\n\n", exit_code);
    exit(exit_code);
}


static void signal_handler(int signal_number) {
    
    if (signal_number == SIGINT) {
        printSysLog(LOG_DEBUG, "Caught signal, exiting");
        printf(": SIGINT\n");
    } else if (signal_number == SIGTERM) {
        printSysLog(LOG_DEBUG, "Caught signal, exiting");
        printf(": SIGTERM\n");
    }

    go_exit(EXIT_SUCCESS);
}


static int setup_signals(void) {

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = signal_handler;

    if (sigaction(SIGINT, &sa, NULL) != 0) {
	printSysLog(LOG_DEBUG, "Setting up SIGINT");
        return ERR_RESULT;
    }

    if (sigaction(SIGTERM, &sa, NULL) != 0) {
        printSysLog(LOG_DEBUG, "Setting up SIGTERM");
	return ERR_RESULT;
    }

    return OK_RESULT;
}


static int open_socket(void) {

    char msg[256];
    int yes = 1;
    
    struct addrinfo hints;
    struct addrinfo *servinfo = NULL;

    memset(&hints, 0, sizeof hints); // cleanup 'hints' structure

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM; // TCP stream sockets
    hints.ai_flags = AI_PASSIVE;     // bind to all interfaces

    int res = getaddrinfo(NULL, PORT, &hints, &servinfo);
    if (res != 0) {
	sprintf(msg, "getaddrinfo: %s", gai_strerror(res));
        printSysLog(LOG_ERR, msg);
        return ERR_RESULT;
    }

    int sfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
    if (sfd == ERR_RESULT) {
	printSysLog(LOG_ERR, "open socket");
        return ERR_RESULT;
    }

    if (setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == ERR_RESULT) {
	printSysLog(LOG_ERR, "set socket options");
        return ERR_RESULT;
    }

    if (bind(sfd, servinfo->ai_addr, servinfo->ai_addrlen) == ERR_RESULT) {
	printSysLog(LOG_ERR, "bind socket");
        return ERR_RESULT;
    }

    freeaddrinfo(servinfo);

    if (listen(sfd, 1 /* BACKLOG */) == ERR_RESULT) {
        printSysLog(LOG_ERR, "couldn't listen to socket");
	return ERR_RESULT;
    }

    return sfd;
}


static int recive_from_socket(int cfd, char** data, size_t* data_len) {
    const size_t CHUNK_SIZE = 1000;
    size_t buf_len = CHUNK_SIZE;
    *data = malloc(buf_len);
    *data_len = 0;

    while (1) {
        ssize_t count = recv(cfd, &((*data)[*data_len]), buf_len - *data_len, 0);

        if (count == ERR_RESULT) {
            printSysLog(LOG_ERR, "failed to receive part of data");
            return ERR_RESULT;
        }
        else if(count == 0) {
            return OK_RESULT;
	}

	//char msg[256];
	//sprintf(msg, "received %ld", count);
	//printSysLog(LOG_INFO, msg);

        *data_len += count;

        if (memchr(&((*data)[*data_len - count]), '\n', count) == NULL) {
	    //printSysLog(LOG_INFO, "no new line found");

            if (*data_len == buf_len) {
		//printSysLog(LOG_INFO, "extend memory bufer");
                buf_len += CHUNK_SIZE;
                char* new_buf = realloc(*data, buf_len);
                if (new_buf == NULL) {
                    printSysLog(LOG_ERR, "failed to extend buffer");
                    return OK_RESULT;
                }

                *data = new_buf;
            }

            continue;
        }
    
        return OK_RESULT;
    }

    return ERR_RESULT;
}


static int write_file(const char* buf, size_t len) {
    
    int result = OK_RESULT;

    data_file = open(FILE_NAME, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (data_file == ERR_RESULT) {
        printSysLog(LOG_ERR, "failed to open file on write");
        return ERR_RESULT;
    }

    if (write(data_file, buf, len) == ERR_RESULT) {
        printSysLog(LOG_ERR, "failed to write buffer to file");
        result = ERR_RESULT;
    }

    if (close(data_file) == ERR_RESULT) {
        printSysLog(LOG_ERR, "failed to close file after write");
        result = ERR_RESULT;
    }

    return result;
}


static int file_to_socket(int cfd) {
    char buf[1000];
    const size_t buf_len = sizeof(buf) / sizeof(buf[0]);

    int data_file = open(FILE_NAME, O_RDONLY);
    if (data_file == ERR_RESULT) {
	printSysLog(LOG_ERR, "failed to open file on read");
        return ERR_RESULT;
    }

    while (1) {
        int count = read(data_file, buf, buf_len);
        if (count == ERR_RESULT) {
            printSysLog(LOG_ERR, "failed to read data from file");
            if (close(data_file)) {
                printSysLog(LOG_ERR, "failed to close file after read");
            }
            return ERR_RESULT;
        }
        else if (count == 0) {
            if (close(data_file)) {
                printSysLog(LOG_ERR, "failed to close file after read");
	        return ERR_RESULT;
            }
            return OK_RESULT;
        }
        
        if (send(cfd, buf, count, 0) == ERR_RESULT) {
            printSysLog(LOG_ERR, "failed to send data to sokcet");
            if (close(data_file)) {
                printSysLog(LOG_ERR, "failed to close file after read");
            }
            return ERR_RESULT;
        }
    }
}


static int communication_loop(int cfd) {
    while (1) {
        char* data;
        size_t data_len;
        
	int res = recive_from_socket(cfd, &data, &data_len);
        if (res == ERR_RESULT) {
            free(data);
	    printSysLog(LOG_ERR, "failed read data from socket");
            return ERR_RESULT;
        }

        if (data_len == 0) {
	    free(data);
            //printSysLog(LOG_INFO, "read 0 bytes data");
	    if (close(cfd) == ERR_RESULT) {
                printSysLog(LOG_ERR, "failed to close input socket after 0 data");
                return ERR_RESULT;
            }
            return OK_RESULT;
        }

        if (write_file(data, data_len) == ERR_RESULT) {
            free(data);
            printSysLog(LOG_ERR, "failed to write data to file");
            return ERR_RESULT;
        }

        free(data);

        if (file_to_socket(cfd) == ERR_RESULT) {
            printSysLog(LOG_ERR, "failed to send file data to socket");
            return ERR_RESULT;
        }
    }

    return ERR_RESULT;
}


static int listen_loop(int serv_fd) {
	
    char msg[256];

    printSysLog(LOG_INFO, "Start listen loop.");
    
    while (1) {
        struct sockaddr conn_addr;
        memset(&conn_addr, 0, sizeof(conn_addr));
        socklen_t conn_addr_len = 0;

	conn_fd = accept(serv_fd, &conn_addr, &conn_addr_len);
        if (conn_fd == ERR_RESULT) {
	    printSysLog(LOG_ERR, "Accept error.");
            return ERR_RESULT;
        }

	char s[INET_ADDRSTRLEN];
        struct sockaddr_in *addr_in = ((struct sockaddr_in *)&conn_addr);
        inet_ntop(AF_INET, &(addr_in->sin_addr), s, INET_ADDRSTRLEN);

	sprintf(msg, "Accepted connection from %s", s);
        printSysLog(LOG_INFO, msg);
        
	if (communication_loop(conn_fd) == ERR_RESULT) {
	    printSysLog(LOG_ERR, "Communcation loop error.");
            return ERR_RESULT;
        }

	if (close(conn_fd) == ERR_RESULT) {
            //printSysLog(LOG_ERR, "Failed to close connection.");
            //return ERR_RESULT;
        }

        sprintf(msg, "Closed connection from  %s", s);
	printSysLog(LOG_INFO, msg);

	sleep(1);
    }

    return ERR_RESULT;
}


int main (int argc, char* argv[]) {

    if(setup_signals() == ERR_RESULT) {
	go_exit(EXIT_FAILURE);
    }

    sock_fd = open_socket();
    if(sock_fd == ERR_RESULT) {
	go_exit(EXIT_FAILURE);
    }

    listen_loop(sock_fd);

    go_exit(EXIT_SUCCESS);
}
