// 2142851 컴퓨터공학과 김형준
// Selective repeat Receiver

#include "../Common.h"
#include <pthread.h>
#include <ctype.h>

#define BUFSIZE    512
#define SERVERPORT 9000

char ACKs[6][10] = { "ACK 0", "ACK 1", "ACK 2", "ACK 3", "ACK 4", "ACK 5" };
char packets[6][10] = { "packet 0", "packet 1", "packet 2", "packet 3", "packet 4", "packet 5" };

int receive_arrow = 0;
int receive_number = 0;
int last_receive_number = 0;

bool receive_flag = false;
bool isReceived[7] = { false, false, false, false, false, false, false };

// 받은 게 뭔가 이상한지 확인
bool something_wrong = false;
bool buffered = false;

bool firstPacket2Drop = true;

// 클라이언트로부터 메시지를 수신하는 함수
void *receive_func(void *arg) {
    int client_sock = *((int *)arg);
    char buf[BUFSIZE + 1];
    int retval;

    while (1) {
        // 데이터 받기
        retval = recv(client_sock, buf, BUFSIZE, 0);
        if (retval == SOCKET_ERROR) {
            err_display("recv()");
            break;
        } else if (retval == 0) {
            printf("[disconnected]\n");
            break;
        }

        if (firstPacket2Drop && (strcmp(buf, packets[2]) == 0)) {
            firstPacket2Drop = false;
            continue;
        }

        // 받은 데이터 출력
        buf[retval] = '\0';
        // printf("[TCP 서버] %d바이트를 받았습니다.\n", retval);// 받은 데이터 출력
        if (strcmp(buf, packets[receive_arrow]) == 0) { // 같다면 (잘 왔다면)
            // 0.1 second delay
            usleep(100000);
            printf("\"%s\" is received. ", buf);
            receive_arrow++;
        }
        else { // 다르다면
            // 0.1 second delay
            usleep(100000);
            printf("\"%s\" is received and buffered. ", buf);
            something_wrong = true;
        }
        
        // "packet" 문자열 제거
        char *ptr = buf;
        while (*ptr && !isdigit(*ptr)) {
            ++ptr;
        }
        // 숫자 부분을 추출하여 정수로 변환
        if (*ptr) {
            receive_number = atoi(ptr);
        }
        if (receive_number < last_receive_number) {
            printf("%s, ", packets[receive_number]);
            int last_i;
            for (int i = receive_number + 1; isReceived[i + 1] == false; i++) {
                printf("%s, ", packets[i]);
                last_i = i;
            }
            printf("and %s are delivered. ", packets[last_i]);
            something_wrong = false;
        }
        last_receive_number = receive_number;

        isReceived[receive_arrow] = true;
        receive_flag = true;
        //printf("receive_flag : %d\n\n", receive_flag);
    }

    close(client_sock);
    pthread_exit(NULL);
}

// 클라이언트로 메시지를 보내는 함수
void *send_func(void *arg) {
    int client_sock = *((int *)arg);
    char buf[BUFSIZE + 1];
    int retval;

    while (1) {
        // if (fgets(buf, BUFSIZE + 1, stdin) == NULL)
        //     break;
        // int len = (int)strlen(buf);
        // if (buf[len - 1] == '\n')
        //     buf[len - 1] = '\0';
        // if (strlen(buf) == 0)
        //     break;

        if (receive_flag) {
            // 1 second delay
			// usleep(1000000);
            strcpy(buf, ACKs[receive_number]);
            retval = send(client_sock, buf, (int)strlen(buf), 0);
            if (retval == SOCKET_ERROR) {
                err_display("send()");
                break;
            }
            if (something_wrong) {
                printf("\"%s\" is retransmitted. \n", buf);
            }
            else {
                printf("\"%s\" is transmitted. \n", buf);
            }
            receive_flag = false;
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

    pthread_t r_tid, s_tid;

    // 클라이언트로부터 메시지를 수신하는 스레드 생성
    if (pthread_create(&r_tid, NULL, receive_func, &client_sock) != 0) {
        printf("[TCP 서버] 수신 스레드 생성 실패\n");
        close(client_sock);
    }

    // 클라이언트로 메시지를 보내는 스레드 생성
    if (pthread_create(&s_tid, NULL, send_func, &client_sock) != 0) {
        printf("[TCP 서버] 전송 스레드 생성 실패\n");
        close(client_sock);
    }

    pthread_join(s_tid, NULL);

    // 소켓 닫기
    close(listen_sock);
    return 0;
}
