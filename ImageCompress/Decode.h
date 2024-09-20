/*����� 2252429 ������*/
#pragma once
#include"Compress.h"
#include<unordered_map>

class Decode {
	//ͼ��Ļ�����Ϣ
	PicReader imread;
	BYTE* data;//���Ҫ��ԭ��ȥ����ͨ��RGB����
	UINT img_width;//ͼƬ���
	UINT img_height;//ͼƬ�߶�
	ifstream infile;//��ȡѹ��ͼƬ�ļ����ļ�������
private:
	//���������е��м���
	//4�����������AC DC��
	unsigned char DC_Y_NRCodes[16] = { 0 };
	unsigned char DC_Y_Values[16] = { 0 };

	unsigned char DC_C_NRCodes[16] = { 0 };
	unsigned char DC_C_Values[16] = { 0 };

	unsigned char AC_Y_NRCodes[16] = { 0 };
	unsigned char AC_Y_Values[256] = { 0 };

	unsigned char AC_C_NRCodes[16] = { 0 };
	unsigned char AC_C_Values[256] = { 0 };

	unsigned char* NRCodes_table[4] = { DC_Y_NRCodes, DC_C_NRCodes, AC_Y_NRCodes, AC_C_NRCodes };
	unsigned char* Values_table[4] = { DC_Y_Values, DC_C_Values, AC_Y_Values, AC_C_Values };

	//������
	unsigned char Luminance_Quantization_Table[64] = { 0 };
	unsigned char Chrominance_Quantization_Table[64] = {0};


	//3��YCbCr��ӦDC����һ��DCֵ
	int preDC[3] = { 0,0,0 };

	//Code Y_dc[16] = { 0 };//Y-DC�����������
	//Code C_dc[16] = { 0 };//cbcr-DC�����������
	//Code Y_ac[256] = { 0 };//Y-AC�����������
	//Code C_ac[256] = { 0 };//cbcr-ACh�����������
	//Code* HuffTable[4] = { Y_dc,C_dc,Y_ac,C_ac };//4�����������������
	//Code* AC_table[2] = { Y_ac,C_ac };//AC��Ӧ�Ĺ�����������
	//Code* DC_table[2] = { Y_dc,C_dc };//DC��Ӧ�Ĺ�����������
	const int Y_flag = 0;
	const int C_flag = 1;


	//��λ��ȡ�����õ����ַ��ͺͱ��ؼ�����
	char byte = 0;
	int bitcount = 0;

	//ͨ����ֵ��λ�ù��������ڹ��������е�λ��-->��ϣ��洢
	unordered_map<string, int> FindinHuffTable[4];//��һ��int��key��code�����ڶ���int��value��λ��

private:
	const int ZigzagMat[64] = {
	 0, 1, 5, 6,14,15,27,28,
	 2, 4, 7,13,16,26,29,42,
	 3, 8,12,17,25,30,41,43,
	 9,11,18,24,31,40,44,53,
	10,19,23,32,39,45,52,54,
	20,22,33,38,46,51,55,60,
	21,34,37,47,50,56,59,61,
	35,36,48,49,57,58,62,63
	};

private:
	//��ȡ�ļ�ͷ
	bool readJPEGhead();
	//�����ļ�ͷ����Ϣ�ؽ������������ACDC��
	void readHuffTable(int tag);
	//�ؽ������������Ͷ�Ӧ�Ĺ�ϣ��
	void BuildHuff_and_Hash();
	//��һ��8*8�����Ӧ�ı���
	void readLine(int FLAG,int zdata[],int preFlag);
	//��zigzag����DCT
	void reverseDCT(int FLAG,int zdata[], double YCbCrT[]);
	//ת����GRB��ͨ��
	void cgto_RGB(double Y[], double Cb[], double Cr[],int x,int y);
	//����ͨ����ѯ��ϣ���ȡһ��Code
	int GetCode(int flag);
	//���ļ��м������¶�ȡ1������λ
	int read_next_bit();
	//�����ļ��ж����ĸߵ�λ�������������������������
	int cgto_num(short word);
	//���������codeֵת��Ϊstring��
	string code_to_str(int code,int code_len);
public:
	//��ѹ����
	void MyDecode();
	//���캯��
	Decode() {};
	//��������
	~Decode() {};
};


