#include "../Common.h"
#include <pthread.h>

#define SERVERPORT 9000
#define BUFSIZE    512

// 서버로부터 메시지를 수신하는 함수
void *receive_func(void *arg) {
    int sock = *((int *)arg);
    char buf[BUFSIZE + 1];
    int retval;

    while (1) {
        // 데이터 받기
        retval = recv(sock, buf, BUFSIZE, 0);
        if (retval == SOCKET_ERROR) {
            err_display("recv()");
            break;
        } else if (retval == 0) {
            printf("- Server: This host left this chatting room.\n");
            break;
        }

        // 받은 데이터 출력
        buf[retval] = '\0';
        // printf("[TCP 클라이언트] %d바이트를 받았습니다.\n", retval);
        printf("- Server: %s\n", buf);
    }

    close(sock);
    pthread_exit(NULL);
}

// 서버로 메시지를 보내는 함수
void *send_func(void *arg) {
    int sock = *((int *)arg);
    char buf[BUFSIZE + 1];
    int retval;

    while (1) {
        if (fgets(buf, BUFSIZE + 1, stdin) == NULL)
            break;

        int len = (int)strlen(buf);
        if (buf[len - 1] == '\n')
            buf[len - 1] = '\0';
        if (strlen(buf) == 0)
            break;

        retval = send(sock, buf, (int)strlen(buf), 0);
        if (retval == SOCKET_ERROR) {
            err_display("send()");
            break;
        }
    }

    close(sock);
    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    int retval;

    // 명령행 인수가 있으면 IP 주소로 사용
    char *SERVERIP = (argc > 1) ? argv[1] : (char *)"127.0.0.1";

    // 소켓 생성
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) err_quit("socket()");

    // connect()
    struct sockaddr_in serveraddr;
    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    inet_pton(AF_INET, SERVERIP, &serveraddr.sin_addr);
    serveraddr.sin_port = htons(SERVERPORT);

    retval = connect(sock, (struct sockaddr *)&serveraddr, sizeof(serveraddr));
    if (retval == SOCKET_ERROR) err_quit("connect()");

    pthread_t r_tid, s_tid;

    // 서버로부터 메시지를 수신하는 스레드 생성
    if (pthread_create(&r_tid, NULL, receive_func, &sock) != 0) {
        printf("[TCP 클라이언트] 수신 스레드 생성 실패\n");
        close(sock);
        exit(1);
    }

    // 서버로 메시지를 보내는 스레드 생성
    if (pthread_create(&s_tid, NULL, send_func, &sock) != 0) {
        printf("[TCP 클라이언트] 전송 스레드 생성 실패\n");
        close(sock);
        exit(1);
    }

    pthread_join(s_tid, NULL);

    // 소켓 닫기
    close(sock);
    return 0;
}
