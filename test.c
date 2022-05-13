#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>

int extract_message(char **buf, char **msg)
{
    char	*newbuf;
    int	i;

    *msg = 0;
    if (*buf == 0)
        return (0);
    i = 0;
    while ((*buf)[i])
    {
        if ((*buf)[i] == '\n')
        {
            newbuf = calloc(1, sizeof(*newbuf) * (strlen(*buf + i + 1) + 1));
            if (newbuf == 0)
                return (-1);
            strcpy(newbuf, *buf + i + 1);
            *msg = *buf;
            (*msg)[i + 1] = 0;
            *buf = newbuf;
            return (1);
        }
        i++;
    }
    return (0);
}

char *str_join(char *buf, char *add)
{
    char	*newbuf;
    int		len;

    if (buf == 0)
        len = 0;
    else
        len = strlen(buf);
    newbuf = malloc(sizeof(*newbuf) * (len + strlen(add) + 1));
    if (newbuf == 0)
        return (0);
    newbuf[0] = 0;
    if (buf != 0)
        strcat(newbuf, buf);
    free(buf);
    strcat(newbuf, add);
    return (newbuf);
}

int sockfd, maxfd, connfd;
fd_set fds, fdread, fdwrite;
struct sockaddr_in serv, cli;
char buffer[1024];
char *msg[65000];
char user[65000];
char send_info[50];
socklen_t len;
int client = 0;

void send_all (char *str, int from) {
    for (int fd = 0; fd < maxfd; fd++){
        if (FD_ISSET(fd, &fdwrite) && fd != from)
            send(fd, str, strlen(str), 0);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2)
        exit(1);

    // socket create and verification
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        printf("socket creation failed...\n");
        exit(0);
    }
    else
        printf("Socket successfully created..\n");

    bzero(&serv, sizeof(serv));

    // assign IP, PORT
    serv.sin_family = AF_INET;
    serv.sin_addr.s_addr = htonl(2130706433); //127.0.0.1
    serv.sin_port = htons(atoi(argv[1]));

    // Binding newly created socket to given IP and verification
    if ((bind(sockfd, (const struct sockaddr *)&serv, sizeof(serv))) != 0) {
        printf("socket bind failed...\n");
        exit(0);
    }
    else
        printf("Socket successfully binded..\n");
    if (listen(sockfd, 1024) != 0) {
        printf("cannot listen\n");
        exit(0);
    }
    FD_ZERO(&fds);
    FD_SET(sockfd, &fds);
    maxfd = sockfd;

    while(1) {
        fdread = fdwrite = fds;

        select(maxfd+1, &fdread, &fdwrite, 0, 0);

        for (int fd = 0; fd <= maxfd; fd++) {
            if (!FD_ISSET(fd, &fdread)) {
                continue;
            }
            if (fd == sockfd) {
                len = sizeof(cli);
                connfd = accept(sockfd, (struct sockaddr *)&cli, &len);
                if (connfd < 0){
                    exit(1);
                }
                if (maxfd < connfd)
                    maxfd = connfd;
                user[connfd] = client;
                client++;
                msg[connfd] = NULL;
                sprintf(send_info, "server: client %d just arrived\n", user[connfd]);
                send_all(send_info, connfd );
                FD_SET(connfd, &fds);
                break;
            } else {
                int ret = recv(fd, buffer, 1024, 0);
                if (ret <= 0) {
                    sprintf(send_info, "server: client %d just left\n", user[fd]);
                    send_all(send_info, fd);
                    free(msg[fd]);
                    msg[fd] = NULL;
                    close(fd);
                    FD_CLR(fd, &fds);
                }
                buffer[ret] = '\0';
                msg[fd] = str_join(msg[fd], buffer);
                char *bu = NULL;
                while (extract_message(&msg[fd], &bu)){
                    sprintf(send_info, "client %d: ", user[fd]);
                    send_all(send_info, fd);
                    send_all(bu, fd);
                    free(bu);
                    bu = NULL;
                }
            }
        }
    }
    return 0;
}