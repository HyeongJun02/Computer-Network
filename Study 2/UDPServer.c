#include "../Common.h"

<<<<<<< HEAD
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
=======
// 포트: 9000
// 버퍼 크기: 512
#define SERVERPORT 9000
#define BUFSIZE    512

int main(int argc, char *argv[])
{
    // 소켓 함수 반환값 저장
	int retval;

	// 소켓 생성
	SOCKET sock = socket(AF_INET, SOCK_DGRAM, 0);
    // 유효하지 않은 소켓의 경우 오류 출력 후 프로그램 종료
	if (sock == INVALID_SOCKET) err_quit("socket()");

	// bind()
    // 서버 주소 설정 구조체
	struct sockaddr_in serveraddr;
    // 구조체 0으로 초기화
	memset(&serveraddr, 0, sizeof(serveraddr));
    // AF_INET: IPv4 사용
	serveraddr.sin_family = AF_INET;
    // INADDR_ANY: 어떤 주소든, 데이터 수락
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    // 서버 포트 번호 설정
	serveraddr.sin_port = htons(SERVERPORT);
    // 소켓을 포트에 바인딩 (bind())
	retval = bind(sock, (struct sockaddr *)&serveraddr, sizeof(serveraddr));
    // 실패한 경우
	if (retval == SOCKET_ERROR) err_quit("bind()");

	// 데이터 통신에 사용할 변수
    // 주소 정보 저장 구조체
	struct sockaddr_in clientaddr;
    // 주소 길이 저장
	socklen_t addrlen;
    // 데이터 저장
	char buf[BUFSIZE + 1];
    char fileName[BUFSIZE];

	// 클라이언트와 데이터 통신
    // 수신하는 동안 무한루프
	while (1) {
		// 데이터 받기
		addrlen = sizeof(clientaddr);
        // recvfrom(): 클라이언트로부터 데이터 수신
		retval = recvfrom(sock, buf, BUFSIZE, 0,
			(struct sockaddr *)&clientaddr, &addrlen);
        // 수신에 실패한 경우
		if (retval == SOCKET_ERROR) {
			err_display("recvfrom()");
			break;
		}
		printf("=================================================\n");

		// 받은 데이터 출력
		buf[retval] = '\0';
		char addr[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &clientaddr.sin_addr, addr, sizeof(addr));
        // [UDP/주소:포트] 메시지
		printf("[UDP/%s:%d] %s\n", addr, ntohs(clientaddr.sin_port), buf);

        // 요청된 파일 이름 추출
        if (sscanf(buf, "request \"%[^\"]\"", fileName) != 1) {
            printf("Format: request \"FILENAME\"\n");
			printf("=================================================\n");
            continue;
        }
		printf("- The server received a request from a client\n");
		printf("FILENAME: %s\n", fileName);

		char *contents = NULL;
		long fileSize;
		size_t result;

        // 파일 열기
        FILE *fp = fopen(fileName, "r");
        if (fp == NULL) {
            err_display("fopen()");
            break;
        }

		// 파일 크기 계산
		fseek(fp, 0, SEEK_END);
		fileSize = ftell(fp);
		rewind(fp);

		// 파일 크기에 맞게 메모리 할당
		contents = (char *)malloc(fileSize * sizeof(char));
		if (contents == NULL) {
			perror("Failed. (contents == NULL)");
			fclose(fp);
			return 1;
		}

    	// 파일 내용 읽기
		result = fread(contents, 1, fileSize, fp);
		if (result != fileSize) {
			perror("Failed. (result != fileSize)");
			free(contents);
			fclose(fp);
			return 1;
		}

		retval = sendto(sock, contents, sizeof(char) * fileSize, 0, (struct sockaddr *)&clientaddr, sizeof(clientaddr));
		// 전송에 실패한 경우
		if (retval == SOCKET_ERROR) {
			err_display("sendto()");
			break;
		}
		printf("- The server sent \"%s\" to the client\n", fileName);
		printf("=================================================\n");

		// 메모리와 파일 닫기
		free(contents);
		fclose(fp);
	}

	// 소켓 닫기
	close(sock);
	return 0;
}
>>>>>>> d62f2bf088604cbef306c6c86be7783c1a74e12e
