#include "csapp.h"

void echo(int connfd);
    
int main(int argc, char **argv) {
    int listenfd, connfd;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    char client_hostname[MAXLINE], client_port[MAXLINE];

    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(0);
    }

    // 1. 리스닝 소켓 생성 및 준비
    listenfd = Open_listenfd(argv[1]);

    // 2. 무한 루프: 클라이언트 연결 계속 수락
    while (1) {
        clientlen = sizeof(struct sockaddr_storage);
        // 3. 클라이언트 연결 수락
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);

        // 클라이언트 정보 출력 (옵션)
        Getnameinfo((SA *) &clientaddr, clientlen, client_hostname, MAXLINE, client_port, MAXLINE, 0);
        printf("Connected to (%s, %s)\n", client_hostname, client_port);

        // 4. 에코 로직 실행
        echo(connfd);

        // 5. 통신 종료 후 연결 소켓 닫기
        Close(connfd);
    }
    exit(0);
}