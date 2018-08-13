
/*

.	SimpleXMLMaker: ���㹹��xml�ַ���
SimpleXMLMaker maker;
maker.SimpleAddNode("Get");			// "<Get/>"
maker.SimpleAddValue("val1","v");	// "<Get> <val1> v </val1> </Get>"
maker.AddValue("/Root/Get/Val","xxx"); //����ֱ����Ӷ��ڵ�,��"/"�ָ�.
maker.AddValue("/Root/Get:1/Val:2","mmm");//���ĳ�ڵ��ж��������ͬ���ӽڵ�,
							//������ ":����" �ķ�ʽָ��ʹ�õڼ����ڵ�.��0��ʼ����.
maker.Goto("/");
for(int i=0;i<2;i++)// for �������ʾ��.
{
	maker.SimpleAddNode("Set");//���Set�ڵ�,������Set�ڵ�.
	maker.SimpleAddValue("val",i);//��Set�ڵ���� val=i ��ֵ.
	maker.Goto(".."); //������һ��ڵ�.
}

std::string str;
maker.str(str); ��  str=maker.str(); ���Ի�ȡ�ַ���.

str��ֵ:
<?xml version="1.0" encoding="gbk" ?>
<Get>
	<val1>v</val1>
</Get>
<Root>
	<Get>
		<Val>xxx</Val>
	</Get>
	<Get>
		<Val />
		<Val />
		<Val>mmm</Val>
	</Get>
</Root>
<Set>
	<val>0</val>
</Set>
<Set>
	<val>1</val>
</Set>

������:

SimpleXMLMaker parse;
parse.Parse(str.c_str());
int i;
parse.GetValue("/Node1/Node2:2/Title",i);//��ȡһ��ֵ.��ֱ��ָ����ϸ��·��.

//���ڶ��ͬһ���ֵĸ��ڵ�����,����for + Goto/Next ������:
for(bool it = parse.Goto("/Set"); it ;it = parse.Next("Set"))
{
	parse.GetValue("",i);
	cout<<i<<",";
}
���:  0,1

//���Ҫ������ǰ�ڵ�������ӽڵ�,��ʹ�� for + GotoFirstChild/Next ������.
for(bool it = parse.GotoFirstChild();it;it=parse.Next())
{
	const char *pTitle = parse.GetNodeName();//��ȡ��ǰ�ڵ������
	const char *pVal = parse.GetValue(""); //��ȡ��ǰ�ڵ��ֵ.
}
parse.Goto("..");//ע��:��Ϊ: GotoFirstChild() �Ѿ��ı��˵�ǰ�ڵ�,������ɺ���Ҫ������һ��ڵ�

*/

#ifndef _SIMPLE_XML_PARSER_H__
#define _SIMPLE_XML_PARSER_H__

//#include <sstream>
//#include "Common/tinyxml/tinyxml.h"
//#include <stack>
#include <string>
#include <vector>

class TiXmlElement;
class TiXmlDocument;
class TiXmlNode;


int SplitStr_x(const char *str, const char *spstr, std::vector<std::string> &ret_result);
int SplitStr_x(const std::string &str, const std::string &spstr, std::vector<std::string> &ret_result);



/*
.	����xml������/������:
.
.	path���ν��͹���:
.	����� / ��ͷ,��ʾ�Ӹ��ڵ㿪ʼ��,�����ʾ�ӵ�ǰ�ڵ㿪ʼ.
.	����ֵ:  ���ڵ�:	/
.			 ��ǰλ��: (��)		.	./
.			 ��һ�ڵ�:	..	../.
.
.	path��ʽ: node1/node2:num/node3:num2/title ...
.	�ڽڵ������Ը���ð��+����,��ʾ���ҵڼ����ӽڵ�.�����ж���ڵ�����.
.	���ִ�0��ʼ����
.	��:
.			/Get/Set		Get�����Set�ڵ�
.			/Get:1/Set:2	��1��Get�ڵ��µ�2��Set�ڵ�
.	ע��: ��ӽڵ��ʱ��,���ʹ��ָ��������ʽ���,��ǰһ�������ǲ����ڵ�,�����Զ���Ӹ�����.
.	��:	������ /Get:2 , �� /Get:0 �� /Get:1 ������,
.		�ͻ��Ȱ� /Get:0 �� /Get:1 �ȼ���,����� /Get:2
.
.	ע: ������Ϊ:  path / title :     �� ��·�����ж��ν���.
.				 n_path / n_title : ���� ��·�����ж��ν���

*/

class SimpleXMLParser
{
public:
	typedef const char cchar;
public:

	//����ַ���
	void str(std::string &out_str, bool isCleanStr = true);
	std::string str();

public:
	SimpleXMLParser();
	SimpleXMLParser(cchar *xml_str);
	~SimpleXMLParser();
	bool Parse(cchar *xml);
	bool ParseFile(cchar *xml_file);
public:

