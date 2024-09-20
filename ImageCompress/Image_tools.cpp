/*计算机 2252429 蔡宇轩*/
#include "PicReader.h"
#include "Compress.h"
#include "Decode.h"


//求一个8*8像素矩阵对应的ycbcr矩阵
void JPEG::cgto_YCbCr(BYTE* RGBbuffer, double Ydata[], double Cbdata[], double Crdata[]) {
	//index是相对于RGBbuffer的偏移量
	int index = 0;
	for (int i = 0; i < 8; i++) {
		int first = index;
		for (int j = 0; j < 8; j++) {
			double R = (double)RGBbuffer[index], G = (double)RGBbuffer[index + 1], B = (double)RGBbuffer[index + 2];
			Ydata[i * 8 + j] = 0.29871 * R + 0.58661 * G + 0.11448 * B - 128;
			Cbdata[i * 8 + j] = -0.16874 * R - 0.33126 * G + 0.5 * B;
			Crdata[i * 8 + j] = 0.5 * R - 0.41869 * G - 0.08131 * B;
			index += 4;
		}
		index = first + img_width * 4;
	}
}
/**********将一个8 * 8矩阵Mat作傅里叶变换并量化***********/
//data[]原始数据
		//dct_data经过DCT变换和zigzag处理后的一维数组
		//QuantTable量化表，Y和CbCr有不同的量化表，常量
void JPEG:: cgto_DCT(double data[], int dct_data[], const unsigned char QuantTable[]) {
	//DCT变换的本质是data[]经过基变换转换到dct_data[]
	//dct_data[]是以64个标准图像为基时对应的系数
	const double pi = 3.1415926;//定义pai的取值
	for (int u = 0; u < 8; u++) {
		for (int v = 0; v < 8; v++) {
			//本质是二维矩阵，2个alpha系数
			double alpha_u = (u == 0) ? 1.f / sqrt(8.f) : 0.5f;
			double alpha_v = (v == 0) ? 1.f / sqrt(8.f) : 0.5f;

			//系数矩阵未量化前的值记录在临时变量tmp中
			double tmp = 0;
			//此处作双层的累加
			for (int x = 0; x < 8; x++) {
				for (int y = 0; y < 8; y++) {
					//当前data中的对应的矩阵元素值
					double cur_data = data[x * 8 + y];
					//求DCT基变换系数的公式
					cur_data *= cos((2 * x + 1) * u * pi / 16.f) * cos((2 * y + 1) * v * pi / 16.f);
					tmp += cur_data;//做累加
				}
			}
			tmp *= alpha_u * alpha_v;
			////tmp的值量化取整
			int tmp_c = (int)round(tmp / (double)QuantTable[u * 8 + v]);//除以量化表对应位置的系数即可
			////直接zigzag存入dct_data[]
			short ZigzagIndex = ZigzagMat[u * 8 + v];
			dct_data[ZigzagIndex] = tmp_c;
		}
	}
}

//编码Huffman
void JPEG::BuildHuffTable() {
	const unsigned char* every_code_len[4] = //每个编码表的不同码长下对应的码数量
	{ DC_Y_NRCodes,DC_C_NRCodes,AC_Y_NRCodes,AC_C_NRCodes };
	const unsigned char* start[4] =//每个编码表的不同码长对应的一组码的开始位置
	{ DC_Y_Values,DC_C_Values,AC_Y_Values,AC_C_Values };
	//type表示当前创建的编码表种类
	//HuffTable是装有4个编码表索引的大表
	for (int type = 0; type < 4; type++) {
		int index = 0;//在自己的表中对应的下标索引
		int code = 0;//编码
		Code* p = HuffTable[type];
		for (int i = 0; i < 16; i++) {
			//cout << "now length=" << i + 1 << endl;
			for (int j = 0; j < *(every_code_len[type] + i); j++) {
				//没有长度为0的编码，码长从1开始
				//范式哈夫曼：同一长度的码长相邻只差1
				
				p[ValuesPtr[type][index]].code = code;
				//cout << "code=" << code << endl;
				p[ValuesPtr[type][index]].length = i + 1;
				code++;
				index++;
			}
			code <<= 1;//相邻码长：某长度开始的第一个编码等于上一个长度的最后一个编码左移一位
		}
	}
}

