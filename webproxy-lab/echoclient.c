#include "csapp.h"

int main(int argc, char **argv)
{
    int clientfd;                    // 커널에서 매핑시킨 파일의 인덱스 선언
    char *host, *port, buf[MAXLINE]; // ip, port, buf사이즈 정의
    rio_t rio;                       // roburst io 선언

    if (argc != 3)
    { // 매개변수가 다 안들어오면 오류
        fprintf(stderr, "usage: %s <host> <port>\n", argv[0]);
    }
    host = argv[1]; // 매개변수로 입력한 ip
    port = argv[2]; // 매개변수로 입력한 포트

    clientfd = Open_clientfd(host, port); // wrapper함수로 fd 받기
    Rio_readinitb(&rio, clientfd);

    while (Fgets(buf, MAXLINE, stdin) != NULL)
    {
        Rio_writen(clientfd, buf, strlen(buf));
        Rio_readlineb(&rio, buf, MAXLINE);
        Fputs(buf, stdout);
    }
    Close(clientfd);
    exit(0);
}