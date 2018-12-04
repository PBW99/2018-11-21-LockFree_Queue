#include "SharedPCH.h"

void err_MsgBox(LPCWSTR msg)
{
	MessageBoxW(nullptr, L"Error", msg, MB_ICONERROR|MB_TASKMODAL|MB_TOPMOST
| MB_SETFOREGROUND|MB_DEFAULT_DESKTOP_ONLY);
}
// 소켓 함수  출력 후 종료
void err_SockMsgBox(LPCWSTR msg)
{
	LPWSTR lpMsgBuf;
	int ret = WSAGetLastError();
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
	              NULL,
	              ret,
	              MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
	              (LPTSTR)&lpMsgBuf,
	              0,
	              NULL);


	MessageBoxW(nullptr, (LPCWSTR)lpMsgBuf, msg, MB_ICONERROR|MB_TASKMODAL|MB_TOPMOST
| MB_SETFOREGROUND|MB_DEFAULT_DESKTOP_ONLY);

	LocalFree(lpMsgBuf);
}


void formatMsg(int err, LPSTR* outmsg)
{

	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
	              NULL,
	              err,
	              MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
	              (LPSTR)outmsg,
	              0,
	              NULL);

	return ;
}