/**************************bit表部分********************************/
//完成游程RLE编码和相应AC、DC部分的数值转化
//int eob表示EOB在data中的位置
//AC编码
void JPEG::AC_bitcnt(int TABLEFLAG, int eob, int dct_z_data[], Code* HuffCode, int& index) {
	//判断是否是（15,0）的标志位,15*16+0=0xf0=240
	bool FZR = false;
	Code combine_FZR= *(AC_table[TABLEFLAG] + 0xf0);//huffcode for (15,0)FZR

	//记录连续0的数量
	int zerocnt = 0;
	int i = 1;
	//我们只能操作到数组下标eob以前
	while(i<eob){

		//当前读到的值是dct_z_data[i]

		//记录连续0
		while (dct_z_data[i] == 0) {
			//cout << i << endl;
			zerocnt++;
			if (i >= eob) {
				i--; 
				zerocnt--;
				break;
			}
			//若有连续的16个0则需跳出
			if (zerocnt >= 16) {
				FZR = true;
				break;
			}//判断FZR
			i++;
		}// counting num of zero
		if (FZR) {
			//只存储FZR即可，无需存储bit码
			HuffCode[index] = combine_FZR;
			index++;

			////test
			//cout << "当前编得AC部分Huffman码为：";
			//printbin(HuffCode[index - 1].code, HuffCode[index - 1].length);
			//getchar();
			//cout << "特殊FZR，无bit码";
			//getchar();
			////test over


			zerocnt = 0;//计数量清零
			FZR = false;//标志位清零
		}//FZR
		else {
			//dct_z_data的绝对值
			int abs_val = (int)fabs(dct_z_data[i]);
			Code BitCode = { 0 };
			BitCode.length = 0; BitCode.code = 0;
			//计算abs_val的二进制数长度
			while (abs_val > 0) {
				abs_val >>= 1;
				BitCode.length++;
			}//length of abs_val=length of bitcode
			abs_val = (int)fabs(dct_z_data[i]);//还原abs_val

			//首先编哈夫曼码
			unsigned char combine = zerocnt * 16 + BitCode.length;
			HuffCode[index] = *(AC_table[TABLEFLAG] + combine);
			index++;

			////test
			//cout << "当前编得AC部分Huffman码为：";
			//printbin(HuffCode[index - 1].code, HuffCode[index - 1].length);
			//getchar();
			////test over


			//比特码
			if (dct_z_data[i] > 0) {//正数
				//BitCode的值是其本身
				BitCode.code = dct_z_data[i];
				HuffCode[index] = BitCode;
				index++; 

				////test
				//cout << "当前编得AC部分bit码为：";
				//printbin(HuffCode[index - 1].code, HuffCode[index - 1].length);
				//getchar();
				////test over

				zerocnt = 0;
			}
			else if (abs_val != dct_z_data[i]) {//负数
				BitCode.code = (int)pow(2, BitCode.length) - 1 +dct_z_data[i];
				HuffCode[index] = BitCode;
				index++;

				////test
				//cout << "当前编得AC部分bit码为：";
				//printbin(HuffCode[index - 1].code, HuffCode[index - 1].length);
				//getchar();
				////test over

				zerocnt = 0;
			}
		}
		i++;
	}
}	
//DC编码
void JPEG::DC_bitcnt(int TABLEFLAG, int dct_z_data, Code HuffCode[], int& index, int PREFLAG) {

	//计算本次的DC与上一个DC的差值
	int difference = dct_z_data - preDC[PREFLAG];
	preDC[PREFLAG] = dct_z_data;//更新pre

	//difference的绝对值
	int abs_val = (int)fabs(difference);
	//bit码
	Code BitCode = { 0 };
	//计算abs_val的二进制数长度
	while (abs_val > 0) {
		abs_val >>= 1;
		BitCode.length++;
	}//length of abs_val=length of bitcode

	//第一位的合并字符,哈夫曼码
	unsigned char combine = 16 * 0 + BitCode.length;
	

	//difference与0合并成为的combine作为第一个哈夫曼码
	HuffCode[index] = *(DC_table[TABLEFLAG] + combine);
	index++;//Huffman

	////test
	//cout << "当前编得DC部分Huffman码为：";
	//printbin(HuffCode[index - 1].code, HuffCode[index - 1].length);
	//getchar();
	////test over



	if (difference != 0) {//第二位存储比特码（只有difference=非零数才存）
		abs_val = (int)fabs(difference);
		if (difference > 0) {//正数
			//BitCode的值是其本身
			BitCode.code = difference;
		}
		else if (abs_val != difference) {//负数
			BitCode.code = (int)pow(2, BitCode.length) - 1 + difference;
		}
		HuffCode[index] = BitCode;
		index++;
		////test
		//cout << "当前编得DC部分bit码为：";
		//printbin(HuffCode[index - 1].code, HuffCode[index - 1].length);
		//getchar();
		////test over
	}
	else {
		//cout << "DC data=0,无bit码";
		//getchar();
		;
	}
}


