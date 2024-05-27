// 2142851 컴퓨터공학과 김형준
// Sender

#include "../Common.h"
#include <pthread.h>

#define SERVERPORT      9000

#define SIZE            32
#define BUFSIZE         50
#define PACKET_SIZE     9
#define PAYLOAD_SIZE    6

clock_t startTime[SIZE], endTime[SIZE];
double elapsedTime[SIZE];

bool receive_flag[SIZE];

typedef struct {
    int index;      // 스레드 인덱스
    SOCKET sock;    // 소켓 정보
} ThreadArgs;

typedef struct {
    char payload[PAYLOAD_SIZE];
    char checksum;
    char packet_number;
} Packet;
Packet packet[32];

int ACK = 0;
int last_ACK = 0;

// ================================================================================= OUTPUT TEXT FILE =================================================================================
FILE *output;

int output_setting() {
    output = fopen("output.txt", "w");
    if (output == NULL) {
        perror("Write Failed.\n");
        return 1;
    }
    if (dup2(fileno(output), STDOUT_FILENO) == -1) {
        perror("Error redirecting stdout.");
        return 1;
    }
    return 0;
}

// ================================================================================= FILE PROCESSING =================================================================================
void process_file(FILE *fp) {
    char payload[PAYLOAD_SIZE + 1];
    int packet_number = 0;
    
    while (fread(payload, 1, PAYLOAD_SIZE, fp) == PAYLOAD_SIZE) {
        payload[PAYLOAD_SIZE] = '\0';

        // 띄어쓰기 부분 처리
        for (int i = 0; i < PAYLOAD_SIZE; i++) {
            if (payload[i] == ' ') {
                fseek(fp, -2, SEEK_CUR);
                payload[PAYLOAD_SIZE - 2] = '\0';
            }
        }
        
        // printf("Packet %2d - payload: %s\n", packet_number, payload);
        memcpy(packet[packet_number].payload, payload, 6);
        // checksum error..
        // packet[packet_number].checksum = 1;
        packet[packet_number].packet_number = (char)packet_number;

        packet_number++;
    }
    // 출력
    // for (int i = 0; i < SIZE; i++) {
    //     printf("---------------------------------------\n");
    //     printf("[Packet %2d] payload: %s\n", i, packet[i].payload);
    //     printf("[Packet %2d] checksum: %c\n", i, packet[i].checksum);
    //     printf("[Packet %2d] packet_number: %d\n", i, packet[i].packet_number);
    //     printf("---------------------------------------\n");
    // }
}

// ================================================================================= RECEIVE FUNC =================================================================================
void *receive_func(void *arg) {
    ThreadArgs *args = (ThreadArgs *)arg; // thread arg
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
        }

        buf[retval] = '\0';

        int ACK_number;
        sscanf(buf, "(ACK = %d)", &ACK_number);
        ACK = ACK_number;
        receive_flag[ACK_number / PACKET_SIZE - 1] = true;
        if (last_ACK == ACK) {
            printf("%s is received and ignored.\n", buf);
        }
        else {
            printf("%s is received.\n", buf);
        }
        last_ACK = ACK;
        break;
    }

    // close(sock);
    pthread_exit(NULL);
}

// ================================================================================= SEND FUNC =================================================================================
void *send_func(void *arg) {
    // usleep(50000); // 0.05초 지연
    ThreadArgs *args = (ThreadArgs *)arg; // thread arg
    int index = args->index;
    SOCKET sock = args->sock;

    int retval;

    // printf("send_func %d is run\n", index);
    while (1) {
        Packet buf;
        memcpy(&buf, &packet[index], sizeof(Packet)); // packet[index]의 내용을 buf로 복사

        // printf("buf: %s\n", buf.payload);

        // usleep(50000);  // 0.05초 지연
        retval = send(sock, &buf, sizeof(Packet), 0); // Packet 구조체의 크기만큼 전송
        // printf("retval: %d\n", retval);
        if (retval == SOCKET_ERROR) {
            printf("send() failed with error: %d\n", errno);
            perror("send() error");
            err_display("send()");
            break;
        }
        printf("packet %2d is transmitted. (%s)\n", index, buf.payload);
        
        startTime[index] = clock();
        while ((!receive_flag[index]) && (elapsedTime[index] < 0.5)) {
            endTime[index] = clock();
            elapsedTime[index] = (double)(endTime[index] - startTime[index]) / CLOCKS_PER_SEC;
            // printf("elapsedTime[%d]: %lf\n", index, elapsedTime[index]);
        }
        if (receive_flag[index]) {
            // printf("is received and ");
            break;
        }
        else {
            printf("packet %2d is timeout.\n", index);
            break;
        }
    }

    // close(sock);
    pthread_exit(NULL);
}

// ================================================================================= MAIN =================================================================================
int main(int argc, char *argv[]) {
    // fopen
    FILE *fp = fopen("text.txt", "r");
    if (fp == NULL) {
        perror("Read Failed.\n");
        return 1;
    }
    output_setting();

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
    
    // FILE PROCESSING
    process_file(fp);

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
    }

    // 서버로 메시지를 보내는 스레드 생성
    for (int i = 0; i < SIZE; i++) {
        if (pthread_create(&s_tid[i], NULL, send_func, (void *)&s_args[i]) != 0) {
            printf("[TCP Sender] 전송 스레드 생성 실패\n");
            close(sock);
            exit(1);
        }
        usleep(50000); // 0.05초 지연
    }

    for (int i = 0; i < SIZE; i++) {
        pthread_join(s_tid[i], NULL);
    }

    printf("END\n");
    
    fclose(output);
    fclose(fp);

    // 소켓 닫기
    close(sock);
    return 0;
}
