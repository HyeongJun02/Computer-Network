#include "../Common.h"

// char *IP = (char *)"127.0.0.1";
#define SERVERPORT 9000
#define BUFSIZE    512

#include <pthread.h>

pthread_mutex_t clientaddr_mutex = PTHREAD_MUTEX_INITIALIZER;
struct sockaddr_in clientaddr; // 전역 변수로 선언

void *receive_func(void *arg) {
    int sock = *((int *)arg);
    char buf[BUFSIZE + 1];
    socklen_t addrlen = sizeof(clientaddr);
    int retval;

    while (1) {
        // recvfrom() 호출
        retval = recvfrom(sock, buf, BUFSIZE, 0, (struct sockaddr *)&clientaddr, &addrlen);
        if (retval == SOCKET_ERROR) {
            err_display("recvfrom()");
            break;
        }

        // 받은 데이터 출력
        buf[retval] = '\0';
        printf("- client[%s:%d]: %s\n", inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port), buf);
    }

    pthread_exit(NULL);
}

void *send_func(void *arg) {
    int sock = *((int *)arg);
    char buf[BUFSIZE + 1];
    int retval;

    while (1) {
        // 사용자로부터 데이터 입력 받기
        // printf("\n[보낼 데이터] ");
        if (fgets(buf, BUFSIZE + 1, stdin) == NULL)
            break;

        int len = (int)strlen(buf);
        if (buf[len - 1] == '\n')
            buf[len - 1] = '\0';

        if (strlen(buf) == 0)
            break;

        // 동기화: 전역 변수 clientaddr 사용 시 동기화 처리
        pthread_mutex_lock(&clientaddr_mutex);
        retval = sendto(sock, buf, len, 0, (struct sockaddr *)&clientaddr, sizeof(clientaddr));
        pthread_mutex_unlock(&clientaddr_mutex);

        if (retval == SOCKET_ERROR) {
            err_display("sendto()");
            break;
        }

        // printf("[UDP 서버] %d바이트를 보냈습니다.\n", retval);
    }

    pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
    // 소켓 생성
    SOCKET sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == INVALID_SOCKET)
        err_quit("socket()");

	// bind()
	struct sockaddr_in serveraddr;
	memset(&serveraddr, 0, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(SERVERPORT);

    // 바인딩
    if (bind(sock, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) == SOCKET_ERROR)
        err_quit("bind()");

    // 수신 및 전송 스레드 생성
    pthread_t r_tid, s_tid;

    if (pthread_create(&r_tid, NULL, receive_func, &sock) != 0) {
        printf("[UDP 서버] 수신 스레드 생성 실패\n");
        close(sock);
        exit(1);
    }

    if (pthread_create(&s_tid, NULL, send_func, &sock) != 0) {
        printf("[UDP 서버] 전송 스레드 생성 실패\n");
        close(sock);
        exit(1);
    }

    pthread_join(s_tid, NULL);

    close(sock);
    return 0;
}