//TABLEFLAG:标记当前是Y还是C表
//dct_data是已经量化并zigzag的数据
int JPEG::Huff_encoding(int TABLEFLAG,int dct_z_data[],Code HuffCode[],int PREFLAG) {
	int index = 0;//当前码的下标索引
	/******************************DC编码*********************************/
	DC_bitcnt(TABLEFLAG, dct_z_data[0], HuffCode, index, PREFLAG);
	/******************************DC编码*********************************/

	/******************************AC编码*********************************/
	//找到dct_data中EOB的位置
	int i = 63;
	while (dct_z_data[i] == 0)
		i--;
	int eob = ++i;//找到末尾第一个0出现的位置
	//编码eob之前的AC部分
	if (eob > 1) {
		AC_bitcnt(TABLEFLAG, eob, dct_z_data, HuffCode, index);
	}
	//写入EOB
	Code EOB = AC_table[TABLEFLAG][0x00];//EOB->0
	if (eob <= 63) {
		HuffCode[index] = EOB;
		index++;
		//cout << "当前编码EOB：";
		//printbin(HuffCode[index - 1].code, HuffCode[index - 1].length);
		//getchar();
	}
	/******************************AC编码*********************************/
	//cout << "Huff ok!" << endl;
	return index;//返回整个编码的长度
}
void JPEG:: write_bits(Code HuffCode[],const int index) {
	int lala = 0;
	//cout << "writing..." << endl;
	for (int i = 0; i < index; i++) {
		int code = HuffCode[i].code;
		int length = HuffCode[i].length;
		while (length>0) {
			//每满8位输出1个字节
			unsigned char bit = (unsigned char)GetBit(code, length);//当前位数下code的比特位值
			//cout << (int)bit;
			byte <<= 1;//待输出字符左移1位
			if (bit)
				byte |= 1;
			bitcount++;
			length--;//已经输出则长度-1，当length减为0时一次循环结束，读数组下一个编码
			if (bitcount == 8) {
				outstr += byte;			
				//cout << "满8位！当前打印的是：0x" << hex << setw(2) << (unsigned int)byte << endl;
				//与JPEG的FF标记区分
				if (byte == (char)0xff) {
					outstr += '\0';
					//cout << "上一个是0xff，需要补尾0：" << "0x00" << endl;
				}
				//清空
				byte = 0;
				bitcount = 0;
			}
		}
		//cout << "一个存储的code已经get完毕！" << endl;
	}
	//getchar();
}
void JPEG:: MyCompress() {
	//创建并打开写入文件
	outfile.open("lena.jpg", ios::binary);
	if (!outfile) {
		cerr << "failed to open output file!" << endl;
		exit(-1);
	}
	//写入文件头
	writeJPEGhead(outfile);
	//生成哈夫曼表
	BuildHuffTable();
	//开始编码
	//每8*8个矩阵快操作一次
	for (unsigned int x = 0; x < img_height; x += 8) {
		for (unsigned int y = 0; y < img_width; y += 8) {
			/*cout << "现在是第"<<times << "个8*8矩阵：" << endl;*/
			//当前8*8矩阵左上角像素点所在的数组下标
			BYTE* RGBbuffer = data + x * img_width * 4 + y * 4;//每个像素点都有rgba四个值
			//由RGB转化为ycbcr三个矩阵
			double Y_data[64] = { 0 }, Cb_data[64] = { 0 }, Cr_data[64] = { 0 };
			cgto_YCbCr(RGBbuffer, Y_data, Cb_data, Cr_data);
			//DCT
			int Y_dct[64] = { 0 }, Cb_dct[64] = { 0 }, Cr_dct[64] = { 0 };
			int index=0;
			//压缩Y通道
			cgto_DCT(Y_data, Y_dct, Luminance_Quantization_Table);
			//test
			//cout << "now you are encoding Y for matrix  " << times << endl;
			//for (int i = 0; i < 64; i++) {
			//	cout <<dec<< Y_dct[i] << ' ';
			//	if ((i + 1) % 8 == 0)
			//		cout << endl;
			//}
			//getchar();
			//test over

			//Y通道的Huffman编码
			Code* HuffCode_Y = new(nothrow)Code[128];//最多有64个，每一个都分成2部分
			if (!HuffCode_Y) {
				cerr << "failed to allocate!" << endl;
				exit(-1);
			}
			index=Huff_encoding(Y_flag, Y_dct, HuffCode_Y,Y_flag);//index?????????????
			//cout << endl << " Y- encoding is over! now let's write!";
			//getchar();
			write_bits(HuffCode_Y,index);
			delete[] HuffCode_Y;


			//压缩Cb通道
			//cout << endl << "now you are encoding Cb for matrix" << times << endl;
			cgto_DCT(Cb_data, Cb_dct, Chrominance_Quantization_Table);
			//test
			//for (int i = 0; i < 64; i++) {
			//	cout << dec<<Cb_dct[i] << ' ';
			//	if ((i + 1) % 8 == 0)
			//		cout << endl;
			//}getchar();
			//test over

			Code* HuffCode_Cb = new(nothrow)Code[128];
			if (!HuffCode_Cb) {
				cerr << "failed to allocate!" << endl;
				exit(-1);
			}
			index=Huff_encoding(C_flag, Cb_dct, HuffCode_Cb,C_flag);

			//cout << endl << " CB- encoding is over! now let's write!";
			//getchar();

			write_bits(HuffCode_Cb,index);
			delete[] HuffCode_Cb;


			//压缩Cr通道
			cgto_DCT(Cr_data, Cr_dct, Chrominance_Quantization_Table);
			//test
			//cout << endl << "now you are encoding Cr for matrix" << times << endl;
			//for (int i = 0; i < 64; i++) {
			//	cout <<dec<< Cr_dct[i] << ' ';
			//	if ((i + 1) % 8 == 0)
			//		cout << endl;
			//}
			//getchar();
			//test over

			Code* HuffCode_Cr = new(nothrow)Code[128];
			if (!HuffCode_Cr) {
				cerr << "failed to allocate!" << endl;
				exit(-1);
			}
			index=Huff_encoding(C_flag, Cr_dct, HuffCode_Cr,Cr_pre_flag);
			//cout << endl << " CR- encoding is over! now let's write!";
			//getchar();
			write_bits(HuffCode_Cr,index);
			delete[] HuffCode_Cr;
		}
	}
	writeJPEGtail(outfile);
	outfile.close();
	return;
}
int JPEG::GetBit(unsigned int code, int length) {
	
	int bit;
	//按位与的遮罩
	int mask[] = { 1,2,4,8,16,32,64,128,256,512,1024,2048,4096,8192,16384,32768 };
	bit = code & mask[length - 1];
	if (bit)
		return 1;
	else
		return 0;
}
void JPEG::write_word(int word,ofstream&outfile) { 
	char low = (char)(word & 0xff);
	char high = (char)(word >> 8);
	outfile.write((char*)&high, sizeof(char));
	outfile.write((char*)&low, sizeof(char));
}
//写文件头
void JPEG::writeJPEGhead(ofstream& outfile){
	//SOI
	write_word(0xffd8, outfile);
	//APPO
	write_word(0xffe0, outfile);
	write_word(16, outfile);//数据长度16
	//标识符"JFIF"
	char JFIF_str[5] = "JFIF";
	outfile.write((char*)JFIF_str, 5);

	//版本
	unsigned char version = 1;
	outfile.write((char*)&version, sizeof(version));
	//版本
	outfile.write((char*)&version, sizeof(version));

	outfile << (unsigned char)0;//X和Y的密度单位
	write_word(1, outfile);//X方向像素密度
	write_word(1, outfile);//Y方向像素密度
	write_word(0, outfile);//缩略图水平和垂直像素数目，无缩略图
	
	
	//DQT
	write_word(0xffdb, outfile);//标识符
	write_word(132, outfile);//数据长度
	//精度和类型
	outfile.put(0);
	//zigzag写入第一个量化表-Y
	unsigned char Qtable[64] = { 0 };
	for (int i = 0; i < 64; i++) 
		Qtable[ZigzagMat[i]] = Luminance_Quantization_Table[i];
	outfile.write((char*)Qtable, 64);


	outfile.put(1);
	//zigzag写入第二个量化表CbCr
	for (int i = 0; i < 64; i++)
		Qtable[ZigzagMat[i]] = Chrominance_Quantization_Table[i];
	outfile.write((char*)Qtable, 64);

	//SOF0
	write_word(0xffc0, outfile);//标记符
	write_word(17, outfile);//数据长度
	//图像精度
	outfile.put(8);
	write_word(img_height & 0xffff, outfile);//图像高度
	write_word(img_width & 0xffff, outfile);//图像宽度


	//颜色分量数恒为3
	outfile.put(3);
	//3个颜色分量YCbCr
	//Y
	outfile.put(1);//颜色分量ID-1-Y标志
	outfile.put(0x11);//水平和垂直采样因子
	outfile.put(0);//当前分量使用的量化表ID YTable id=0
	//Cb
	outfile.put(2);//颜色分量ID-2-Cb
	outfile.put(0x11);//水平和垂直采样因子
	outfile.put(1);//当前分量使用的量化表ID CTable id=1

	//Cr
	outfile.put(3);//颜色分量ID-2-Cb
	outfile.put(0x11);//水平和垂直采样因子
	outfile.put(1);//当前分量使用的量化表ID CTable id=1


	//DHT
	write_word(0xffc4, outfile);//标识符
	//计算DHT段数据长度
	int len[4] = { 0 };
	for (int i = 0; i < 16; i++) {
		len[0] += DC_Y_NRCodes[i];
		len[1] += DC_C_NRCodes[i];
		len[2] += AC_Y_NRCodes[i];
		len[3] += AC_C_NRCodes[i];
	}
	int length = len[0] + len[1] + len[2] + len[3] + 2 + 4 + 4 * 16;
	//数据长度计算完毕
	write_word(length, outfile);//写入数据长度
	
	//给Y和CbCr分别编DC,AC表
	//HT-Y-DC
	outfile.put(0x00);
	outfile.write((char*)DC_Y_NRCodes, 16);
	outfile.write((char*)DC_Y_Values, len[0]);

	
	//HT-Y-AC
	outfile.put(0x10);
	outfile.write((char*)AC_Y_NRCodes, 16);
	outfile.write((char*)AC_Y_Values, len[2]);


	//HT-CbCr-DC
	outfile.put(0x01);
	outfile.write((char*)DC_C_NRCodes, 16);
	outfile.write((char*)DC_C_Values, len[1]);


	//HT-CbCr-AC
	outfile.put(0x11);
	outfile.write((char*)AC_C_NRCodes, 16);
	outfile.write((char*)AC_C_Values, len[3]);


	//SOS
	write_word(0xffda, outfile);//标识符
	write_word(12, outfile);//数据长度
	//颜色分量数Ycbcr=3
	outfile.put(3);

	//颜色分量信息
	outfile.put(1); outfile.put(0x00);//y
	outfile.put(2); outfile.put(0x11);//cb
	outfile.put(3); outfile.put(0x11);//cr

	//压缩图像数据
	outfile.put(0x00);//谱选择开始
	outfile.put(0x3f);//谱选择结束
	outfile.put(0x00);//谱选择

}
//写文件尾
void JPEG::writeJPEGtail(ofstream& outfile) {
	if (bitcount > 0 && bitcount < 8) {
		byte <<= (8 - bitcount);
		outstr += byte;
	}
	outfile << outstr;
	//EOI
	write_word(0xffd9, outfile);
}


