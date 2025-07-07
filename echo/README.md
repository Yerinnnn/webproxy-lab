# Echo 서버 실습

TCP 소켓을 이용한 간단한 에코 서버 예제

## 📁 파일 설명

- `echo.c`: 클라이언트로부터 받은 데이터를 그대로 돌려주는 echo 함수 구현
- `echoserveri.c`: 반복적으로 클라이언트를 처리하는 에코 서버 (Iterative 방식)
- `echoclient.c`: 에코 서버에 연결하여 메시지를 보내고 응답을 받는 클라이언트
- `Makefile`: 위의 파일들을 컴파일하고 실행 파일을 생성하기 위한 Makefile

## 🛠️ 컴파일 방법

```bash
make

