#pragma once

namespace UtilDef
{
	static const int MAX_USER                   = 1024;
	static const int MAX_ROOM                   = 1024;
	static const int MAX_ROOM_FULL              = 1024;
	static const int SERIES_BUFFER_DEFAULT_SIZE = 1024;


	namespace Network
	{
		static const int SERVER_PORT            = 6000;
		static const int UM_SOCKET              = WM_USER + 1;
		static const int SERVER_RINGBUFFER_SIZE = 500000;
		static const int SERVER_WSASEND_MAX     = 100;

		namespace Error
		{
			static const int ENGINE_ERROR = 100000;
		}
	};
};
