#include <syslog.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>

#define LISTEN_BACKLOG 10 /* Max number of pending connections*/
#define OUT_FILENAME "/var/tmp/aesdsocketdata"
#define BUF_SIZE 128

int out_fd;
char *content = NULL;
char *s = NULL;
int sock_fd, cfd;

static void
sig_handler_f(int sig_num)
{
    syslog(LOG_DEBUG, "Caught signal, exiting\n");

    if (content != NULL)
        free(content);
    if (s != NULL)
        free(s);
    close(out_fd);
    close(sock_fd);
    close(cfd);
    closelog();

    exit(EXIT_SUCCESS);
}

void execute_server()
{
    struct addrinfo hints;
    struct addrinfo *result;
    struct sigaction new_act;

    memset(&new_act, 0, sizeof(struct sigaction));
    new_act.sa_handler = sig_handler_f;

    if (sigaction(SIGINT, &new_act, NULL) != 0)
    {
        syslog(LOG_ERR, "Error setting SIGINT handler. Errno message is:  \n%s\n", strerror(errno));
        closelog();
        close(sock_fd);
        exit(-1);
    }
    if (sigaction(SIGTERM, &new_act, NULL) != 0)
    {
        syslog(LOG_ERR, "Error setting SIGTERM handler. Errno message is:  \n%s\n", strerror(errno));
        closelog();
        close(sock_fd);
        exit(-1);
    }

    memset(&hints, 0, sizeof(struct addrinfo));

    hints.ai_family = AF_INET;      /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_DGRAM; /* Datagram socket */
    hints.ai_flags = AI_PASSIVE;    /* For wildcard IP address */
    hints.ai_protocol = 0;          /* Any protocol */
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;

    if (getaddrinfo(NULL, "9000", &hints, &result) != 0)
    {
        syslog(LOG_ERR, "Error while getaddrinfo. Errno message is:  \n%s\n", strerror(errno));
        closelog();
        close(sock_fd);
        exit(-1);
    }

    if (bind(sock_fd, result->ai_addr, result->ai_addrlen) != 0)
    {
        syslog(LOG_ERR, "Error while binding socket. Errno message is: \n%s\n", strerror(errno));
        closelog();
        close(sock_fd);
        exit(-1);
    }

    freeaddrinfo(result);

    if (listen(sock_fd, LISTEN_BACKLOG) == -1)
    {
        syslog(LOG_ERR, "Error while listening. Errno message is: \n%s\n", strerror(errno));
        closelog();
        close(sock_fd);
        exit(-1);
    }

    socklen_t peer_addr_size = sizeof(struct sockaddr);
    struct sockaddr peer_addr;

    cfd = accept(sock_fd, (struct sockaddr *)&peer_addr,
                 &peer_addr_size);
    if (cfd == -1)
    {
        syslog(LOG_ERR, "Error while accepting. Errno message is: \n%s\n", strerror(errno));
        closelog();
        close(sock_fd);
        exit(-1);
    }
    s = malloc(peer_addr_size);

    struct sockaddr_in *sin = (struct sockaddr_in *)&peer_addr;
    inet_ntop(AF_INET, &(sin->sin_addr),
              s, peer_addr_size);

    syslog(LOG_DEBUG, "Accepted connection from %s\n", s);

    int out_fd = open(OUT_FILENAME, O_WRONLY | O_CREAT | O_TRUNC, 0644);

    if (out_fd == -1)
    {
        syslog(LOG_ERR, "Error while opening out file. Errno message is: \n%s\n", strerror(errno));
        free(s);
        closelog();
        close(sock_fd);
        exit(-1);
    }

    ssize_t nread, f_length = 0, prev_length = 0;
    char buf[BUF_SIZE];
    bool last_char = false;

    for (;;)
    {
        nread = recv(cfd, buf, BUF_SIZE, 0);

        if (nread > 0)
        {
            f_length += nread;

            content = realloc(content, f_length + 1); /* Allocating 1 B more for terminating char */
            if (content == NULL)
            {
                syslog(LOG_ERR, "Error while reallocating new memory. Errno message is: \n%s\n", strerror(errno));
                free(s);
                close(out_fd);
                close(sock_fd);
                closelog();
                exit(-1);
            }

            for (size_t i = prev_length, j = 0; j < nread; i++, j++)
            {
                content[i] = buf[j];
                last_char = (buf[j] == '\n');
            }

            content[f_length] = 0;
            prev_length = f_length;

            write(out_fd, buf, nread);

            /* Return full content to client */
            if (last_char)
            {
                if (send(cfd, content, f_length, 0) != f_length)
                {
                    syslog(LOG_ERR, "Error while writing back to socket. Errno message is: \n%s\n", strerror(errno));
                    free(content);
                    free(s);
                    close(out_fd);
                    close(sock_fd);
                    closelog();
                    exit(-1);
                    last_char = false;
                }
            }

            /* Clear buf */
            memset(buf, 0, BUF_SIZE);
        }
        else
        {

            syslog(LOG_DEBUG, "Closed connection from %s\n", s);
            close(cfd);

            cfd = accept(sock_fd, (struct sockaddr *)&peer_addr,
                         &peer_addr_size);
            if (cfd == -1)
            {
                syslog(LOG_ERR, "Error while accepting. Errno message is: \n%s\n", strerror(errno));
                free(content);
                free(s);
                close(out_fd);
                close(sock_fd);
                closelog();
                exit(-1);
            }

            sin = (struct sockaddr_in *)&peer_addr;
            inet_ntop(AF_INET, &(sin->sin_addr),
                      s, peer_addr_size);

            syslog(LOG_DEBUG, "Accepted connection from %s\n", s);
        }
    }

    free(content);
    free(s);
    close(out_fd);
    close(sock_fd);
    close(cfd);
    closelog();
}

int main(int argc, char *argv[])
{
    int c;
    bool daemon_mode = false;

    while ((c = getopt(argc, argv, "d")) != -1)
    {
        switch (c)
        {
        case 'd':
            daemon_mode = true;
            break;
        case '?':
            return 1;
        default:
            abort();
        }
    }

    openlog(NULL, 0, LOG_USER);

    sock_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (sock_fd == -1)
    {
        syslog(LOG_ERR, "Error while creating socket FD. Errno message is: \n%s\n", strerror(errno));
        closelog();
        exit(-1);
    }

    if (daemon_mode)
    {
        pid_t sock_pid = fork();

        if (sock_pid == -1)
        {
            syslog(LOG_ERR, "Error while creating child process. Errno message is: \n%s\n", strerror(errno));
            closelog();
            exit(-1);
        }
        else if (sock_pid == 0)
        {
            execute_server();
        }
        else
        {
            exit(EXIT_SUCCESS);
        }
    }
    else
    {
        execute_server();
    }
    return 0;
}
