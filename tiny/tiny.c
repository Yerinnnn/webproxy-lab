/* $begin tinymain */
/*
 * tiny.c - A simple, iterative HTTP/1.0 Web server that uses the
 *     GET method to serve static and dynamic content.
 *
 * Updated 11/2019 droh
 *   - Fixed sprintf() aliasing issue in serve_static(), and clienterror().
 */
#include "csapp.h"

void doit(int fd);
void read_requesthdrs(rio_t *rp);
int parse_uri(char *uri, char *filename, char *cgiargs);
void serve_static(int fd, char *filename, int filesize);
void get_filetype(char *filename, char *filetype);
void serve_dynamic(int fd, char *filename, char *cgiargs);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg,
                 char *longmsg);

/**
 * @brief Tiny Web 서버의 진입점
 *
 * 지정된 포트에서 연결을 수락하고, 각 클라이언트 요청을 반복적으로 처리함
 * 사용 예: ./tiny 8000
 *
 * @param argc 인자 개수 (기대값: 2)
 * @param argv 포트 번호를 포함한 명령행 인자 배열
 * @return int 프로그램 종료 상태 코드
 */
int main(int argc, char **argv)
{
  int listenfd, connfd;                  // 리스닝 소켓 디스크립터, 연결 소켓 디스크립터
  char hostname[MAXLINE], port[MAXLINE]; // 클라이언트의 호스트명, 포트명 저장 버퍼
  socklen_t clientlen;                   // 클라이언트 주소 구조체의 길이
  struct sockaddr_storage clientaddr;    // 클라이언트 소켓 주소 저장 구조체 (IPv4/IPv6 호환)

  /* Check command line args */
  // 명령줄 인자 개수 확인: 포트 번호가 반드시 제공되어야 함
  if (argc != 2)
  {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  // 주어진 포트 번호로 리스닝 소켓을 열고, 클라이언트 연결 요청을 기다릴 준비를 함
  listenfd = Open_listenfd(argv[1]);
  while (1)
  {
    clientlen = sizeof(clientaddr); // 클라이언트 주소 구조체의 크기 초기화
    // 리스닝 소켓(listenfd)을 통해 클라이언트의 연결 요청을 수락하고, 새로운 연결 소켓(connfd)을 생성
    // 클라이언트의 주소 정보는 clientaddr에 저장됨
    connfd = Accept(listenfd, (SA *)&clientaddr,
                    &clientlen); // line:netp:tiny:accept
    // 클라이언트의 소켓 주소(clientaddr)로부터 호스트명과 포트명을 얻어 hostname과 port 버퍼에 저장
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
    // 연결된 클라이언트의 호스트명과 포트 번호를 출력
    // doit 함수를 호출하여 현재 연결(connfd)을 통해 하나의 HTTP 트랜잭션을 처리
    doit(connfd); // line:netp:tiny:doit
    // HTTP 트랜잭션 처리가 끝나면 연결 소켓을 닫음
    Close(connfd); // line:netp:tiny:close
  }
}

/**
 * @brief 클라이언트 요청을 처리하는 메인 함수
 *
 * 하나의 HTTP 트랜잭션을 처리하며, 요청을 읽고 파싱하여 정적 또는 동적 콘텐츠를 서빙
 *
 * @param fd 클라이언트와 연결된 소켓 파일 디스크립터
 */
void doit(int fd)
{
  int is_static;    // 요청이 정적 콘텐츠인지 (1) 동적 콘텐츠인지 (0) 나타내는 플래그
  struct stat sbuf; // 파일 상태 정보를 저장할 구조체
  // HTTP 요청 라인, 메서드, URI, 버전, 파일명, CGI 인자를 저장할 버퍼들
  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
  char filename[MAXLINE], cgiargs[MAXLINE];
  rio_t rio;

  // 소켓 디스크립터(fd)와 rio 스트림을 연결하고 초기화
  Rio_readinitb(&rio, fd);
  // rio 스트림으로부터 HTTP 요청 라인(첫 줄)을 읽어 buf에 저장
  Rio_readlineb(&rio, buf, MAXLINE);
  printf("Request headers:\n");
  printf("%s", buf);

  // 요청 라인에서 메서드, URI, 버전을 파싱
  sscanf(buf, "%s %s %s", method, uri, version);
  // 메서드가 "GET"이 아니면 (대소문자 무시 비교)
  if (strcasecmp(method, "GET") && strcasecmp(method, "HEAD"))
  {
    // 오류 메시지를 클라이언트에게 보냄
    clienterror(fd, method, "501", "Not implemented",
                "Tiny does not implement this method");
    return;
  }
  // 나머지 요청 헤더들을 읽고 무시 (Tiny는 헤더 정보를 사용하지 않음)
  read_requesthdrs(&rio);

  int is_head = !strcasecmp(method, "HEAD");
  if (is_head)
  {
    Rio_writen(fd, buf, strlen(buf));
  }

  // URI를 파싱하여 파일명과 CGI 인자 추출, is_static 플래그 설정
  is_static = parse_uri(uri, filename, cgiargs);
  // 파싱된 파일명(filename)에 해당하는 파일의 상태 정보를 sbuf에 저장
  // 파일이 존재하지 않거나 접근할 수 없으면 -1 반환
  if (stat(filename, &sbuf) < 0)
  {
    // 오류 메시지를 클라이언트에게 보냄
    clienterror(fd, filename, "404", "Not found", "Tiny couldn’t find this file");
    return;
  }

  // 요청이 정적 콘텐츠인 경우
  if (is_static)
  {
    // 파일이 일반 파일이 아니거나 (디렉토리, 파이프 등)
    // 소유자에게 읽기 권한이 없으면
    if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode))
    {
      // "403 Forbidden" 오류 메시지를 클라이언트에게 보냄
      clienterror(fd, filename, "403", "Forbidden",
                  "Tiny couldn’t read the file");
      return; // 함수 종료
    }
    // 정적 콘텐츠를 클라이언트에게 서빙
    serve_static(fd, filename, sbuf.st_size);
  }
  // 요청이 동적 콘텐츠인 경우
  else
  {
    // 파일이 일반 파일이 아니거나
    // 소유자에게 실행 권한이 없으면
    if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode))
    {
      // "403 Forbidden" 오류 메시지를 클라이언트에게 보냄
      clienterror(fd, filename, "403", "Forbidden",
                  "Tiny couldn’t run the CGI program");
      return; // 함수 종료
    }
    // 동적 콘텐츠(CGI 프로그램)를 클라이언트에게 서빙
    serve_dynamic(fd, filename, cgiargs);
  }
}

