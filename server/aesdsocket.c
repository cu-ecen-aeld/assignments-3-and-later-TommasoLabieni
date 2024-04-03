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
#include <pthread.h>
#include <sys/time.h>
#include <time.h>
#include <list.h>

#define LISTEN_BACKLOG 10 /* Max number of pending connections*/
#define TIMEOUT_SEC 10
#define OUT_FILENAME "/var/tmp/aesdsocketdata"
#define BUF_SIZE 128

int out_fd;
char s[4], *content = NULL;
int sock_fd;
ssize_t f_length = 0, prev_length = 0;
pthread_mutex_t mux;
List l = NULL;
struct itimerspec ts;
timer_t timer_tid;

void WaitThread()
{
    ListNode *tmp = l;

    pthread_mutex_lock(&mux);
    while (tmp != NULL)
    {
        if ((tmp->data->complete) && !(tmp->data->joined))
        {
            tmp->data->joined = true;
            pthread_join((*tmp->data->tid), NULL);
            close(tmp->data->cfd);
        }
        tmp = tmp->next;
    }
    pthread_mutex_unlock(&mux);
}

static void
sig_handler_f(int sig_num)
{
    syslog(LOG_DEBUG, "Caught signal, exiting\n");

    WaitThread();
    close(out_fd);
    close(sock_fd);
    free(content);
    FreeList(&l);
    closelog();

    exit(EXIT_SUCCESS);
}

static void timer_handler_f(union sigval signum)
{
    struct timeval cur_time;
    char tmbuf[64], buf[80];

    if (gettimeofday(&cur_time, NULL) != 0)
    {
        syslog(LOG_ERR, "Error getting curime. Errno message is: \n%s\n", strerror(errno));
        exit(-1);
    }

    struct tm *nowtm = localtime(&cur_time.tv_sec);

    strftime(tmbuf, sizeof tmbuf, "%Y-%m-%d-%H:%M:%S", nowtm);

    snprintf(buf, sizeof buf, "timestamp:%s\n", tmbuf);

    pthread_mutex_lock(&mux);

    bool last_char = false;

    f_length += strlen(buf);
    content = realloc(content, f_length + 1); /* Allocating 1 B more for terminating char */
    if (content == NULL)
    {
        pthread_mutex_unlock(&mux);
        syslog(LOG_ERR, "Error while reallocating new memory. Errno message is: \n%s\n", strerror(errno));
        exit(-1);
    }

    for (size_t i = prev_length, j = 0; j < strlen(buf); i++, j++)
    {
        content[i] = buf[j];
        last_char = (buf[j] == '\n');
    }

    content[f_length] = 0;
    prev_length = f_length;

    size_t written_chars = write(out_fd, buf, strlen(buf));

    if (written_chars != strlen(buf))
    {
        pthread_mutex_unlock(&mux);
        syslog(LOG_ERR, "Error while writing to file. Errno message is: \n%s\n", strerror(errno));
        exit(-1);
    }

    pthread_mutex_unlock(&mux);

    if (timer_settime(timer_tid, 0, &ts, NULL) == -1)
    {
        syslog(LOG_ERR, "Error while setting timer. Errno message is: \n%s\n", strerror(errno));
        exit(-1);
    }
}

