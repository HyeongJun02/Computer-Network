#include "../Common.h"
#include <pthread.h>

#define SERVERPORT 9000
#define BUFSIZE    512

#define SIZE 5

bool isReceived[SIZE] = { false, false, false, false, false };

int arrow = 0;

typedef struct {
    int index;      // 스레드 인덱스
    SOCKET sock;    // 소켓 정보
} ThreadArgs;

char packets[SIZE][BUFSIZE] = {
    "packet 0:I am a boy.",
    "packet 1:You are a girl.",
    "packet 2:There are many animals in the zoo.",
    "packet 3:철수와 영희는 서로 좋아합니다!",
    "packet 4:나는 점심을 맛있게 먹었습니다."
};
char correct_ACK[SIZE + 1][BUFSIZE] = { 
    "ACK = 0",
    "ACK = 11",
    "ACK = 26",
    "ACK = 60",
    "ACK = 103",
    "ACK = 146"
};
char last_ACK[BUFSIZE];

int receive_arrow = 0;

int pre_receive = 0;

// 서버로부터 메시지를 수신하는 함수
void *receive_func(void *arg) {
    ThreadArgs *args = (ThreadArgs *)arg; // 스레드 인수 추출
    int index = args->index;
    SOCKET sock = args->sock;

    char buf[BUFSIZE + 1];
    int retval;

    while (1) {
        // 데이터 받기
        retval = recv(sock, buf, BUFSIZE, 0);
        if (retval == SOCKET_ERROR) {
            err_display("recv()");
            break;
        } else if (retval == 0) {
            break;
        }

        // 받은 데이터 출력
        buf[retval] = '\0';
        // printf("[TCP Sender] %d바이트를 받았습니다.\n", retval);
        printf("(%s) is received.\n", buf);
        // break;
        // if (strcmp(correct_ACK, buf) == 0) {
        //     break;
        // }

        // =======================================================================================
        strcpy(last_ACK, buf);
        if (strcmp(correct_ACK[receive_arrow + 1], buf) == 0) {
            isReceived[receive_arrow] = true;
            receive_arrow++;
            pre_receive = receive_arrow + 1;
            break;
        }
        else {
            isReceived[pre_receive] = true;
            pre_receive++;
        }
        // printf("\n");
        // printf("[isReceived]");
        // printf("[%d %d %d %d %d]\n\n", isReceived[0], isReceived[1], isReceived[2], isReceived[3], isReceived[4]);
    }

    int check_i = 0;
    while (1) {
        if (!isReceived[check_i]) {
            check_i = 0;
            continue;
        }
        check_i++;
        if (check_i == SIZE) break;
    }

    close(sock);
    pthread_exit(NULL);
}

// Time out
clock_t startTime[SIZE], endTime[SIZE];
double elapsedTime[SIZE];


bool first_packet1 = true;

// 서버로 메시지를 보내는 함수
void *send_func(void *arg) {
    ThreadArgs *args = (ThreadArgs *)arg; // 스레드 인수 추출
    int index = args->index;
    SOCKET sock = args->sock;
    
    // 0.5 * index second delay
    usleep(500000 * index);

    char buf[BUFSIZE + 1];
    int retval;

    while (!isReceived[index]) {
        while (arrow != index);
        // printf("%d is run\n", arrow);
        arrow++;

        memset(buf, 0, sizeof(buf));
        strcpy(buf, packets[index]);

        int len = (int)strlen(buf);
        if (buf[len - 1] == '\n')
            buf[len - 1] = '\0';
        if (strlen(buf) == 0)
            break;

        // first_packet1 => no!!
        // if (first_packet1 && strcmp(packets[1], buf)) {
        //     continue;
        // }

        retval = send(sock, buf, (int)strlen(buf), 0);
        if (retval == SOCKET_ERROR) {
            err_display("send()");
            break;
        }
        printf("packet %d is transmitted. (%s)\n", index, packets[index]);
        startTime[index] = clock();

        while (!isReceived[index]) {
            endTime[index] = clock();
            elapsedTime[index] = (double)(endTime[index] - startTime[index]) / CLOCKS_PER_SEC;
            // 전송 후 10초가 지나면 타임아웃
            if (elapsedTime[index] > 10) {
                printf("packet %d Time out\n", index);
                arrow = index;
                break;
            }
        }
    }

    // printf("[Thread %d is done]\n", index);

    int check_i = 0;
    while (1) {
        if (!isReceived[check_i]) {
            check_i = 0;
            continue;
        }
        check_i++;
        if (check_i == SIZE) break;
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

    pthread_t r_tid[SIZE], s_tid[SIZE];

    // 스레드 인수 초기화
    ThreadArgs r_args[SIZE], s_args[SIZE];
    for (int i = 0; i < SIZE; i++) {
        r_args[i].index = i;
        r_args[i].sock = sock;
        s_args[i].index = i;
        s_args[i].sock = sock;
    }

    // 서버로부터 메시지를 수신하는 스레드 생성
    for (int i = 0; i < SIZE; i++) {
        if (pthread_create(&r_tid[i], NULL, receive_func, (void *)&r_args[i]) != 0) {
            printf("[TCP Sender] 수신 스레드 생성 실패\n");
            close(sock);
            exit(1);
        }
        // 0.1 second delay
        usleep(100000);
    }

    // 서버로 메시지를 보내는 스레드 생성
    for (int i = 0; i < SIZE; i++) {
        if (pthread_create(&s_tid[i], NULL, send_func, (void *)&s_args[i]) != 0) {
            printf("[TCP Sender] 전송 스레드 생성 실패\n");
            close(sock);
            exit(1);
        }
    }

    for (int i = 0; i < SIZE; i++) {
        pthread_join(s_tid[i], NULL);
    }

    // 소켓 닫기
    close(sock);
    return 0;
}