/****************************解码部分******************************/
//读取文件头存储4个哈夫曼AC DC表以及图片的基本信息，返回值为是否读取成功
bool Decode::readJPEGhead() {
	infile.open("lena.jpg", ios::binary);
	if (!infile) {
		cerr << "failed to open compressed file!" << endl;
		return false;
	}
	//只需要图片的基本信息，JPEG格式的文件头实则不需要
	//这里定义不同长度的缓冲变，方便读取
	unsigned char byte=0;//1字节
	unsigned short word=0;//字=2字节
	unsigned int dword=0;//双字=4四节
	//哎，这个方法做压缩的时候没想到，解码的时候想到了。。

	//读取SOI
	infile.read((char*)&word, sizeof(word));//0xffd8
	//读取APPO
	infile.read((char*)&dword, sizeof(dword));//0xffe0,0x16
	//读取标识符JFIF
	char JFIF[5] = { 0 };
	infile.read(JFIF, 5);
	//版本
	infile.read((char*)&byte, sizeof(byte));//version1
	infile.read((char*)&byte, sizeof(byte));//version2
	//X,Y密度单位
	infile.read((char*)&byte, sizeof(byte));
	infile.read((char*)&word, sizeof(word));//X方向像素密度
	infile.read((char*)&word, sizeof(word));//Y方向像素密度
	infile.read((char*)&word, sizeof(word));//缩略图水平和垂直像素数目，无缩略图

	//读取DQT
	infile.read((char*)&word, sizeof(word));//标识符
	infile.read((char*)&word, sizeof(word));//数据长度

	//精度和类型
	infile.read((char*)&byte, sizeof(byte));//0
	unsigned char Qtable_z[64] = { 0 };//暂存读取到的zigzag之后的量化表
	//反zigzag还原Y量化表
	for (int i = 0; i < 64; i++) {
		infile.read((char*)&byte, sizeof(byte));
		Qtable_z[i] = (unsigned char)byte;
	}
	//反zigzag赋值给Y量化表
	for (int i = 0; i < 64; i++)
		Luminance_Quantization_Table[i] = Qtable_z[ZigzagMat[i]];


	infile.read((char*)&byte, sizeof(byte));//1
	//反zigzag还原C量化表
	for (int i = 0; i < 64; i++) {
		infile.read((char*)&byte, sizeof(byte));
		Qtable_z[i] = (unsigned char)byte;
	}
	//反zigzag赋值给C量化表
	for (int i = 0; i < 64; i++)
		Chrominance_Quantization_Table[i] = Qtable_z[ZigzagMat[i]];

	//读取SOFO
	infile.read((char*)&word, sizeof(word));//0xffc0
	infile.read((char*)&word, sizeof(word));//length of data=17
	//图像精度
	infile.read((char*)&byte, sizeof(byte));//8
	infile.read((char*)&word, sizeof(word));//图像高度
	img_height = (UINT)cgto_num(word);
	
	infile.read((char*)&word, sizeof(word));//图像宽度
	img_width = (UINT)cgto_num(word);


	//颜色分量YCbCr
	infile.read((char*)&byte, sizeof(byte));//颜色分量数=3
	//Y
	infile.read((char*)&byte, sizeof(byte)); //颜色分量ID-1-Y标志
	infile.read((char*)&byte, sizeof(byte));//水平和垂直采样因子0x11
	infile.read((char*)&byte, sizeof(byte));//当前分量使用的量化表ID YTable id=0
	//Cb
	infile.read((char*)&byte, sizeof(byte)); //颜色分量ID-2-Cb标志
	infile.read((char*)&byte, sizeof(byte));//水平和垂直采样因子0x11
	infile.read((char*)&byte, sizeof(byte));//当前分量使用的量化表ID CTable id=1
	//Cr
	infile.read((char*)&byte, sizeof(byte)); //颜色分量ID-2-Cr标志
	infile.read((char*)&byte, sizeof(byte));//水平和垂直采样因子0x11
	infile.read((char*)&byte, sizeof(byte));//当前分量使用的量化表ID CTable id=1

	//读取DHT
	infile.read((char*)&word, sizeof(word));//标识符0xffc4
	infile.read((char*)&word, sizeof(word));//数据长度

	//读取4个哈夫曼ACDC表
	for (int i = 0; i < 4; i++) {
		//当前表的标识记号type=0~3
		//分别对应:0-y_dc,1-c_dc,2-y_ac,3-c_ac
		infile.read((char*)&byte, sizeof(byte));
		unsigned char tag = (unsigned char)byte;
		readHuffTable(tag);//生成ACDC表
	}
	//读取SOS
	infile.read((char*)&word, sizeof(word));//标识符0xffda
	infile.read((char*)&word, sizeof(word));//数据长度12

	infile.read((char*)&byte, sizeof(byte));//颜色分量数ycbcr=3

	//颜色分量信息
	infile.read((char*)&word, sizeof(word));//y
	infile.read((char*)&word, sizeof(word));//cb
	infile.read((char*)&word, sizeof(word));//cr

	//压缩图像数据
	infile.read((char*)&byte, sizeof(byte));//谱选择开始
	infile.read((char*)&byte, sizeof(byte));//谱选择结束
	infile.read((char*)&byte, sizeof(byte));//请选择

	return true;
}
void Decode::readHuffTable(int tag) {
	int len = 0;
	unsigned char nrc_codes[16] = { 0 };
	infile.read((char*)nrc_codes, 16);
	int type = 2 * (tag / 16) + tag % 16;

	for (int i = 0; i < 16; i++) {
		len += nrc_codes[i];//求values数组的数量
		*(NRCodes_table[type] + i) = nrc_codes[i];//存储NRCodes数组
	}
	//存储values数组
	for (int i = 0; i < len; i++) {
		infile.read((char*)&byte, sizeof(byte));
		*(Values_table[type]+i)= (unsigned char)byte;
	}
}