void *handle_connection(void *arg)
{
    ThreadData *t_data = (ThreadData *)arg;
    pthread_t tid = (*t_data->tid);
    ssize_t nread;
    char buf[BUF_SIZE];
    bool last_char = false;

    for (;;)
    {
        nread = recv(t_data->cfd, buf, BUF_SIZE, 0);

        if (nread > 0)
        {
            pthread_mutex_lock(&mux);
            f_length += nread;

            content = realloc(content, f_length + 1); /* Allocating 1 B more for terminating char */
            if (content == NULL)
            {
                pthread_mutex_unlock(&mux);
                syslog(LOG_ERR, "Error while reallocating new memory. Errno message is: \n%s\n", strerror(errno));
                exit(-1);
            }

            for (size_t i = prev_length, j = 0; j < nread; i++, j++)
            {
                content[i] = buf[j];
                last_char = (buf[j] == '\n');
            }

            content[f_length] = 0;
            prev_length = f_length;

            size_t written_chars = write(out_fd, buf, nread);

            if (written_chars != nread)
            {
                pthread_mutex_unlock(&mux);
                syslog(LOG_ERR, "Error while writing to file. Errno message is: \n%s\n", strerror(errno));
                exit(-1);
            }

            /* Return full content to client */
            if (last_char)
            {
                if (send(t_data->cfd, content, f_length, 0) != f_length)
                {
                    pthread_mutex_unlock(&mux);
                    syslog(LOG_ERR, "Error while writing back to socket. Errno message is: \n%s\n", strerror(errno));
                    exit(-1);
                }
                pthread_mutex_unlock(&mux);
            }

            /* Clear buf */
            memset(buf, 0, BUF_SIZE);
        }
        else
        {
            pthread_mutex_lock(&mux);
            t_data->complete = true;
            pthread_mutex_unlock(&mux);
            return NULL;
        }
    }
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

    hints.ai_family = AF_INET;       /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_STREAM; /* STREAM socket */
    hints.ai_flags = AI_PASSIVE;     /* For wildcard IP address */
    hints.ai_protocol = 0;           /* Any protocol */
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

    out_fd = open(OUT_FILENAME, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (out_fd == -1)
    {
        syslog(LOG_ERR, "Error while opening out file. Errno message is: \n%s\n", strerror(errno));
        closelog();
        close(sock_fd);
        exit(-1);
    }

    struct sigevent sev;

    sev.sigev_notify = SIGEV_THREAD;             /* Notify via thread */
    sev.sigev_notify_function = timer_handler_f; /* Thread start function */
    sev.sigev_notify_attributes = NULL;
    sev.sigev_value.sival_ptr = NULL;

    if (timer_create(CLOCK_MONOTONIC, &sev, &timer_tid) == -1)
    {
        syslog(LOG_ERR, "Error while creating timer. Errno message is: \n%s\n", strerror(errno));
        closelog();
        close(sock_fd);
        exit(-1);
    }

    printf("Timer ID: %ld\n", (long)timer_tid);
    ts.it_value.tv_sec = TIMEOUT_SEC;
    ts.it_value.tv_nsec = 1e6;

    if (timer_settime(timer_tid, 0, &ts, NULL) == -1)
    {
        syslog(LOG_ERR, "Error while setting timer. Errno message is: \n%s\n", strerror(errno));
        closelog();
        close(sock_fd);
        exit(-1);
    }

    socklen_t peer_addr_size = sizeof(struct sockaddr);
    struct sockaddr peer_addr;
    int cfd;

    for (;;)
    {
        cfd = accept(sock_fd, (struct sockaddr *)&peer_addr,
                     &peer_addr_size);
        if (cfd == -1)
        {
            syslog(LOG_ERR, "Error while accepting. Errno message is: \n%s\n", strerror(errno));
            closelog();
            close(sock_fd);
            close(out_fd);
            FreeList(&l);
            exit(-1);
        }

        struct sockaddr_in *sin = (struct sockaddr_in *)&peer_addr;
        inet_ntop(AF_INET, &(sin->sin_addr),
                  s, peer_addr_size);

        syslog(LOG_DEBUG, "Accepted connection from %s\n", s);

        ThreadData *t_data = (ThreadData *)malloc(sizeof(ThreadData));
        if (t_data == NULL)
        {
            syslog(LOG_ERR, "Error while allocating space for thread data. Errno message is: \n%s\n", strerror(errno));
            closelog();
            close(sock_fd);
            close(out_fd);
            FreeList(&l);
            exit(-1);
        }

        t_data->tid = malloc(sizeof(pthread_t));
        t_data->cfd = cfd;
        t_data->complete = false;
        t_data->joined = false;

        if (t_data->tid == NULL)
        {
            syslog(LOG_ERR, "Error while allocating space for thread ID. Errno message is: \n%s\n", strerror(errno));
            closelog();
            close(sock_fd);
            close(out_fd);
            free(t_data);
            FreeList(&l);
            exit(-1);
        }

        if (pthread_create(t_data->tid, NULL, handle_connection, (void *)t_data) != 0)
        {
            syslog(LOG_ERR, "Error while spawning thread. Errno message is: \n%s\n", strerror(errno));
            closelog();
            close(sock_fd);
            close(out_fd);
            free(t_data->tid);
            free(t_data);
            FreeList(&l);
            exit(-1);
        }

        ListNode *node = CreateNode(t_data);
        if (node == NULL)
        {
            syslog(LOG_ERR, "Error while spawning thread. Errno message is: \n%s\n", strerror(errno));
            closelog();
            close(sock_fd);
            close(out_fd);
            free(t_data->tid);
            free(t_data);
            FreeList(&l);
            exit(-1);
        }

        if (l == NULL)
        {
            l = node;
        }
        else
        {
            AppendNode(&l, node);
        }

        WaitThread();
    }
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

    if (pthread_mutex_init(&mux, NULL) != 0)
    {
        syslog(LOG_ERR, "Error while initializing Mutex. Errno message is: \n%s\n", strerror(errno));
        closelog();
        exit(-1);
    }

    pid_t sock_pid = -1;
    if (daemon_mode)
    {
        sock_pid = fork();

        if (sock_pid == -1)
        {
            syslog(LOG_ERR, "Error while creating child process. Errno message is: \n%s\n", strerror(errno));
            closelog();
            exit(-1);
        }
    }
    if ((sock_pid == 0) || (!daemon_mode))
    {
        execute_server();
    }

    exit(EXIT_SUCCESS);
}
