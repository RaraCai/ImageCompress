/*计算机 2252429 蔡宇轩*/
#pragma once
#include"Compress.h"
#include<unordered_map>

class Decode {
	//图像的基本信息
	PicReader imread;
	BYTE* data;//最后要还原回去的三通道RGB数据
	UINT img_width;//图片宽度
	UINT img_height;//图片高度
	ifstream infile;//读取压缩图片文件的文件输入流
private:
	//操作过程中的中间量
	//4个哈夫曼码的AC DC表
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

	//量化表
	unsigned char Luminance_Quantization_Table[64] = { 0 };
	unsigned char Chrominance_Quantization_Table[64] = {0};


	//3个YCbCr对应DC的上一个DC值
	int preDC[3] = { 0,0,0 };

	//Code Y_dc[16] = { 0 };//Y-DC哈夫曼编码表
	//Code C_dc[16] = { 0 };//cbcr-DC哈夫曼编码表
	//Code Y_ac[256] = { 0 };//Y-AC哈夫曼编码表
	//Code C_ac[256] = { 0 };//cbcr-ACh哈夫曼编码表
	//Code* HuffTable[4] = { Y_dc,C_dc,Y_ac,C_ac };//4个哈夫曼表的索引表
	//Code* AC_table[2] = { Y_ac,C_ac };//AC对应的哈夫曼表索引
	//Code* DC_table[2] = { Y_dc,C_dc };//DC对应的哈夫曼表索引
	const int Y_flag = 0;
	const int C_flag = 1;


	//按位读取数据用到的字符型和比特计数量
	char byte = 0;
	int bitcount = 0;

	//通过码值定位该哈夫曼码在哈夫曼表中的位置-->哈希表存储
	unordered_map<string, int> FindinHuffTable[4];//第一个int是key（code），第二个int是value（位序）

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
	//读取文件头
	bool readJPEGhead();
	//依据文件头的信息重建哈夫曼编码的ACDC表
	void readHuffTable(int tag);
	//重建哈夫曼编码表和对应的哈希表
	void BuildHuff_and_Hash();
	//读一个8*8矩阵对应的编码
	void readLine(int FLAG,int zdata[],int preFlag);
	//反zigzag、反DCT
	void reverseDCT(int FLAG,int zdata[], double YCbCrT[]);
	//转换回GRB三通道
	void cgto_RGB(double Y[], double Cb[], double Cr[],int x,int y);
	//向下通过查询哈希表读取一个Code
	int GetCode(int flag);
	//在文件中继续向下读取1个比特位
	int read_next_bit();
	//将从文件中读到的高低位倒序的数字正序计算出来并返回
	int cgto_num(short word);
	//将计算出的code值转换为string类
	string code_to_str(int code,int code_len);
public:
	//解压函数
	void MyDecode();
	//构造函数
	Decode() {};
	//析构函数
	~Decode() {};
};


