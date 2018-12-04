#pragma once



#pragma once
//Deprecate waring
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers

#pragma comment(lib,"ws2_32")
#pragma comment(lib,"Dbghelp")
#pragma comment(lib, "Winmm.lib")
#pragma comment(lib,"Imm32.lib")
// Windows Header Files:
#include <windows.h>
#include <Windowsx.h>
#include <winsock2.h>
#include <WS2tcpip.h>
#include <process.h>
#include <psapi.h>

#include <mmsystem.h>
#include <winuser.h>
#include <timeapi.h>

// C RunTime Header Files
#include <stdlib.h>
#include <stdio.h>
#include <cstdint>
#include <string.h>
#include <memory.h>
#include <tchar.h>
#include <stdarg.h>
#include <conio.h>


// TODO: reference additional headers your program requires here

#include <DbgHelp.h>

#include <iostream>
#include <vector>
#include <queue>
#include <list>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <stack>
#include <string>
#include <sstream>
#include <cstring>
#include <memory>
#include <fstream>
#include <locale>
#include <algorithm>
#include <numeric>
#include <limits>
#include <utility>
#include <cstdint>

using std::cin;
using std::cout;
using std::cerr;
using std::endl;
using std::getline;
using std::get;
using std::vector;
using std::queue;
using std::map;
using std::multimap;
using std::unordered_map;
using std::unordered_set;
using std::stack;
using std::list;
using std::string;
using std::wstring;
using std::wostringstream;
using std::to_string;
using std::to_wstring;
using std::accumulate;
using std::ifstream;
using std::ofstream;
using std::ios_base;
using std::tolower;
using std::locale;
using std::transform;
using std::pair;
using std::make_pair;
using std::move;
using std::ref;
using std::forward;
using std::min_element;
using std::max;
using std::numeric_limits;
using std::begin;
using std::end;
using std::advance;

using std::wifstream;
using std::wofstream;
using std::wostream;
using std::wcin;
using std::wcout;
using std::wcerr;

#include "UtilDef.h"
#include "UtilMath.h"
#include "UtilLogger.h"
#include "UtilErrorCheck.h"
#include "UtilCrashDump.h"
#include "UtilSocketErrorFunction.h"
#include "CProfiler.h"
#include "CProfilerManager.h"


#include "CRingBuffer.h"
#include "CMemoryPool.h"
#include "CMemoryPoolTLS.h"
#include "QueueBound.h"
#include "Queue.h"
#include "CSeriesBufferException.h"
#include "CSeriesBuffer.h"

#include "CLanSess.h"
#include "CLanServer.h"