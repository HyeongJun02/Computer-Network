#include "../Common.h"
#include <pthread.h>
#include <string.h>

#define BUFSIZE    512
#define SERVERPORT 9000

#define SIZE 5

typedef struct {
    int index;      // 스레드 인덱스
    SOCKET sock;    // 소켓 정보
} ThreadArgs;

bool send_flag[SIZE] = { false, false, false, false, false };
bool is_end[SIZE] = { false, false, false, false, false };
char ACK[BUFSIZE];

long int ACK_num = 0;

int arrow = 0;

char packet[BUFSIZE];
char packets[SIZE][BUFSIZE] = { "packet 0", "packet 1", "packet 2", "packet 3", "packet 4" };
char message[BUFSIZE];

bool something_wrong = false;

// 클라이언트로부터 메시지를 수신하는 함수
void *receive_func(void *arg) {
    ThreadArgs *args = (ThreadArgs *)arg; // 스레드 인수 추출
    int index = args->index;
    SOCKET client_sock = args->sock;
    // int client_sock = *((int *)arg);

    char buf[BUFSIZE + 1];
    int retval;

    while (1) {
        // 데이터 받기
        retval = recv(client_sock, buf, BUFSIZE, 0);
        if (retval == SOCKET_ERROR) {
            err_display("recv()");
            break;
        } else if (retval == 0) {
            printf("[TCP 서버] 클라이언트와의 연결 종료\n");
            break;
        }

        // packet 부분을 추출하여 저장
        for (int i = 0; i < 8; ++i) {
            packet[i] = buf[i];
        }
        packet[8] = '\0'; // 문자열 끝을 표시하는 널 문자 추가

        // message 부분을 추출하여 저장
        int j = 0;
        for (int i = 9; i < strlen(buf); ++i) {
            message[j++] = buf[i];
        }
        message[j] = '\0'; // 문자열 끝을 표시하는 널 문자 추가

        // printf("[reciever %d]\n", index);
        // printf("[packet: %s]\n", packet);
        // printf("[message: %s]\n", message);

        // 받은 데이터 출력
        buf[retval] = '\0';

        // =================================== Data Check ===================================

        if (strcmp(packet, packets[arrow]) != 0) {
            printf("[Something Wrong!]\n");
            printf("[packet: %s]\n", packet);
            printf("[packet[%d]: %s]\n", arrow, packets[arrow]);
            something_wrong = true;
        }

        arrow++;

        printf("%s is received and there is no error. (%s) ", packet, message);

        send_flag[index] = true;
        // printf("[TCP 서버] %d바이트를 받았습니다.\n", retval);// 받은 데이터 출력
        // printf("- Client: %s\n", buf);
    }

    close(client_sock);
    pthread_exit(NULL);
}

// 클라이언트로 메시지를 보내는 함수
void *send_func(void *arg) {
    ThreadArgs *args = (ThreadArgs *)arg; // 스레드 인수 추출
    int index = args->index;
    SOCKET client_sock = args->sock;
    // int client_sock = *((int *)arg);

    char buf[BUFSIZE + 1];
    int retval;

    while (1) {
        // printf("\n\n[%d is wait..]\n\n", index);
        while (!send_flag[index]);
        // printf("\n\n[%d is run!]\n\n", index);
        send_flag[index] = false;
        
        char send_message[BUFSIZE] = "ACK = ";
        if (!something_wrong) {
            ACK_num += strlen(message);
        }
        sprintf(send_message + strlen(send_message), "%ld", ACK_num);
        strcpy(ACK, send_message);
        strcpy(buf, ACK);

        printf("(%s) is transmitted.\n", buf);
        // 3 second
        // usleep(3000000);

        int len = (int)strlen(buf);
        if (buf[len - 1] == '\n')
            buf[len - 1] = '\0';
        if (strlen(buf) == 0)
            break;

        retval = send(client_sock, buf, (int)strlen(buf), 0);
        if (retval == SOCKET_ERROR) {
            err_display("send()");
            break;
        }

    }

    close(client_sock);
    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    // 소켓 생성
    SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_sock == INVALID_SOCKET) err_quit("socket()");

    struct sockaddr_in serveraddr;
    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons(SERVERPORT);

    if (bind(listen_sock, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) == SOCKET_ERROR)
        err_quit("bind()");

    if (listen(listen_sock, SOMAXCONN) == SOCKET_ERROR)
        err_quit("listen()");

    struct sockaddr_in clientaddr;
    socklen_t addrlen = sizeof(clientaddr);
    SOCKET client_sock = accept(listen_sock, (struct sockaddr *)&clientaddr, &addrlen);
    if (client_sock == INVALID_SOCKET) {
        err_display("accept()");
    }

    pthread_t r_tid[SIZE], s_tid[SIZE];

    // 스레드 인수 초기화
    ThreadArgs r_args[SIZE], s_args[SIZE];
    for (int i = 0; i < SIZE; i++) {
        r_args[i].index = i;
        r_args[i].sock = client_sock;
        s_args[i].index = i;
        s_args[i].sock = client_sock;
    }

    // 클라이언트로부터 메시지를 수신하는 스레드 생성
    for (int i = 0; i < SIZE; i++) {
        if (pthread_create(&r_tid[i], NULL, receive_func, (void *)&r_args[i]) != 0) {
            printf("[TCP 서버] 수신 스레드 생성 실패\n");
            close(client_sock);
        }
    }

    // 클라이언트로 메시지를 보내는 스레드 생성
    for (int i = 0; i < SIZE; i++) {
        if (pthread_create(&s_tid[i], NULL, send_func, (void *)&r_args[i]) != 0) {
            printf("[TCP 서버] 전송 스레드 생성 실패\n");
            close(client_sock);
        }
    }

    for (int i = 0; i < SIZE; i++) {
        pthread_join(s_tid[i], NULL);
    }

    // 소켓 닫기
    close(listen_sock);
    return 0;
}
