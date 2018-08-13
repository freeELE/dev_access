#ifndef _STRING_UTILTY_H_
#define _STRING_UTILTY_H_

#define END_LINE "\r\n"

#include <string>
#include <vector>
#include <map>
using namespace std;

//ȥ���ַ����������ߵĿո�
//����true��ʾ���޸�
//����false��ʾû���޸�
bool StrTrim(string &strData);

//iCount��1��ʼ
bool GetValueFromStr(const string &strData, const string &strKey, string &strValue, int iCount = 1);

bool GetValueFromStr(const string &strData, const string &strKey, int &iValue, int iCount = 1);

bool GetValueFromStr(const string &strData, const string &strKey, bool &bValue, int iCount = 1);

//����ָ���ָ���strDelimsǰ������
bool GetLastValueFromStr(const string &strData, const string &strDelims, int &iValue);

bool GetKeyAndValueFromStr(const string &strData, const string &strDelims, string &strKey, string &strValue);

//�ֽ��ַ���
int SplitStr(const char *str,const char *spstr,std::vector<std::string> &ret_result,bool isSkipEmptyItem = false);
int SplitStr(const std::string &str,const std::string&spstr,std::vector<std::string> &ret_result,bool isSkipEmptyItem = false);

int SplitParamStr(const char* str,std::map<string,string> &out,const char *spstr = ";",const char *opt_str = "=");

#endif