void Decode::BuildHuff_and_Hash() {
	const unsigned char* every_code_len[4] = //每个编码表的不同码长下对应的码数量
	{ DC_Y_NRCodes,DC_C_NRCodes,AC_Y_NRCodes,AC_C_NRCodes };
	const unsigned char* start[4] =//每个编码表的不同码长对应的一组码的开始位置
	{ DC_Y_Values,DC_C_Values,AC_Y_Values,AC_C_Values };
	//type表示当前创建的编码表种类
	//HuffTable是装有4个编码表索引的大表
	for (int type = 0; type < 4; type++) {
		int index = 0;//在自己的表中对应的下标索引
		int code = 0;//编码
		//Code* p = HuffTable[type];
		for (int i = 0; i < 16; i++) {
			//cout << "now length=" << i + 1 << endl;
			for (int j = 0; j < *(every_code_len[type] + i); j++) {
				//没有长度为0的编码，码长从1开始
				//范式哈夫曼：同一长度的码长相邻只差1
				int value = (int)Values_table[type][index];
				string codestr = "";
				codestr = code_to_str(code, i + 1);
				//插入对应的哈希表
				FindinHuffTable[type].insert({codestr,value});

				code++;
				index++;
			}
			code <<= 1;//相邻码长：某长度开始的第一个编码等于上一个长度的最后一个编码左移一位
		}
	}
}

