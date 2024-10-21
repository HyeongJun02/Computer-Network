// Common.h 헤더 파일을 사용
#include "../Common.h"
#include <cstring>

using namespace std;

// 서버 포트 번호: 9000
// 버퍼 크기: 512
#define SERVERPORT 9000
#define BUFSIZE    512

// Calendar Size
#define CALSIZE 500

// 윤년인지 판별
int is_leap_year(int year)
{
    if ((year > 0) && !(year % 4) && ((year % 100) || !(year % 400)))
        return 1;

    return 0;
}

// AD 1년 1월 1일 12:00 AM 을 기준으로 year 까지 몇 번의 윤년이 있었는가?
int count_leaps(int year) {
    return (year - 1) / 4 - (year - 1) / 100 + (year - 1) / 400;
}

// AD 1년 1월 1일부터 몇일이 지났는가?
int count_days(int year) {
    return (year - 1) * 365 + count_leaps(year);
}

// 주어진 날의 수를 해수로 계산
int count_years(int days) {
    return (1 + (days - count_leaps(days / 365)) / 365);
}

// 지정한 달은 며칠까지 있는가?
int get_days_of_the_month(int year, int month) {
    // 각달의날짜수
    static int days_of_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

    // 해당월의날짜수
    int days = days_of_month[month - 1];
    // 윤년처리
    if (is_leap_year(year) && month == 2)
        days++;

    return days;
}

// AD 1년 1월 1일 부터 며칠이 지났는가?
int get_total_days(int year, int month, int day) {
    // 작년까지의 총 날짜
    int total_days = count_days(year);

    // 올해 지난달 까지의 총 날짜
    int m;
    for (m = 1; m < month; ++m) {
        total_days += get_days_of_the_month(year, m);
    }
    
    // 이번 달 오늘까지의 총 날짜
    total_days += day;

    return total_days;
}

// 주어진 날짜의 요일 계산
// 0 : 일요일, 1 : 월요일, ... 6: 토요일
int get_weekday(int year, int month, int day) {
    return get_total_days(year, month, day) % 7;
}

char calendar[CALSIZE];

// calendar를 그린다
void draw_calendar(int year, int month) {
    int days = get_days_of_the_month(year, month);
    // 1일의 요일을 구하고 빈칸의 개수를 계산한다
    int day = 1 - get_weekday(year, month, 1);

    int i, end;

    static char months[12][4] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    sprintf(calendar, "[%4d]%4s Calendar\n\n", year, months[month - 1]);

    printf("%4s%4s%4s%4s%4s%4s%4s\n", "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat");
    sprintf(calendar + strlen(calendar), "%4s%4s%4s%4s%4s%4s%4s\n", "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat");
    printf("----------------------------\n");
    sprintf(calendar + strlen(calendar), "----------------------------\n");
    // printf("%s", calendar);
    end = days - day + 1;
    for (i = 0; i < end; ++i) {
        // 빈칸 출력
        if (day < 1) {
            printf("%4s", " ");
            sprintf(calendar + strlen(calendar), "%4s", " ");
        }
        // 날짜 출력
        else {
            printf("%4d", day);
            sprintf(calendar + strlen(calendar), "%4d", day);
        }

        ++day;

        // 일요일이면 줄바꿈
        if (i % 7 == 6) {
            printf("\n");
            sprintf(calendar + strlen(calendar), "\n");
        }
    }
    printf("\n");
    sprintf(calendar + strlen(calendar), "\n");
    // printf("%s", calendar);
}

bool isLeapYear(int year) {
    if((year % 4 == 0 && year % 100 != 0 ) || year % 400 == 0)
        return true;
    else
        return false;
}

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

        // 클라이언트와 데이터 통신
        while (1) {
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

            // 받은 데이터 출력
            buf[retval] = '\0';
            printf("[TCP/%s:%d] %s\n", addr, ntohs(clientaddr.sin_port), buf);

            // char charYear[10], charMonth[10];
            // for(int i=0; i<4; i++) {
            //     charYear[i] = buf[i];
            // }
            // for(int i=0; i<2; i++) {
            //     charMonth[i] = buf[i+6];
            // }

            // printf("[charYear] %s\n", charYear);
            // printf("[charMonth] %s\n", charMonth);

            // int year = atoi(charYear);
            // int month = atoi(charMonth);
            
            // printf("[year] %d\n", year);
            // printf("[month] %d\n", month);

            int year, month;
            char dot;

            sscanf(buf, "%d.%c%d", &year, &dot, &month);

            printf("[strlen(calendar)] %ld\n", strlen(calendar));

            draw_calendar(year, month);

            // 데이터 보내기
            // retval = send(client_sock, buf, retval, 0);
            retval = send(client_sock, calendar, 500, 0);
            if (retval == SOCKET_ERROR) {
                err_display("send()");
                break;
            }
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