/**
 * @brief 요청 헤더를 읽고 로그에 출력하는 함수
 *
 * 요청 라인을 처리한 후, 나머지 요청 헤더들을 무시하며 읽어들임
 * 현재는 로그 출력 용도로만 사용되고, 실질적인 처리는 하지 않음
 *
 * @param rp RIO 버퍼 구조체 포인터 (클라이언트 입력 스트림)
 */
void read_requesthdrs(rio_t *rp)
{
  char buf[MAXLINE];

  // 루프 진입 전에 첫 번째 라인을 읽어서 buf를 초기화 함
  Rio_readlineb(rp, buf, MAXLINE);

  // 이제 buf에 유효한 데이터가 있으므로, 안전하게 비교할 수 있음
  while (strcmp(buf, "\r\n"))
  {
    // 현재 buf에 있는 라인은 이미 처리(printf)했거나 조건 검사에 사용되었으므로,
    // 다음 루프를 위해 새로운 라인을 읽어와야 함
    Rio_readlineb(rp, buf, MAXLINE);
    printf("%s", buf);
  }
  return;
}

/**
 * @brief 요청 URI를 분석하여 정적 또는 동적 요청을 구분하고 정보 추출
 *
 * @param uri 클라이언트가 보낸 요청 URI
 * @param filename 정적 파일 또는 CGI 스크립트 경로 (출력)
 * @param cgiargs CGI 프로그램에 전달할 인자 문자열 (출력)
 *
 * @return 1 정적 콘텐츠, 0 동적 콘텐츠
 */
