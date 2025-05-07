#include "csapp.h"
/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

void doit(int fd, char *hostname);
void read_requesthdrs(rio_t *rp);
void parse_uri(char *uri, char *hostname, char *port, char *path);
/* You won't lose style points for including this long line in your code */
// Mozilla/5.0 = Mozilla와 호환 가능하다는 것을 나타내는 토큰
// X11; Linux x86_64 = 브라우저가 작동하는 네이티브 플랫폼
// rv:10.0.3 = Gecko의 릴리즈 버전
// Gecko/20120305 = 브라우저가 Gecko기반임을 나타낸다. Gecko = 모질라 재단에서 개발한 레이아웃 엔진
// Firefox/10.0.3 = Firefox 버전
static const char *user_agent_hdr =
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 "
    "Firefox/10.0.3\r\n";

int main(int argc, char **argv)
{
  int listenfd, connfd, proxyfd;
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;
  printf("%s", user_agent_hdr);
  // 클라이언트의 요청을 받기
  if (argc != 2)
  {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }
  listenfd = Open_listenfd(argv[1]); // 해당 포트로 대기 소켓 생성

  while (1)
  {
    clientlen = sizeof(clientaddr);
    connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
    printf("Accepted connection from (%s, %s)\n", hostname, port);
    doit(connfd, hostname);
    Close(connfd);
  }
}

void doit(int fd, char *hostname)
{
  int proxyfd;
  struct stat sbuf;
  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
  char filename[MAXLINE], cgiargs[MAXLINE], parsed_uri[MAXLINE];
  char port[MAXLINE];
  rio_t rio_client, rio_server;
  int is_head = 0; // is_head = 1 ? method = HEAD

  Rio_readinitb(&rio_client, fd);
  Rio_readlineb(&rio_client, buf, MAXLINE);
  printf("Request headers:\n");
  sscanf(buf, "%s %s %s", method, uri, version);

  // 요청들어온 uri 파싱
  parse_uri(uri, hostname, port, parsed_uri);
  // tiny 서버에 파싱한 문자열 요청
  proxyfd = Open_clientfd(hostname, port);
  Rio_readinitb(&rio_server, proxyfd);
  read_requesthdrs(&rio_client);

  write_proxyhdrs(proxyfd, hostname, parsed_uri);

  // tiny server에서 받아온 응답을 읽어서 clientfd에 써주기
  int len;
  while (len = Rio_readlineb(&rio_server, buf, MAXLINE))
  {
    Rio_writen(fd, buf, len);
  }

  Close(proxyfd);
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

void read_requesthdrs(rio_t *rp)
{
  char buf[MAXLINE];

  Rio_readlineb(rp, buf, MAXLINE);
  while (strcmp(buf, "\r\n"))
  {
    Rio_readlineb(rp, buf, MAXLINE);
    printf("%s", buf);
  }
  return;
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