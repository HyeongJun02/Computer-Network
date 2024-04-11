#include "../Common.h"

char *SERVERIP = (char *)"127.0.0.1";
#define SERVERPORT 9000
#define BUFSIZE    512

// 수신 스레드 함수
void *receive_func(void *arg) {
    int sock = *((int *)arg);
    char buf[BUFSIZE + 1];
    struct sockaddr_in serveraddr;
    socklen_t addrlen = sizeof(serveraddr);
    int retval;

    while (1) {
        retval = recvfrom(sock, buf, BUFSIZE, 0, (struct sockaddr *)&serveraddr, &addrlen);
        if (retval == SOCKET_ERROR) {
            err_display("recvfrom()");
            break;
        }

        buf[retval] = '\0';
        printf("- server[%s:%d]: %s\n", inet_ntoa(serveraddr.sin_addr), ntohs(serveraddr.sin_port), buf);
    }

    pthread_exit(NULL);
}

// 전송 스레드 함수
void *send_func(void *arg) {
    int sock = *((int *)arg);
    char buf[BUFSIZE + 1];
    int retval;
    struct sockaddr_in serveraddr;

    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    inet_pton(AF_INET, SERVERIP, &serveraddr.sin_addr);
    serveraddr.sin_port = htons(SERVERPORT);

    while (1) {
        // printf("\n[보낼 데이터] ");
        if (fgets(buf, BUFSIZE + 1, stdin) == NULL)
            break;

        int len = (int)strlen(buf);
        if (buf[len - 1] == '\n')
            buf[len - 1] = '\0';

        if (strlen(buf) == 0)
            break;

        retval = sendto(sock, buf, len, 0, (struct sockaddr *)&serveraddr, sizeof(serveraddr));
        if (retval == SOCKET_ERROR) {
            err_display("sendto()");
            break;
        }

        // printf("[UDP 클라이언트] %d바이트를 보냈습니다.\n", retval);
    }

    pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
    if (argc > 1) {
        SERVERIP = argv[1];
    }

    // UDP 소켓 생성
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == INVALID_SOCKET) {
        err_quit("socket");
    }

    // 수신 및 전송 스레드 생성
    pthread_t r_tid, s_tid;

    if (pthread_create(&r_tid, NULL, receive_func, &sock) != 0) {
        printf("[UDP 클라이언트] 수신 스레드 생성 실패\n");
        close(sock);
        exit(1);
    }

    if (pthread_create(&s_tid, NULL, send_func, &sock) != 0) {
        printf("[UDP 클라이언트] 전송 스레드 생성 실패\n");
        close(sock);
        exit(1);
    }

    pthread_join(s_tid, NULL);

    close(sock);
    return 0;
}