int parse_uri(char *uri, char *filename, char *cgiargs)
{
  char *ptr; // 문자열 검색을 위한 포인터

  // URI에 "cgi-bin" 문자열이 포함되어 있지 않으면 (정적 콘텐츠)
  if (!strstr(uri, "cgi-bin"))
  {
    strcpy(cgiargs, "");   // CGI 인자 문자열을 비움
    strcpy(filename, "."); // 파일명 시작을 현재 디렉토리로 설정 ("./")
    strcat(filename, uri); // URI를 파일명에 추가 (예: "./index.html")
    // URI가 '/' 문자로 끝나면 (예: "http://example.com/images/")
    if (uri[strlen(uri) - 1] == '/')
      strcat(filename, "home.html"); // 기본 파일명 "home.html"을 추가 (예: "./images/home.html")
    return 1;                        // 정적 콘텐츠임을 나타내는 1 반환
  }
  // URI에 "cgi-bin" 문자열이 포함되어 있으면 (동적 콘텐츠)
  else
  {
    ptr = index(uri, '?'); // URI에서 '?' 문자의 첫 번째 등장 위치를 찾음
    if (ptr)
    {                           // '?' 문자가 존재하면 (CGI 인자가 있음)
      strcpy(cgiargs, ptr + 1); // '?' 다음부터를 CGI 인자로 복사
      *ptr = '\0';              // '?' 위치에 널 문자 삽입하여 URI를 파일명 부분과 분리
    }
    else                   // '?' 문자가 없으면 (CGI 인자 없음)
      strcpy(cgiargs, ""); // CGI 인자 문자열을 비움
    strcpy(filename, "."); // 파일명 시작을 현재 디렉토리로 설정
    strcat(filename, uri); // URI의 (수정된) 앞부분을 파일명에 추가 (예: "./cgi-bin/adder")
    return 0;              // 동적 콘텐츠임을 나타내는 0 반환
  }
}

/**
 * @brief 정적 파일 콘텐츠를 클라이언트에게 전송
 *
 * @param fd 클라이언트 소켓 디스크립터
 * @param filename 클라이언트에 전송할 정적 파일 경로
 * @param filesize 전송할 파일 크기 (바이트 단위)
 */
void serve_static(int fd, char *filename, int filesize)
{
  int srcfd;                                  // 원본 파일 디스크립터
  char *srcp, filetype[MAXLINE], buf[MAXBUF]; // 메모리 매핑 포인터, 파일 유형 버퍼, HTTP 응답 버퍼

  // 파일명으로부터 파일 유형(MIME 타입)을 결정하여 filetype 버퍼에 저장
  get_filetype(filename, filetype);
  sprintf(buf, "HTTP/1.0 200 OK\r\n");
  sprintf(buf, "%sServer: Tiny Web Server\r\n", buf);
  sprintf(buf, "%sConnection: close\r\n", buf);
  sprintf(buf, "%sContent-length: %d\r\n", buf, filesize);
  sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype);
  // 완성된 HTTP 응답 헤더를 클라이언트에게 전송
  Rio_writen(fd, buf, strlen(buf));
  printf("Response headers:\n");
  printf("%s", buf);

  srcfd = Open(filename, O_RDONLY, 0);

  // 1. 메모리 동적 할당 (malloc)
  // 파일 내용을 담을 buffer를 heap에 생성
  char *srcbuf = malloc(filesize);

  // 2. 파일 디스크립터로부터 filesize만큼 읽기
  // rio_readn: 낮은 수준의 I/O 읽기 (파일 → 메모리)
  Rio_readn(srcfd, srcbuf, filesize);

  // 3. 클라이언트에게 전송 (소켓 디스크립터로 쓰기)
  // rio_writen: 낮은 수준의 I/O 쓰기 (메모리 → 소켓)
  Rio_writen(fd, srcbuf, filesize);

  // 4. 사용한 메모리 해제 및 파일 닫기
  free(srcbuf);
  Close(srcfd);
}

/**
 * @brief 파일 확장자에 따라 MIME 타입 문자열을 결정
 *
 * @param filename 검사할 파일 이름
 * @param filetype MIME 타입 문자열 (출력)
 */
void get_filetype(char *filename, char *filetype)
{
  if (strstr(filename, ".html"))
    strcpy(filetype, "text/html");
  else if (strstr(filename, ".gif"))
    strcpy(filetype, "image/gif");
  else if (strstr(filename, ".png"))
    strcpy(filetype, "image/png");
  else if (strstr(filename, ".jpg"))
    strcpy(filetype, "image/jpeg");
  else if (strstr(filename, ".mp4"))
    strcpy(filetype, "video/mp4");
  else
    strcpy(filetype, "text/plain");
}