	/*
	.	��ת��ĳ���ڵ�.(һ���������ظ��ڵ�)
	.ע:1)��ת��ı䵱ǰ�ڵ�λ��.
	.		2)���ڵ㲻����ʱ��תʧ��
	.��������� AddNode ������:
	.	AddNode : ���·��������,�򴴽���·��,����ת�����´����Ľڵ�.
	.	Goto	: ���·��������,����תʧ��,�Ҳ��ı䵱ǰ·��.
	*/
	bool Goto(cchar *path);

	/*
	.	��ת����ǰ�ڵ��һ���ӽڵ�.
	.�����path���ж��ν���.
	*/
	bool GotoFirstChild(cchar *n_path = NULL);

	//������ǰ�ڵ���һ���ڵ�.
	//����path���ж��ν���
	//���޸ĵ�ǰ�ڵ�λ��.
	bool Next(cchar *path = NULL);


	/* ��ȡ�ڵ������. (���޸ĵ�ǰ�ڵ�λ��)
	. ����һ���� "." ".." �Ȳ�ȷ����·��.(�Ѿ�ȷ����·����û�б�Ҫ��ȡ��)
	. �����ȡʧ��,���� NULL
	*/
	cchar *GetNodeName(cchar *path = NULL);

	//------ xml ������

	//��ȡ�ڵ��ֵ.(���� false ʱ,���޸��������.)
	//title = "" �� "." �� NULL ��ʾ��ȡ��ǰ�ڵ��ֵ.
	cchar *GetValue(cchar *title = NULL);
	bool GetValue(cchar *title, std::string &out);
	bool GetValue(cchar *title, char *out, unsigned int iOutBufSize = 0);
	bool GetValue(cchar *title, int &out);
	bool GetValue(cchar *title, unsigned int &out);
	bool GetValue(cchar *title, short &out);
	bool GetValue(cchar *title, unsigned short &out);
	bool GetValue(cchar *title, char &out);
	bool GetValue(cchar *title, unsigned char &out);
	bool GetValue(cchar *title, bool &out);
	bool GetValue(cchar *title, float &out);
	bool GetValue(cchar *title, double &out);
	bool GetValue(cchar *title, time_t &t);
	//��ȡ����������.. Ҫ��ڵ��ֵ������ʮ����������.
	//��ʽ��������������һ��:
	//		AA BB CC DD
	//		AA,BB,CC,DD
	//		a b,c , d
	//		AABBCCDD
	//		0xAA 0xBB 0xCC 0xDD
	//		0xAA0xBB0xCC0xDD
	//�����ᾡ���ܶ��ַ�������ʮ�����Ʒ���ת��,
	//����Դ�ַ����д���������������²���֤׼ȷ��.
	//����: out : ����Ļ�����
	//.		in_out_bufsize : ͬʱ��Ϊ����/�������.
	//.						����: out�������Ĵ�С.
	//.						���: out�������Ѿ�ʹ�õĴ�С.
	//.�����������޴������ AddValue() ���ɵ�ʮ�������ַ���.
	bool GetValue(cchar *title, unsigned char *out, int &in_out_bufsize);

protected:
	bool _create_xml_(cchar *xml_str = NULL);
	TiXmlElement *_find_node(TiXmlElement *parent, cchar *title, int index, int *out_i = NULL, bool *out_isFind = NULL);
	bool _goto(cchar *path, TiXmlElement *&out);
protected:
	TiXmlDocument *m_doc;//
	TiXmlElement *m_currNode;//��ǰ�ڵ�

};