void Decode::MyDecode() {

	//读文件头
	if (readJPEGhead() == false) {
		cerr << "Failed to read！" << endl;
		return;
	}
	else
		cout << "Succeed to read！" << endl;

	BYTE* RGBdata = new(nothrow)BYTE[(int)img_width * (int)img_height * 4];
	if (!RGBdata) {
		cerr << "failed to allocate!" << endl;
		exit(-1);
	}
	data = RGBdata;

	cout << "Picture showing..." << endl;

	//重建赫夫曼树与哈希表
	BuildHuff_and_Hash();

	for (int x = 0; x < (int)img_height; x += 8) {
		for (int y = 0; y < (int)img_width; y += 8) {
			//还原出的zigzag完成后的待编码数组
			int Y_zdata[64] = { 0 }, Cb_zdata[64] = { 0 }, Cr_zdata[64] = { 0 };
			//还原出的zigzag以前的、DCT傅里叶变换以前的数组
			double Y[64] = { 0 }, Cb[64] = { 0 }, Cr[64] = { 0 };
			//一次解码一行，即一个8*8矩阵
			//Y
			readLine(Y_flag, Y_zdata, 0);
			reverseDCT(Y_flag, Y_zdata, Y);
			//cout << "你得到的dct之前的Y矩阵是：" << endl;
			//for (int i = 0; i < 64; i++) {
			//	cout << setw(10)<<Y[i] << ' ';
			//	if ((i + 1) % 8 == 0)
			//		cout << endl;
			//}
			//getchar();

			//Cb
			readLine(C_flag, Cb_zdata, 1);
			//cout << "你得到的zigzag之后的cb矩阵是：" << endl;
			//for (int i = 0; i < 64; i++) {
			//	cout << Cb_zdata[i] << ' ';
			//	if ((i + 1) % 8 == 0)
			//		cout << endl;
			//}
			//getchar();
			reverseDCT(C_flag, Cb_zdata, Cb);
			//Cr
			readLine(C_flag, Cr_zdata, 2);
			//cout << "你得到的dct之前的cr矩阵是：" << endl;
			//for (int i = 0; i < 64; i++) {
			//	cout << Cr_zdata[i] << ' ';
			//	if ((i + 1) % 8 == 0)
			//		cout << endl;
			//}
			//getchar();
			reverseDCT(C_flag, Cr_zdata, Cr);

			//转换回RGB三通道数组
			cgto_RGB(Y, Cb, Cr, x, y);
		}
	}


	imread.showPic(data, img_width, img_height);
	return;
}

