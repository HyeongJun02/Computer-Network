// 2142851 컴퓨터공학과 김형준

// Common.h 헤더 파일을 사용
#include "../Common.h"
#include <cstring>

using namespace std;

// 서버 포트 번호: 9000
// 버퍼 크기: 512
#define SERVERPORT 9000
#define BUFSIZE    512

char ACKs[10][10] = { "ACK 0", "ACK 1", "ACK 2", "ACK 3", "ACK 4", "ACK 5" };
char packets[10][10] = { "packet 0", "packet 1", "packet 2", "packet 3", "packet 4", "packet 5" };

int main(int argc, char *argv[])
{
	// 소켓 함수의 반환값을 저장하는 변수 선언
	int retval;

	// 소켓 생성
	SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	// 유효한 소켓이 아닌 경우 err_quit() 함수 호출
	// 오류 출력 후 프로그램 종료
	if (listen_sock == INVALID_SOCKET) err_quit("socket()");

	// bind()
	// 구조체(struct) serveraddr 선언
	// IPv4 + 포트
	struct sockaddr_in serveraddr;
	// serveraddr의 값을 0으로 초기화
	memset(&serveraddr, 0, sizeof(serveraddr));
	// IPv4를 나타내는 AF_INET 값 설정
	serveraddr.sin_family = AF_INET;
	// 수신할 IP 주소 설정
	// INADDR_ANY: 서버가 시스템에 있는 모든 네트워크 인터페이스를 통해 들어오는 연결을 수신한다.
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	// 수신할 포트 설정
	// 포트: SERVERPORT 값 (#define SERVERPORT 9000)
	serveraddr.sin_port = htons(SERVERPORT);
	// bind(): 서버 소켓에 주소 할당
	// listen_sock: 서버 소켓의 파일 디스크립터
	retval = bind(listen_sock, (struct sockaddr *)&serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR) err_quit("bind()");

	// listen()
	// 클라이언트의 접속 대기
	retval = listen(listen_sock, SOMAXCONN);
	if (retval == SOCKET_ERROR) err_quit("listen()");

	// 데이터 통신에 사용할 변수
	// 통신할 소켓, 클라이언트 주소, 버퍼 선언
	SOCKET client_sock;
	struct sockaddr_in clientaddr;
	socklen_t addrlen;
	char buf[BUFSIZE + 1];

	while (1) {
		// accept()
		// 클라이언트 연결 요청이 들어올 시 소켓 생성
		// (그 전까지 대기)
		addrlen = sizeof(clientaddr);
		client_sock = accept(listen_sock, (struct sockaddr *)&clientaddr, &addrlen);
		if (client_sock == INVALID_SOCKET) {
			err_display("accept()");
			break;
		}

		// 접속한 클라이언트 정보 출력
		// 클라이언트 접속 시 IP 주소와 포트 출력
		char addr[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &clientaddr.sin_addr, addr, sizeof(addr));
		printf("\n[TCP 서버] 클라이언트 접속: IP 주소=%s, 포트 번호=%d\n",
			addr, ntohs(clientaddr.sin_port));

		// count : 몇 번째 패킷을 받을 차례인가
		int cnt = 0;
		char lastACK[10];
		strcpy(lastACK, ACKs[0]);

		// 클라이언트와 데이터 통신
		while (1) {
			bool isMissing;
			// 데이터 받기
			retval = recv(client_sock, buf, BUFSIZE, 0);
			// 오류 발생 시 break
			if (retval == SOCKET_ERROR) {
				err_display("recv()");
				break;
			}
			// 수신 데이터 크기가 0일 시 break
			else if (retval == 0)
				break;

			// 받은 데이터 CHECK
			buf[retval] = '\0';
			// TEST
			printf("\n===========\nbuf: %s\npackets[%d]: %s\nstrcmp(buf, packets[cnt]): %d\n===========\n", buf, cnt, packets[cnt], strcmp(buf, packets[cnt]));
			if (strcmp(buf, packets[cnt]) == 0) { // 같다면 (잘 왔다면)
				printf("\"%s\" is received. ", buf);
				strcpy(lastACK, ACKs[cnt]);
				isMissing = false;
			}
			else { // 다르다면
				printf("\"%s\" is received and dropped. ", buf);
				isMissing = true;
			}

			// 타임아웃 및 sender의 전송시간을 벌기 위해 0.1초 동안 멈춤 (100,000)
			usleep(100000);

			// 데이터 보내기
			// retval = send(client_sock, buf, retval, 0);

			retval = send(client_sock, lastACK, 500, 0);	
			printf("\"%s\" is transmitted.\n", lastACK);
			if (retval == SOCKET_ERROR) {
				err_display("send()");
				break;
			}

			if(!isMissing) cnt++;
		}

		// 소켓 닫기
		// 연결 종료
		close(client_sock);
		// 종료 시 메시지 출력
		printf("[TCP 서버] 클라이언트 종료: IP 주소=%s, 포트 번호=%d\n",
			addr, ntohs(clientaddr.sin_port));
	}

	// 소켓 닫기
	close(listen_sock);
	return 0;
}
