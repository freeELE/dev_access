#ifndef __DEV_ACCESS_H__
#define __DEV_ACCESS_H__

#include <stdint.h>

//PLC通信协议模式
enum PLCMODE
{
	eMODBUS,
	eMC_ACSII,
	eMC_BINARY
};

//数据库类型
enum SQLTYPE
{
	eSQL_SERVER,
	eORCALE,
	eMYSQL
};

//PLC设备类型
enum PLCDEVTYPE
{
	eXINJIE_XC3_32T_E = 0,				//信捷
	eMITSUBISHI_Q02UCCPU,			//三菱Q02UCCPU
	eMITSUBISHI_Q03UDVCPU,			//三菱Q03UDVCPU
	eMITSUBISHI_FX3U_32M,			//三菱FX3U-32M

};

//SQL设备类型
enum SQLDEVTYPE
{
	eMICROPLAN = 0,				//迈克普兰
};

//plc 配置
typedef struct _stPLCConf
{
	char szTitle[256];		//标题
	char szIpAddr[20];		//IP地址
	uint16_t uPort;			//端口号
	//PLCMODE mode;			//协议类型
	PLCDEVTYPE devType;		//设备类型
	int interval;			//与设备交互的操作间隔
}stPLCConf;

//数据库配置
typedef struct _stSQLConf
{
	char szTitle[256];		//标题
	char szIpAddr[20];		//IP地址
	uint16_t uPort;			//端口号
	char szUser[256];		//用户名
	char szPwd[256];		//密码
	char szDbName[256];		//数据库名
	SQLDEVTYPE devType;		//设备类型
	int interval;			//与设备交互的操作间隔
}stSQLConf;




#endif // !__DEV_ACCESS_H__