class SimpleXMLMaker
	: public SimpleXMLParser
{
public:
	typedef const char cchar;

public:
	SimpleXMLMaker();
	SimpleXMLMaker(cchar *xml_str);
	~SimpleXMLMaker();

	/*
	.	�ڵ�ǰλ�����һ���ڵ�.
	.ע: 1)�������ж����ͬ���ֵĽڵ����.���һ���ڵ��Ѿ�����,�򲻻�������µĽڵ�.
	.		����½ڵ����ͨ��ǿ��ָ���ڵ���ŵķ�ʽָ��.
	.	 2)���һ���ڵ��,��ǰλ�ý��л�Ϊ����ӵĽڵ�
	.��������� Goto ������:
	.	AddNode : ���·��������,�򴴽���·��,����ת�����´����Ľڵ�.
	.	Goto	: ���·��������,����תʧ��,�Ҳ��ı䵱ǰ·��.
	*/
	bool AddNode(cchar *path);


	/*
	.	�ڵ�ǰλ�����һ���ڵ�.
	.ע: 1) ������Ӷ����ͬ�Ľڵ�.
	.	 2) ����½ڵ�ʱ��������нڵ�����κμ��.
	.	 3) ����path���ж��ν���.path�����ַ�����ֱ����Ϊ�ڵ����ƽ������.
	.��AddNode��������,AddNode��Խڵ�����������,��SimpleAddNode�򲻼��.
	*/
	bool SimpleAddNode(cchar *n_path);


	/*
	.	�ڵ�ǰλ�����һ��ֵ
	.ע:1)�����������ͬ������.�������,���ֵ����ϲ���һ��.
	.	2)���ֵ���ı䵱ǰλ��.
	.
	.ʮ�����Ʒ�ʽ:
	.	fmt		ʾ��:
	.	0		aabbccdd
	.	1		aa bb cc dd
	.	2		0xaa 0xbb 0xcc 0xdd
	.	4		AABBCCDD
	.	5		AA BB CC DD
	.	6		0XAA 0XBB 0XCC 0XDD
	*/
	//bool AddValue(cchar *path,cchar *title,int val);
	bool AddValue(cchar *title, cchar *val);//�ַ���
	bool AddValue(cchar *title, const std::string &val);//�ַ���
	bool AddValue(cchar *title, int val);//����
	bool AddValue(cchar *title, const unsigned int &val);//32λ�޷���
	bool AddValue(cchar *title, bool val);//true/false
	bool AddValue(cchar *title, const time_t &t);//time_t
	bool AddValue(cchar *title, const unsigned char *val, int bufflen , int fmt = 1); // 16�����ַ���
	bool AddValue(cchar *title, const float &val);//
	bool AddValue(cchar *title, const double &val);//

	/* ����ֵ.
	.	�����Ҫ�ı����нڵ������,������������.
	.	���title�ڵ㲻����,��ȼ��� AddValue()
	.
	.	�˺����� AddValue() ��������:
	.	�����Ѿ����ڵ�title , AddValue() �ǽ��µ�ֵ���ӵ��ɵ�ֵ����., �ȼ���  += ����
	.						UpdateValue() �������µ�ֵ�滻�ɵ�ֵ.�ȼ��� = ����
	.	����:���title�ڵ����ӽڵ�,�����������ӽڵ�..
	*/
	bool UpdateValue(cchar *title, cchar *val);//�ַ���
	bool UpdateValue(cchar *title, const std::string &val);//�ַ���
	bool UpdateValue(cchar *title, int val);//����
	bool UpdateValue(cchar *title, const unsigned int &val);//32λ�޷���
	bool UpdateValue(cchar *title, bool val);//true/false
	bool UpdateValue(cchar *title, const time_t &t);//time_t
	bool UpdateValue(cchar *title, const unsigned char *val, int bufflen , int fmt = 1); // 16�����ַ���
	bool UpdateValue(cchar *title, const float &val);//
	bool UpdateValue(cchar *title, const double &val);//

	// ����ע��
	// ͬһ·��������ö��,ע�ͽ������.
	bool AddComments(cchar *path, cchar *val);

	/*	���һ��ֵ.
	.ע: 1)���������ͬ������.
	.	 2)����path���ж��ν���.path�����ַ�����ֱ����Ϊ�ڵ����ƽ������.
	.	 3)���ֵ���ı䵱ǰλ��
	.	 4)�����for,������Ҫ��Ӷ����ͬ��ֵ,ֻ���� SimpleAddXxx()����
	*/
	bool SimpleAddValue(cchar *n_title, cchar *val);
	bool SimpleAddValue(cchar *n_title, const std::string &val);//�ַ���
	bool SimpleAddValue(cchar *n_title, int val);//����
	bool SimpleAddValue(cchar *n_title, const unsigned int &val);//32λ�޷���
	bool SimpleAddValue(cchar *n_title, bool val);//true/false
	bool SimpleAddValue(cchar *n_title, const time_t &t);//time_t
	bool SimpleAddValue(cchar *n_title, const unsigned char *val, int bufflen, int fmt = 1); // 16�����ַ���
	bool SimpleAddValue(cchar *n_title, const float &val);//
	bool SimpleAddValue(cchar *n_title, const double &val);//

	/* ɾ��һ��ֵ.
	*/
	bool EraseValue(cchar *n_title);

	bool SaveToFile(cchar *szFile);

protected:
	TiXmlElement *_create_mult_node(cchar *path);//�����ڵ�(��������ڵĻ�)
	TiXmlElement *_find_and_create_node(TiXmlElement *parent, cchar *title, int index); //���һ򴴽�һ���ڵ�(��������ڵĻ�)

};



//����ת��
//int Utf8ToGb2312(char *sOut,int iMaxOutLen,const char *sIn,int iInLen);
//int Gb2312ToUtf8(char *sOut,int iMaxOutLen,const char *sIn,int iInLen);





#endif
