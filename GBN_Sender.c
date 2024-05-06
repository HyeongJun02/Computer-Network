// 2142851 컴퓨터공학과 김형준
// Go-Back-N Sender

#include "../Common.h"
#include <pthread.h>

#define SERVERPORT 9000
#define BUFSIZE    512

char ACKs[6][10] = { "ACK 0", "ACK 1", "ACK 2", "ACK 3", "ACK 4", "ACK 5" };
char packets[6][10] = { "packet 0", "packet 1", "packet 2", "packet 3", "packet 4", "packet 5" };
// 가장 최근 받은 ACK
char lastACK[10];
// 사용하진 않음. => 잘 받았는지 확인
bool isReceived[6] = { false, false, false, false, false, false };

int arrow = 0;
int send_arrow = 0;

int send_area = 4;

clock_t startTime[6], endTime[6];
double elapsedTime[6];

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
        }

        buf[retval] = '\0';
        if (strcmp(buf, ACKs[arrow]) == 0) {
            printf("\"%s\" is received.\n", buf);
            isReceived[arrow] = true;
            arrow++;
            send_area++;
        }
        else {
            printf("\"%s\" is received and ignored.\n", buf);
        }
        strcpy(lastACK, buf);
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
        /*
        if (fgets(buf, BUFSIZE + 1, stdin) == NULL)
            break;

        int len = (int)strlen(buf);
        if (buf[len - 1] == '\n')
            buf[len - 1] = '\0';
        if (strlen(buf) == 0)
            break;
        */

        if (send_area > 0 && send_arrow < 6) {
            strcpy(buf, packets[send_arrow]);
            retval = send(sock, buf, (int)strlen(buf), 0);
            if (retval == SOCKET_ERROR) {
                err_display("send()");
                break;
            }
            printf("\"%s\" is transmitted.\n", buf);
            // 0.1 second delay
			usleep(100000);
            startTime[send_arrow] = clock();
            send_arrow++;
            send_area--;
        }
        if ((strcmp(buf, packets[5]) == 0) && (strcmp(lastACK, ACKs[5]) == 0)) {
            /*
            printf("===== END =====\n");
            printf("buf : %s\n", buf);
            printf("packets[5] : %s\n", packets[5]);
            printf("lastACK : %s\n", lastACK);
            printf("ACKs[5] : %s\n", ACKs[5]);
            printf("strcmp(buf, packets[5]) : %d\n", strcmp(buf, packets[5]));
            printf("strcmp(lastACK, ACKs[5]) : %d\n", strcmp(lastACK, ACKs[5]));
            printf("===== END =====\n");
            */
            break;
        }
        for (int i = arrow; i < send_arrow; i++) {
            endTime[i] = clock();
            elapsedTime[i] = (double)(endTime[i] - startTime[i]) / CLOCKS_PER_SEC;
        }
        // Time Out
        for (int i = 0; i < send_arrow; i++) {
            // 전송 후 5초가 지나면 타임아웃
            if (elapsedTime[i] > 5) {
                printf("%s is timeout.\n", packets[i]);
                // 보낼 수 있는 공간 및 arrow 초기화
                send_area = 4;
                send_arrow = i;
                break;
            }
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
