#include "csapp.h"

void echo(int connfd) {
    size_t n;
    char buf[MAXLINE];
    rio_t rio;

    // 1. Rio 스트림 초기화 (connfd에서 읽기 위함)
    Rio_readinitb(&rio, connfd);

    printf("hi\n");

    // 2. 클라이언트로부터 데이터 읽고 다시 쓰기 (반복)
    while((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0) {
        printf("server received %d bytes\n", (int)n);
        Rio_writen(connfd, buf, n);  // 읽은 데이터를 그대로 다시 connfd로 쓰기
    }

    printf("bye\n");
}