// 入门提示: 
//   1. 使用解决方案资源管理器窗口添加/管理文件
//   2. 使用团队资源管理器窗口连接到源代码管理
//   3. 使用输出窗口查看生成输出和其他消息
//   4. 使用错误列表窗口查看错误
//   5. 转到“项目”>“添加新项”以创建新的代码文件，或转到“项目”>“添加现有项”以将现有代码文件添加到项目
//   6. 将来，若要再次打开此项目，请转到“文件”>“打开”>“项目”并选择 .sln 文件

#ifndef PCH_H
#define PCH_H

// TODO: 添加要在此处预编译的标头

//错误码定义
#define ERROR_UNKNOWN		-1
#define ERROR_PARSE_JSON	-2


//业务类型
#define CS_UNDEFINED		-1
#define CS_LOGIN             1
#define CS_REGISTER          2
#define CS_GETDEADLINE       3
#define CS_GETSYSTEMTIME     4
#define CS_RESETPASSWORD     5
#define CS_GETLASTESTVER     6
#define CS_BEYONDDEADLINE    7
#define CS_GETUSERTYPE       8
#define CS_GETUSERLEVEL      9
#define CS_GETFUNCTIONPERMISSION 10

#define CS_RELOGINTHREAD     99
#define CS_HEARTTHREAD		100

#endif //PCH_H
