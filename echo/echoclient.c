#include "csapp.h"

int main(int argc, char **argv) {
    int clientfd;  // 서버와의 연결을 나타내는 소켓 파일 디스크립터를 저장
    char *host, *port, buf[MAXLINE];
    rio_t rio;

    if (argc != 3) {
        fprintf(stderr, "usage: %s <host> <port>\n", argv[0]);
        exit(0);
    }

    host = argv[1];
    port = argv[2];

    clientfd = Open_clientfd(host, port);  // 클라이언트가 특정 서버에 연결하기 위한 소켓 디스크립터를 생성하고 연결을 시도
    Rio_readinitb(&rio, clientfd);

    while (Fgets(buf, MAXLINE, stdin) != NULL) {
        Rio_writen(clientfd, buf, strlen(buf));  // buf에 저장된 모든 바이트를소켓(clientfd)으로 안전하게 전송
        Rio_readlineb(&rio, buf, MAXLINE);  // rio 스트림에 연결된 소켓(clientfd)으로부터 데이터를 읽어, 첫 번째 개행 문자(\n)를 만날 때까지 또는 MAXLINE 바이트를 읽을 때까지 buf에 저장
        Fputs(buf, stdout);
    }
    Close(clientfd);
    exit(0);
}