#ifndef _STR_UTILTY_H_
#define _STR_UTILTY_H_

#ifdef WIN32
#pragma warning(disable:4996)
#endif

#include <string>

#define END_LINE "\r\n"

//ȥ���ַ����������ߵĿո�
//����true��ʾ���޸�
//����false��ʾû���޸�
bool StrTrim(char *pData, int iLen = 0);

//iCount��1��ʼ
bool GetValueFromStr(const char *pData, const char *pKey, char *pValue, int iMaxValueLen, int iCount = 1);

bool GetValueFromStr(const char *pData, const char *pKey, int &iValue, int iCount = 1);

bool GetValueFromStr(const char *pData, const char *pKey, bool &bValue, int iCount = 1);

//����ָ���ָ���strDelimsǰ������
bool GetLastValueFromStr(const char *pData, const char* pDelims, int &iValue);

bool GetKeyAndValueFromStr(const char *pData, const char* pDelims, char *pKey, int iKeyLen, char *pValue, int iValueLen);

//utf8 <--> gbk ��ת
int Str_Utf8ToGbk(const char *inbuf,int inlen,char *outbuf,int outlen);
int Str_GbkToUtf8(const char *inbuf,int inlen,char *outbuf,int outlen);

//utf8 <--> gbk ��ת
std::string Str_Utf8ToGbk(const char *inbuf,int inlen = 0);
std::string Str_GbkToUtf8(const char *inbuf,int inlen = 0);

int Str_Utf8ToGbk(const char *inbuf,int inlen,std::string &out);
int Str_GbkToUtf8(const char *inbuf,int inlen,std::string &out);

#endif