//FLAG:0=y-dc,1=c-dc,2-y-ac,3=c-ac
void Decode::readLine(int FLAG,int zdata[],int preFlag) {
	int index = 0;//zdata数组的下标索引
	
	/****************DC解码*****************/
	//cout << "你即将开始DC解码：";
	//getchar();
	int combine = GetCode(FLAG);

	int zerocnt = combine / 16;//连续0的数量
	//cout << "你在DC部分读到的0个数=" << zerocnt;
	//getchar();
	int bit_len = combine % 16;//比特码的长度
	//cout << "你在DC部分读到的bit码长度=" << bit_len;
	//getchar();
	int value = 0;//哈夫曼编码对应的真正的原值
	while (zerocnt > 0) {
		zdata[index++] = 0;
		zerocnt--;
	}
	for (int i = 0; i < bit_len; i++) {
		value *= 2;
		value += read_next_bit();
		//cout << "当前在for循环中，value=" << dec<<value;
		//getchar();
	}//get real value in zdata[]
	//cout << "你算得的value=" <<dec<< value;
	//getchar();
	//判断正负数：正数的高位为1，所以一定大于2^(len-1)
	if (value >= pow(2, bit_len - 1))//positive
		preDC[preFlag] += value;
	else
		preDC[preFlag] += value - ((int)pow(2, bit_len) - 1);
	//cout << "你计算得到的DC原值=" << preDC[preFlag];
	//getchar();
	//DC存储编码的时候用的是2次差值，还原
	zdata[index++] = preDC[preFlag];
	/****************DC解码*****************/

	//cout << "你即将开始AC解码：";
	//getchar();
	/****************AC解码*****************/
	//两个特殊的其后没有bit码的哈夫曼编码
	int combine_EOB = 0, combine_FZR = 15 * 16 + 0;
	while (index < 64) {
		combine = GetCode(FLAG+2);
		if (combine == combine_EOB) {
			//cout << "你读到了EOB！";
			//getchar();
			while (index < 64) {
				zdata[index++] = 0;
			}
			break;
		}//读到EOB直接填满0，然后离开循环结束函数就可以
		else if (combine == combine_FZR) {
			//cout << "你读到了FZR!";
			//getchar();
			int fill_zero = 0;
			while (fill_zero < 16) {
				zdata[index++] = 0;
				fill_zero++;
			}
		}//读到FZR=(15,0)直接连续填16个0即可，
		else {
			//普通的AC编码，其解码过程和DC解码是完全相同的
			zerocnt = combine / 16;
			bit_len = combine % 16;
			//cout << "你在AC部分读到的0个数=" << zerocnt;
			//getchar();
			//cout << "你在AC部分读到的bit码长度=" << bit_len;
			//getchar();
			value = 0;
			while (zerocnt > 0) {
				zdata[index++] = 0;
				zerocnt--;
			}//add zero
			for (int i = 0; i < bit_len; i++) {
				//cout << "我们再算AC部分的真值";
				//getchar();
				value <<= 1;
				value += read_next_bit();
			}//get real value in zdata[]
			//cout << "你算的AC部分的真值=" << value;
			//getchar();
			//判断正负数：正数的高位为1，所以一定大于2^(len-1)
			if (value >= (int)pow(2, bit_len - 1))
				zdata[index++] = value;
			else
				zdata[index++] = value - ((int)pow(2, bit_len) - 1);
		}
	}
}

//在文件中读取到一个编码并返回该编码对应的实际值
int Decode::GetCode(int FLAG) {
	bool toRead = true;
	int length = 0;
	int code = 0;
	int value = 0;//返回值：查询到的码所对应的实际值
	string tosearch_code = "";
	while (toRead) {
		//cout << "这里是toRead循环的read_next_bit" << endl;
		int bit= read_next_bit();
		code <<= 1;
		code += bit;//读取文件中的下一个比特位
		length++;
		tosearch_code= code_to_str(code, length);
		//cout << "当前我们生成的str=" << tosearch_code;
		//getchar();

		//在哈希中寻找这个键值
		auto myCode = FindinHuffTable[FLAG].find(tosearch_code);
		if (myCode != FindinHuffTable[FLAG].end()) {
			//cout << "找到了！读到的是" << endl;
			//cout << "code=" << hex << code << endl;
			//cout << "value=" << myCode->second << endl;
			//cout << tosearch_code << endl;
			//getchar();

			toRead = false;
			value = myCode->second;
		}//if found
	}
	return value;
}

