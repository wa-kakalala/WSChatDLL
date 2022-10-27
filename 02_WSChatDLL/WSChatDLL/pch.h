// pch.h: 这是预编译标头文件。
// 下方列出的文件仅编译一次，提高了将来生成的生成性能。
// 这还将影响 IntelliSense 性能，包括代码完成和许多代码浏览功能。
// 但是，如果此处列出的文件中的任何一个在生成之间有更新，它们全部都将被重新编译。
// 请勿在此处添加要频繁更新的文件，这将使得性能优势无效。

#ifndef PCH_H
#define PCH_H

#define _DLLAPI
#ifdef  _DLLAPI
#define DLLAPI __declspec(dllexport)  // 导出
#else
#define DLLAPI __declspec(dllimport)  // 导入
#endif

// 添加要在此处预编译的标头
#include "framework.h"

extern "C" DLLAPI int  Init(void (*f) (const char* msg, int msglen));
extern "C" DLLAPI void Defer();
extern "C" DLLAPI int  ServerStart(const char* ip, unsigned short port, const char* name, int mode);
extern "C" DLLAPI int  ClientConnect(const char* ip, unsigned short port, int mode);
extern "C" DLLAPI int  ClientSendMessage(const char* msg, int msglen);
extern "C" DLLAPI int  ServerClose();
extern "C" DLLAPI int  ClientClose();
#endif //WSCHATDLL_H
