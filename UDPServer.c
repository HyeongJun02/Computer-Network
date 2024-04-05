#include "../Common.h"

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