//在文件中向下读取1个比特位
int Decode::read_next_bit() {
	//如果上一次读取到的byte还没用完则不需要进入这个分支
	if (bitcount == 0) {//byte中已经没有余量，继续从文件中读
		infile.read((char*)&byte, sizeof(byte));
		bitcount = 8;//重置可以数的的比特位
		if (byte == (char)0xff) {
			char zero = 0;
			infile.read((char*)&zero, sizeof(zero));
			//FF后面跟着的00需要直接吞掉
		}
	}
	int bit = byte & 128;
	//cout << "你当前在readbit函数里得到的bit=" << (bool)bit;
	//getchar();
	byte <<= 1;//每次都是按位与比较byte的最高位，为当前在文件中读取到的比特位
	bitcount--;//读完1个，比特计数量-1
	if (bit)
		return 1;
	else
		return 0;
}

void Decode::reverseDCT(int FLAG,int zdata[], double YCbCr[]) {
	double before_zigzag[64] = { 0 };
	//首先反zigzag
	for (int i = 0; i < 64; i++)
		before_zigzag[i] = zdata[ZigzagMat[i]];
	
	//反量化
	unsigned char* QTable = NULL;
	if (FLAG == Y_flag)
		QTable = Luminance_Quantization_Table;
	else
		QTable = Chrominance_Quantization_Table;
	for (int i = 0; i < 64; i++)
		before_zigzag[i] *= QTable[i];

	//反DCT
	const double pi = 3.1415926;
	for (int x = 0; x < 8; x++) {
		for (int y = 0; y < 8; y++) {
			//每一项暂记为tmp
			double tmp = 0;
			//此处作双层的累加
			for (int u = 0; u < 8; u++) {
				for (int v = 0; v < 8; v++) {
					//两个C系数
					double c_u = (u == 0) ? 1.f / sqrt(8.f) : 0.5f;;
					double c_v = (v == 0) ? 1.f / sqrt(8.f) : 0.5f;;
					//当前before_zigzag数组中对应的值
					double cur_data = before_zigzag[u * 8 + v];
					//求DCT反变换的基变换系数公式
					cur_data *= cos((x + 0.5) * pi * u / 8) * cos((y + 0.5) * pi * v / 8) * c_u * c_v;
					tmp += cur_data;//做累加
				}
			}
			YCbCr[x * 8 + y] = tmp;
		}
	}
}

//将YCbCr数组转化为RGB
void Decode::cgto_RGB(double Y[], double Cb[], double Cr[],int x,int y) {
	//(x,y)是矩阵左上角第一个点的像素位置
	int first = x * img_width + y;//first是第一个点在所有像素中的位序
	int index = 0;;
	int i = 0;
	//将1个8*8矩阵对应的ycbcr值还原回rgb
	for (int x = 0; x < 8; x++) {//row
		for (int y = 0; y < 8; y++) {//col
			//在data中的索引
			index = (first + x * img_width + y)*4;
			i = x * 8 + y;//在ycbcr矩阵中的索引
			int R = (int)round(1.000200*(Y[i]+128) - 0.000992 * Cb[i]+1.402126*Cr[i]);
			int G = (int)round(1.000200*(Y[i]+128) - 0.345118 * Cb[i] - 0.714010 * Cr[i]);
			int B = (int)round(1.000200*(Y[i]+128) + 1.771018 * Cb[i]+0.000143*Cr[i]);
			//处理RGB计算值的上下界溢出
			if (R < 0)R = 0;
			if (G < 0)G = 0;
			if (B < 0)B = 0;
			if (R > 255)R = 255;
			if (G > 255)G = 255;
			if (B > 255)B = 255;

			//cout << "index=" << index << endl;
			//cout << "R=" << R << " G=" << G << " B=" << B << endl;
		
			data[index] = (BYTE)R;
			data[index+1] = (BYTE)G;
			data[index+2] = (BYTE)B;
			////RGBA的A
			data[index+3] = 255;
		}
	}
}

//将从文件中读到的高低位倒序的数字正序计算出来并返回
int Decode::cgto_num(short word) {
	int num_low= word >> 8;
	int num_high = (int)((char)word);
	int num = (num_high << 8) + num_low;
	return num;
}
//将计算出的code值转换为string类
string Decode::code_to_str(int code,int code_len) {
	string str = "";
	int bit;
	//按位与的遮罩
	int mask[] = { 1,2,4,8,16,32,64,128,256,512,1024,2048,4096,8192,16384,32768 };
	for (int i = 0; i < code_len; i++) {
		bit = code & mask[code_len - i - 1];
		if (bit)
			str += '1';
		else
			str += '0';
	}
	return str;
}