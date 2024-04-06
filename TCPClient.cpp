#include "../Common.h"

// 서버의 IP 주소: 127.0.0.1
char *SERVERIP = (char *)"127.0.0.1";
#define SERVERPORT 9000
#define BUFSIZE    512

#define CALSIZE 500

int main(int argc, char *argv[])
{
    // 소켓 함수 반환값을 저장할 변수 선언
    int retval;

    // 명령행 인수가 있으면 IP 주소로 사용
    if (argc > 1) SERVERIP = argv[1];

    // 소켓 생성
    // socket() 함수 호출 -> TCP 소켓 생성
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    // 오류 발생 시 메시지 출력 후 종료
    if (sock == INVALID_SOCKET) err_quit("socket()");

    // connect()
    // serveraddr: 주소 정보 설정
    struct sockaddr_in serveraddr;
    // 구조체 serveraddr 0으로 초기화
    memset(&serveraddr, 0, sizeof(serveraddr));
    // AF_INET: IPv4 주소 체계 사용함
    serveraddr.sin_family = AF_INET;
    // inet_pton(): 문자열 형태의 IP 주소를 이진 형태로 변환 후 sin_addr에 저장
    inet_pton(AF_INET, SERVERIP, &serveraddr.sin_addr);
    // server의 포트 번호 설정
    serveraddr.sin_port = htons(SERVERPORT);
    // connect(): 서버 연결
    retval = connect(sock, (struct sockaddr *)&serveraddr, sizeof(serveraddr));
    // 연결 실패 시 오류 출력 후 종료
    if (retval == SOCKET_ERROR) err_quit("connect()");

    // 데이터 통신에 사용할 변수
    char buf[BUFSIZE + 1];
    // 변수 len: 입력 데이터 길이 저장 변수
    int len;

    // 서버와 데이터 통신
    while (1) {
        // 데이터 입력
        printf("\n[보낼 데이터 (YYYY. MM)] ");
        // fgets를 통해 데이터 입력 받고, buf에 저장
        // 입력 실패 시 break
        if (fgets(buf, BUFSIZE + 1, stdin) == NULL)
            break;

        // '\n' 문자 제거
        // 입력 데이터 길이 계산 후 len에 저장
        len = (int)strlen(buf);
        // \n 문자가 있는 경우 제거
        if (buf[len - 1] == '\n')
            buf[len - 1] = '\0';
        // 입력이 없다면 break
        if (strlen(buf) == 0)
            break;

        // 데이터 보내기
        // send(): 데이터를 소켓에 작성
        retval = send(sock, buf, (int)strlen(buf), 0);
        if (retval == SOCKET_ERROR) {
            err_display("send()");
            break;
        }
        // 데이터 전송한 경우: 전송 바이트 출력
        printf("[TCP 클라이언트] %d바이트를 보냈습니다.\n", retval);

        // 데이터 받기
        // recv(): 데이터를 소켓에서 읽어옴
        retval = recv(sock, buf, 500, MSG_WAITALL);
        // 수신 실패 or 연결 종료: err_display() 출력 후 break
        if (retval == SOCKET_ERROR) {
            err_display("recv()");
            break;
        }
        else if (retval == 0)
            break;

        // 받은 데이터 출력
        buf[retval] = '\0';
        printf("[TCP 클라이언트] %d바이트를 받았습니다.\n", 500);
        printf("[받은 데이터]\n%s\n", buf);
    }

    // 소켓 닫기
    close(sock);
    return 0;
}
