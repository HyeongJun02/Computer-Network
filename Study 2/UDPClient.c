#include "../Common.h"

<<<<<<< HEAD
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
=======
// IP 주소 저장하는 char 타입 배열
char *SERVERIP = (char *)"127.0.0.1";

// 포트: 9000
// 버퍼 크기: 10000
#define SERVERPORT 9000
#define BUFSIZE    10000
#define FILECOUNT  10

int main(int argc, char *argv[])
{
	// 소켓 함수의 반환값 저장
	int retval;

	// 명령행 인수가 있으면 IP 주소로 사용
	if (argc > 1) SERVERIP = argv[1];

	// 소켓 생성
	SOCKET sock = socket(AF_INET, SOCK_DGRAM, 0);
	// 유효하지 않은 경우
	if (sock == INVALID_SOCKET) err_quit("socket()");

	// 소켓 주소 구조체 초기화
	// serveraddr: 서버 주소 정보 설정 구조체
	struct sockaddr_in serveraddr;
	// memset: 구조체 0으로 초기화
	memset(&serveraddr, 0, sizeof(serveraddr));
	// AF_INET: IPv4 주소 사용
	serveraddr.sin_family = AF_INET;
	// sin_addr: 서버 IP 주소 설정
	inet_pton(AF_INET, SERVERIP, &serveraddr.sin_addr);
	// sin_port: 서버 포트 번호 설정
	serveraddr.sin_port = htons(SERVERPORT);

	// 데이터 통신에 사용할 변수
	// peeraddr: 주소 정보 저장 구조체
	struct sockaddr_in peeraddr;
	// addrlen: peeraddr의 길이
	socklen_t addrlen;
	// buf: 데이터 저장
	char buf[BUFSIZE + 1];
	// len: buf의 길이
	int len;

    // File List
    char fileNames[FILECOUNT][30] = {"novel.txt", "anthem.txt", "computer_network.txt"};

	// 서버와 데이터 통신
	// 서버에 데이터 전송
	while (1) {
		// 데이터 입력
		printf("\n[보낼 데이터] (request \"FILENAME\")\n");
        printf("<File List>\n");
        for(int i = 0; i < FILECOUNT; i++) {
            if (fileNames[i][0] == '\0') continue;
            printf("- %s\n", fileNames[i]);
        }
        printf("=> ");
		// 입력 실패 or EOF 도달 시 fgets => NULL
		if (fgets(buf, BUFSIZE + 1, stdin) == NULL)
			break;

		// '\n' 문자 제거
		len = (int)strlen(buf);
		if (buf[len - 1] == '\n')
			buf[len - 1] = '\0';
		// 입력이 없다면
		if (strlen(buf) == 0)
			break;

		// 데이터 보내기
		retval = sendto(sock, buf, (int)strlen(buf), 0,
			(struct sockaddr *)&serveraddr, sizeof(serveraddr));
		// 전송에 실패한 경우
		if (retval == SOCKET_ERROR) {
			err_display("sendto()");
			break;
		}
		printf("[UDP 클라이언트] %d바이트를 보냈습니다.\n", retval);

        char fileName[BUFSIZE];
        if (sscanf(buf, "request \"%[^\"]\"", fileName) != 1) {
            printf("Format: request \"FILENAME\"\n");
            continue;
        }
		printf("FILENAME: %s\n", fileName);

		// 데이터 받기
		addrlen = sizeof(peeraddr);
		retval = recvfrom(sock, buf, BUFSIZE, 0,
			(struct sockaddr *)&peeraddr, &addrlen);
		// 수신에 실패한 경우
		if (retval == SOCKET_ERROR) {
			err_display("recvfrom()");
			break;
		}

		// 송신자의 주소 체크
		if (memcmp(&peeraddr, &serveraddr, sizeof(peeraddr))) {
			printf("[오류] 잘못된 데이터입니다!\n");
			break;
		}

		// 받은 데이터 출력
		buf[retval] = '\0';
		printf("[UDP 클라이언트] %d바이트를 받았습니다.\n", retval);
		printf("[받은 데이터]\n");
        printf("%s\n", buf);
        printf("\nThe client received \"%s\" from the server.\n", fileName);
	}

	// 소켓 닫기
	close(sock);
	return 0;
}
>>>>>>> d62f2bf088604cbef306c6c86be7783c1a74e12e
