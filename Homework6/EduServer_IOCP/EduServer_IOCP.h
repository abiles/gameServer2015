#pragma once

//todo: 아래 설정은 여러분들의 상황에 맞게 고치기

#define LISTEN_PORT		9001
#define MAX_CONNECTION	10000

#define CONNECT_SERVER_ADDR	"127.0.0.1"
#define CONNECT_SERVER_PORT 9001

// 제가 이부분을 127.0.0.1로 하면 연결이 안되는데 왜그럴까요? SY-PC라고 해야 됩니다 ㅜ.ㅜ ///# DB 인스턴스 이름 빼먹어서 그래
#define SQL_SERVER_CONN_STR	L"Driver={SQL SERVER};Server=SY-PC;Database=GameDB;UID=sa;PWD=next1234"

#define GQCS_TIMEOUT	10 //INFINITE


