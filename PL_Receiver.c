// 2142851 컴퓨터공학과 김형준
// FR_Receiver.c

#include "../Common.h"
#include <pthread.h>
#include <string.h>


#define SERVERPORT      9000

#define SIZE            32
#define BUFSIZE         50
#define PACKET_SIZE     9
#define PAYLOAD_SIZE    6

typedef struct {
    int index;      // 스레드 인덱스
    SOCKET sock;    // 소켓 정보
} ThreadArgs;

// 패킷에 저장될 데이터들
typedef struct {
    char payload[PAYLOAD_SIZE];
    char checksum;
    char packet_number;
} Packet;
Packet packet[32];

// "받았으니 보내라"를 의미하는 변수
bool send_flag[SIZE];

// ACK 설정 (데이터의 크기만큼 증가)
// 버퍼에 더 못 넣을 경우 -> ACK 증가 X, buf_overflow true로 변경
int ACK = 0;
bool buf_overflow;

// 버퍼에 저장 (최대 크기 : 50)
char buf_save[BUFSIZE];
// receive된 버퍼의 크기
int buf_size;

int message_length(const char *packet) {
    const char *a = strchr(packet, ':');
    if (a == NULL) {
        return 0;
    }
    return strlen(a + 1);
}

// checksum
int checksum_func(const char *packet) {
    int length = message_length(packet);
    return length;
}

// ================================================================================= RECEIVE FUNC =================================================================================
void *receive_func(void *arg) {
    ThreadArgs *args = (ThreadArgs *)arg; // thread arg
    int index = args->index;
    SOCKET client_sock = args->sock;

    char buf[BUFSIZE + 1];
    int retval;

    int receive_index;

    while (1) {
        // 데이터 받기
        // printf("Receive Thread %d is ready.\n", index);
        retval = recv(client_sock, &packet[index], sizeof(Packet), 0);
        // 인덱스 변경 필요!!
        // 받은 packet_number를 server receive_func의 index에 넣기
        receive_index = packet[index].packet_number;

        // CHECK retval
        // printf("retval: %d\n", retval);
        // printf("packet.payload: %s\n", packet[index].payload);
        if (retval == SOCKET_ERROR) {
            err_display("recv()");
            break;
        } else if (retval == 0) {
            printf("[TCP Receiver %d] END\n", index);
            break;
        }

        // 받은 데이터 출력
        buf[retval] = '\0';

        usleep(100000);  // 0.1초 지연

        if ((int)strlen(buf_save) + (int)strlen(packet[index].payload) >= BUFSIZE) {
            buf_overflow = true;
        }
        else {
            strcat(buf_save, packet[index].payload);
        }
        buf_size = (int)sizeof(packet[index]);

        printf("packet %2d is received and there is no error. (%s)\n", packet[index].packet_number, packet[index].payload);
        send_flag[receive_index] = true;
        break;
    }

    // close(client_sock);
    pthread_exit(NULL);
}

// ================================================================================= SEND FUNC =================================================================================
void *send_func(void *arg) {
    ThreadArgs *args = (ThreadArgs *)arg; // thread arg
    int index = args->index;
    SOCKET client_sock = args->sock;
    // int client_sock = *((int *)arg);

    char buf[BUFSIZE + 1];
    int retval;

    while (1) {
        while (!send_flag[index]);

        if (!buf_overflow) {
            ACK += buf_size;
        }
        sprintf(buf, "(ACK = %d)", ACK);

        retval = send(client_sock, buf, (int)strlen(buf), 0);

        printf("%s is transmitted.\n", buf);
        break;
    }

    // close(client_sock);
    pthread_exit(NULL);
}

// ================================================================================= MAIN =================================================================================
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
            printf("[TCP Receiver] 수신 스레드 생성 실패\n");
            close(client_sock);
        }
        // printf("[Receiver Thread Create %d]\n", i);
    }

    // 클라이언트로 메시지를 보내는 스레드 생성
    for (int i = 0; i < SIZE; i++) {
        if (pthread_create(&s_tid[i], NULL, send_func, (void *)&r_args[i]) != 0) {
            printf("[TCP Receiver] 전송 스레드 생성 실패\n");
            close(client_sock);
        }
        // printf("[Sender Thread Create %d]\n", i);
    }

    for (int i = 0; i < SIZE; i++) {
        pthread_join(s_tid[i], NULL);
    }

    // 소켓 닫기
    close(listen_sock);
    close(client_sock);
    return 0;
}