/**
 * @brief CGI 프로그램을 실행하여 동적 콘텐츠를 클라이언트에게 전송
 *
 * @param fd 클라이언트 소켓 디스크립터
 * @param filename 실행할 CGI 프로그램 경로
 * @param cgiargs CGI 프로그램에 전달할 인자 문자열
 */
void serve_dynamic(int fd, char *filename, char *cgiargs)
{
  char buf[MAXLINE], *emptylist[] = {NULL}; // HTTP 응답 버퍼, execve 인자 리스트s

  // HTTP 응답 라인 생성 (예: HTTP/1.0 200 OK)
  sprintf(buf, "HTTP/1.0 200 OK\r\n");
  // 응답 라인을 클라이언트에게 전송
  Rio_writen(fd, buf, strlen(buf));
  // 서버 정보 헤더 추가
  sprintf(buf, "Server: Tiny Web Server\r\n");
  // 서버 정보 헤더를 클라이언트에게 전송
  Rio_writen(fd, buf, strlen(buf));
  // 이 시점 이후의 응답 (Content-type, Content-length, 본문 등)은 CGI 프로그램이 책임짐

  // 자식 프로세스 생성 (Fork() == 0 이면 자식 프로세스)
  if (Fork() == 0)
  { /* Child */
    // QUERY_STRING 환경 변수를 CGI 인자(cgiargs)로 설정 (이것이 CGI 프로그램으로 전달됨)
    setenv("QUERY_STRING", cgiargs, 1); // 1은 이미 존재하는 경우 덮어씀을 의미
    // 자식 프로세스의 표준 출력(STDOUT_FILENO)을 클라이언트 소켓(fd)으로 리디렉션
    // 이제 CGI 프로그램이 표준 출력에 쓰는 모든 내용은 클라이언트에게 직접 전송됨
    Dup2(fd, STDOUT_FILENO);
    // CGI 프로그램을 로드하고 실행
    // filename: 실행할 CGI 프로그램의 경로
    // emptylist: argv (인자 리스트, 이 경우 비어 있음)
    // environ: 현재 환경 변수 (QUERY_STRING 포함)
    Execve(filename, emptylist, environ);
  }
  // 부모 프로세스: 자식 프로세스가 종료될 때까지 대기하고 회수 (좀비 프로세스 방지)
  // 여러 요청이 들어오면 처리할 수 없음
  Wait(NULL);
}

/**
 * @brief HTTP 오류를 클라이언트에게 HTML 형식으로 전송
 *
 * @param fd 클라이언트 소켓 디스크립터
 * @param cause 오류를 유발한 원인 문자열
 * @param errnum HTTP 상태 코드 문자열 (예: "404")
 * @param shortmsg 간단한 오류 메시지 (예: "Not Found")
 * @param longmsg 상세 오류 메시지 (예: "Tiny couldn't find this file")
 */
void clienterror(int fd, char *cause, char *errnum,
                 char *shortmsg, char *longmsg)
{
  char buf[MAXLINE], body[MAXBUF]; // HTTP 응답 헤더 및 본문을 위한 버퍼

  // HTML 응답 본문 시작
  sprintf(body, "<html><title>Tiny Error</title>");
  sprintf(body, "%s<body bgcolor=\"ffffff\">\r\n", body);        // 배경색 지정
  sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);         // 오류 번호와 짧은 메시지 추가
  sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);        // 긴 메시지와 원인 추가
  sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body); // 푸터 추가

  // HTTP 응답 라인 생성 (예: HTTP/1.0 404 Not Found)
  sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
  // 응답 라인을 클라이언트에게 전송
  Rio_writen(fd, buf, strlen(buf));
  // Content-type 헤더 생성 및 전송 (HTML 파일임을 알림)
  sprintf(buf, "Content-type: text/html\r\n");
  Rio_writen(fd, buf, strlen(buf));
  // Content-length 헤더 생성 및 전송 (본문 길이 알림)
  sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body)); // 헤더 끝을 나타내는 빈 줄 포함
  Rio_writen(fd, buf, strlen(buf));
  // 완성된 HTTP 응답 본문을 클라이언트에게 전송
  Rio_writen(fd, body, strlen(body));
}
