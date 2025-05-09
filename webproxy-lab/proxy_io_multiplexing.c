#include <sys/select.h>
#include "csapp.h"
/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

void doit(int fd);
void read_requesthdrs(rio_t *rp, int fd);
void parse_uri(char *uri, char *hostname, char *port, char *path);
static const char *user_agent_hdr =
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 "
    "Firefox/10.0.3\r\n";

#define MAX_CLIENTS FD_SETSIZE

int main(int argc, char **argv)
{
    int listenfd;
    printf("%s", user_agent_hdr);
    // 클라이언트의 요청을 받기
    if (argc != 2)
    {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }
    listenfd = Open_listenfd(argv[1]);

    int client_fds[MAX_CLIENTS];
    fd_set all_set, read_set;
    int maxfd = listenfd, maxi = -1;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    FD_ZERO(&all_set);
    FD_SET(listenfd, &all_set);
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        client_fds[i] = -1;
    }

    while (1)
    {
        read_set = all_set;
        Select(maxfd + 1, &read_set, NULL, NULL, NULL);

        if (FD_ISSET(listenfd, &read_set))
        {
            clientlen = sizeof(clientaddr);
            int connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
            int i;
            for (i = 0; i < MAX_CLIENTS; i++)
            {
                if (client_fds[i] < 0)
                {
                    client_fds[i] = connfd;
                    break;
                }
            }
            if (i == MAX_CLIENTS)
            {
                Close(connfd);
            }
            else
            {
                FD_SET(connfd, &all_set);
                if (connfd > maxfd)
                    maxfd = connfd;
                if (i > maxi)
                    maxi = i;
            }
        }

        for (int i = 0; i <= maxi; i++)
        {
            int sockfd = client_fds[i];
            if (sockfd >= 0 && FD_ISSET(sockfd, &read_set))
            {
                doit(sockfd);
                Close(sockfd);
                FD_CLR(sockfd, &all_set);
                client_fds[i] = -1;
            }
        }
    }
}

void doit(int fd)
{
    rio_t rio_client;
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char hostname[MAXLINE], port[MAXLINE], parsed_uri[MAXLINE];
    int is_head;
    int proxyfd;
    rio_t rio_server;
    ssize_t n;

    /* 1) 한 번만 초기화 */
    Rio_readinitb(&rio_client, fd);

    /* 2) 클라이언트가 닫을 때까지 반복 */
    while (1)
    {
        /* 2-1) 요청 라인 읽기 */
        if ((n = Rio_readlineb(&rio_client, buf, MAXLINE)) <= 0)
            break; /* EOF 혹은 에러 → 종료 */
        if (strcmp(buf, "\r\n") == 0)
            continue; /* 빈 줄 무시 */

        printf("Request headers:\n");

        /* 2-2) 요청 라인 파싱 */
        sscanf(buf, "%s %s %s", method, uri, version);
        is_head = (strcasecmp(method, "HEAD") == 0);

        /* 2-3) URI → hostname, port, path */
        parse_uri(uri, hostname, port, parsed_uri);

        /* 2-4) 백엔드 연결 */
        proxyfd = Open_clientfd(hostname, port);
        Rio_readinitb(&rio_server, proxyfd);

        /* 2-5) 백엔드로 요청 전송 */
        write_proxyhdrs(proxyfd, hostname, parsed_uri);
        read_requesthdrs(&rio_client, proxyfd);

        /* 2-6) 백엔드 응답 중계 */
        while ((n = Rio_readlineb(&rio_server, buf, MAXLINE)) > 0)
        {
            Rio_writen(fd, buf, n);
        }

        Close(proxyfd);
        /* 이 시점에서 백엔드와의 한 트랜잭션이 끝났으니
           클라이언트 측에 남은 요청이 있으면 계속 루프 */
    }
    /* 클라이언트가 소켓을 닫으면(fd EOF) 이 함수 종료 */
}
void write_proxyhdrs(int fd, char *host, char *path)
{
    char buf[MAXLINE];
    sprintf(buf, "GET %s HTTP/1.0\r\n", path);
    sprintf(buf + strlen(buf), "Host: %s\r\n", host);
    sprintf(buf + strlen(buf), "%s", user_agent_hdr);
    sprintf(buf + strlen(buf), "Connection: close\r\n");
    sprintf(buf + strlen(buf), "Proxy-Connection: close\r\n");
    sprintf(buf + strlen(buf), "\r\n");
    Rio_writen(fd, buf, strlen(buf));
}

void read_requesthdrs(rio_t *rp, int fd)
{
    char buf[MAXLINE];

    Rio_readlineb(rp, buf, MAXLINE);
    while (strcmp(buf, "\r\n"))
    {
        Rio_readlineb(rp, buf, MAXLINE);
        printf("%s", buf);
        if (!strncasecmp(buf, "Connection:", 11) ||
            !strncasecmp(buf, "Proxy-Connection:", 17))
        {
        }
        else
        {
            Rio_writen(fd, buf, strlen(buf));
        }
    }
    Rio_writen(fd, "\r\n", 2);
}
void parse_uri(char *uri, char *hostname, char *port, char *path)
{
    char *hostbegin, *hostend, *portbegin, *pathbegin;
    int hostlen, portlen;

    // 1. Skip "http://" or "https://"
    if ((hostbegin = strstr(uri, "://")) != NULL)
    {
        hostbegin += 3;
    }
    else
    {
        hostbegin = uri;
    }

    // 2. Find end of hostname (':' or '/')
    hostend = strpbrk(hostbegin, ":/");

    if (hostend == NULL)
    {
        strcpy(hostname, hostbegin);
        strcpy(port, "80");
        strcpy(path, "/");
        return;
    }

    hostlen = hostend - hostbegin;
    strncpy(hostname, hostbegin, hostlen);
    hostname[hostlen] = '\0';

    // 3. Check for port
    if (*hostend == ':')
    {
        portbegin = hostend + 1;
        pathbegin = strchr(portbegin, '/');
        if (pathbegin != NULL)
        {
            portlen = pathbegin - portbegin;
            strncpy(port, portbegin, portlen);
            port[portlen] = '\0';
            strcpy(path, pathbegin);
        }
        else
        {
            strcpy(port, portbegin);
            strcpy(path, "/");
        }
    }
    else if (*hostend == '/')
    {
        strcpy(port, "80");
        strcpy(path, hostend);
    }
}