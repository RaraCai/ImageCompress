/*����� 2252429 ������*/
#include "PicReader.h"
#include "Compress.h"
#include "Decode.h"


//��һ��8*8���ؾ����Ӧ��ycbcr����
void JPEG::cgto_YCbCr(BYTE* RGBbuffer, double Ydata[], double Cbdata[], double Crdata[]) {
	//index�������RGBbuffer��ƫ����
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
/**********��һ��8 * 8����Mat������Ҷ�任������***********/
//data[]ԭʼ����
		//dct_data����DCT�任��zigzag������һά����
		//QuantTable������Y��CbCr�в�ͬ������������
void JPEG:: cgto_DCT(double data[], int dct_data[], const unsigned char QuantTable[]) {
	//DCT�任�ı�����data[]�������任ת����dct_data[]
	//dct_data[]����64����׼ͼ��Ϊ��ʱ��Ӧ��ϵ��
	const double pi = 3.1415926;//����pai��ȡֵ
	for (int u = 0; u < 8; u++) {
		for (int v = 0; v < 8; v++) {
			//�����Ƕ�ά����2��alphaϵ��
			double alpha_u = (u == 0) ? 1.f / sqrt(8.f) : 0.5f;
			double alpha_v = (v == 0) ? 1.f / sqrt(8.f) : 0.5f;

			//ϵ������δ����ǰ��ֵ��¼����ʱ����tmp��
			double tmp = 0;
			//�˴���˫����ۼ�
			for (int x = 0; x < 8; x++) {
				for (int y = 0; y < 8; y++) {
					//��ǰdata�еĶ�Ӧ�ľ���Ԫ��ֵ
					double cur_data = data[x * 8 + y];
					//��DCT���任ϵ���Ĺ�ʽ
					cur_data *= cos((2 * x + 1) * u * pi / 16.f) * cos((2 * y + 1) * v * pi / 16.f);
					tmp += cur_data;//���ۼ�
				}
			}
			tmp *= alpha_u * alpha_v;
			////tmp��ֵ����ȡ��
			int tmp_c = (int)round(tmp / (double)QuantTable[u * 8 + v]);//�����������Ӧλ�õ�ϵ������
			////ֱ��zigzag����dct_data[]
			short ZigzagIndex = ZigzagMat[u * 8 + v];
			dct_data[ZigzagIndex] = tmp_c;
		}
	}
}

//����Huffman
void JPEG::BuildHuffTable() {
	const unsigned char* every_code_len[4] = //ÿ�������Ĳ�ͬ�볤�¶�Ӧ��������
	{ DC_Y_NRCodes,DC_C_NRCodes,AC_Y_NRCodes,AC_C_NRCodes };
	const unsigned char* start[4] =//ÿ�������Ĳ�ͬ�볤��Ӧ��һ����Ŀ�ʼλ��
	{ DC_Y_Values,DC_C_Values,AC_Y_Values,AC_C_Values };
	//type��ʾ��ǰ�����ı��������
	//HuffTable��װ��4������������Ĵ��
	for (int type = 0; type < 4; type++) {
		int index = 0;//���Լ��ı��ж�Ӧ���±�����
		int code = 0;//����
		Code* p = HuffTable[type];
		for (int i = 0; i < 16; i++) {
			//cout << "now length=" << i + 1 << endl;
			for (int j = 0; j < *(every_code_len[type] + i); j++) {
				//û�г���Ϊ0�ı��룬�볤��1��ʼ
				//��ʽ��������ͬһ���ȵ��볤����ֻ��1
				
				p[ValuesPtr[type][index]].code = code;
				//cout << "code=" << code << endl;
				p[ValuesPtr[type][index]].length = i + 1;
				code++;
				index++;
			}
			code <<= 1;//�����볤��ĳ���ȿ�ʼ�ĵ�һ�����������һ�����ȵ����һ����������һλ
		}
	}
}

/**************************bit����********************************/
//����γ�RLE�������ӦAC��DC���ֵ���ֵת��
//int eob��ʾEOB��data�е�λ��
//AC����
void JPEG::AC_bitcnt(int TABLEFLAG, int eob, int dct_z_data[], Code* HuffCode, int& index) {
	//�ж��Ƿ��ǣ�15,0���ı�־λ,15*16+0=0xf0=240
	bool FZR = false;
	Code combine_FZR= *(AC_table[TABLEFLAG] + 0xf0);//huffcode for (15,0)FZR

	//��¼����0������
	int zerocnt = 0;
	int i = 1;
	//����ֻ�ܲ����������±�eob��ǰ
	while(i<eob){

		//��ǰ������ֵ��dct_z_data[i]

		//��¼����0
		while (dct_z_data[i] == 0) {
			//cout << i << endl;
			zerocnt++;
			if (i >= eob) {
				i--; 
				zerocnt--;
				break;
			}
			//����������16��0��������
			if (zerocnt >= 16) {
				FZR = true;
				break;
			}//�ж�FZR
			i++;
		}// counting num of zero
		if (FZR) {
			//ֻ�洢FZR���ɣ�����洢bit��
			HuffCode[index] = combine_FZR;
			index++;

			////test
			//cout << "��ǰ���AC����Huffman��Ϊ��";
			//printbin(HuffCode[index - 1].code, HuffCode[index - 1].length);
			//getchar();
			//cout << "����FZR����bit��";
			//getchar();
			////test over


			zerocnt = 0;//����������
			FZR = false;//��־λ����
		}//FZR
		else {
			//dct_z_data�ľ���ֵ
			int abs_val = (int)fabs(dct_z_data[i]);
			Code BitCode = { 0 };
			BitCode.length = 0; BitCode.code = 0;
			//����abs_val�Ķ�����������
			while (abs_val > 0) {
				abs_val >>= 1;
				BitCode.length++;
			}//length of abs_val=length of bitcode
			abs_val = (int)fabs(dct_z_data[i]);//��ԭabs_val

			//���ȱ��������
			unsigned char combine = zerocnt * 16 + BitCode.length;
			HuffCode[index] = *(AC_table[TABLEFLAG] + combine);
			index++;

			////test
			//cout << "��ǰ���AC����Huffman��Ϊ��";
			//printbin(HuffCode[index - 1].code, HuffCode[index - 1].length);
			//getchar();
			////test over


			//������
			if (dct_z_data[i] > 0) {//����
				//BitCode��ֵ���䱾��
				BitCode.code = dct_z_data[i];
				HuffCode[index] = BitCode;
				index++; 

				////test
				//cout << "��ǰ���AC����bit��Ϊ��";
				//printbin(HuffCode[index - 1].code, HuffCode[index - 1].length);
				//getchar();
				////test over

				zerocnt = 0;
			}
			else if (abs_val != dct_z_data[i]) {//����
				BitCode.code = (int)pow(2, BitCode.length) - 1 +dct_z_data[i];
				HuffCode[index] = BitCode;
				index++;

				////test
				//cout << "��ǰ���AC����bit��Ϊ��";
				//printbin(HuffCode[index - 1].code, HuffCode[index - 1].length);
				//getchar();
				////test over

				zerocnt = 0;
			}
		}
		i++;
	}
}	
//DC����
void JPEG::DC_bitcnt(int TABLEFLAG, int dct_z_data, Code HuffCode[], int& index, int PREFLAG) {

	//���㱾�ε�DC����һ��DC�Ĳ�ֵ
	int difference = dct_z_data - preDC[PREFLAG];
	preDC[PREFLAG] = dct_z_data;//����pre

	//difference�ľ���ֵ
	int abs_val = (int)fabs(difference);
	//bit��
	Code BitCode = { 0 };
	//����abs_val�Ķ�����������
	while (abs_val > 0) {
		abs_val >>= 1;
		BitCode.length++;
	}//length of abs_val=length of bitcode

	//��һλ�ĺϲ��ַ�,��������
	unsigned char combine = 16 * 0 + BitCode.length;
	

	//difference��0�ϲ���Ϊ��combine��Ϊ��һ����������
	HuffCode[index] = *(DC_table[TABLEFLAG] + combine);
	index++;//Huffman

	////test
	//cout << "��ǰ���DC����Huffman��Ϊ��";
	//printbin(HuffCode[index - 1].code, HuffCode[index - 1].length);
	//getchar();
	////test over



	if (difference != 0) {//�ڶ�λ�洢�����루ֻ��difference=�������Ŵ棩
		abs_val = (int)fabs(difference);
		if (difference > 0) {//����
			//BitCode��ֵ���䱾��
			BitCode.code = difference;
		}
		else if (abs_val != difference) {//����
			BitCode.code = (int)pow(2, BitCode.length) - 1 + difference;
		}
		HuffCode[index] = BitCode;
		index++;
		////test
		//cout << "��ǰ���DC����bit��Ϊ��";
		//printbin(HuffCode[index - 1].code, HuffCode[index - 1].length);
		//getchar();
		////test over
	}
	else {
		//cout << "DC data=0,��bit��";
		//getchar();
		;
	}
}


//TABLEFLAG:��ǵ�ǰ��Y����C��
//dct_data���Ѿ�������zigzag������
int JPEG::Huff_encoding(int TABLEFLAG,int dct_z_data[],Code HuffCode[],int PREFLAG) {
	int index = 0;//��ǰ����±�����
	/******************************DC����*********************************/
	DC_bitcnt(TABLEFLAG, dct_z_data[0], HuffCode, index, PREFLAG);
	/******************************DC����*********************************/

	/******************************AC����*********************************/
	//�ҵ�dct_data��EOB��λ��
	int i = 63;
	while (dct_z_data[i] == 0)
		i--;
	int eob = ++i;//�ҵ�ĩβ��һ��0���ֵ�λ��
	//����eob֮ǰ��AC����
	if (eob > 1) {
		AC_bitcnt(TABLEFLAG, eob, dct_z_data, HuffCode, index);
	}
	//д��EOB
	Code EOB = AC_table[TABLEFLAG][0x00];//EOB->0
	if (eob <= 63) {
		HuffCode[index] = EOB;
		index++;
		//cout << "��ǰ����EOB��";
		//printbin(HuffCode[index - 1].code, HuffCode[index - 1].length);
		//getchar();
	}
	/******************************AC����*********************************/
	//cout << "Huff ok!" << endl;
	return index;//������������ĳ���
}
void JPEG:: write_bits(Code HuffCode[],const int index) {
	int lala = 0;
	//cout << "writing..." << endl;
	for (int i = 0; i < index; i++) {
		int code = HuffCode[i].code;
		int length = HuffCode[i].length;
		while (length>0) {
			//ÿ��8λ���1���ֽ�
			unsigned char bit = (unsigned char)GetBit(code, length);//��ǰλ����code�ı���λֵ
			//cout << (int)bit;
			byte <<= 1;//������ַ�����1λ
			if (bit)
				byte |= 1;
			bitcount++;
			length--;//�Ѿ�����򳤶�-1����length��Ϊ0ʱһ��ѭ����������������һ������
			if (bitcount == 8) {
				outstr += byte;			
				//cout << "��8λ����ǰ��ӡ���ǣ�0x" << hex << setw(2) << (unsigned int)byte << endl;
				//��JPEG��FF�������
				if (byte == (char)0xff) {
					outstr += '\0';
					//cout << "��һ����0xff����Ҫ��β0��" << "0x00" << endl;
				}
				//���
				byte = 0;
				bitcount = 0;
			}
		}
		//cout << "һ���洢��code�Ѿ�get��ϣ�" << endl;
	}
	//getchar();
}
void JPEG:: MyCompress() {
	//��������д���ļ�
	outfile.open("lena.jpg", ios::binary);
	if (!outfile) {
		cerr << "failed to open output file!" << endl;
		exit(-1);
	}
	//д���ļ�ͷ
	writeJPEGhead(outfile);
	//���ɹ�������
	BuildHuffTable();
	//��ʼ����
	//ÿ8*8����������һ��
	for (unsigned int x = 0; x < img_height; x += 8) {
		for (unsigned int y = 0; y < img_width; y += 8) {
			/*cout << "�����ǵ�"<<times << "��8*8����" << endl;*/
			//��ǰ8*8�������Ͻ����ص����ڵ������±�
			BYTE* RGBbuffer = data + x * img_width * 4 + y * 4;//ÿ�����ص㶼��rgba�ĸ�ֵ
			//��RGBת��Ϊycbcr��������
			double Y_data[64] = { 0 }, Cb_data[64] = { 0 }, Cr_data[64] = { 0 };
			cgto_YCbCr(RGBbuffer, Y_data, Cb_data, Cr_data);
			//DCT
			int Y_dct[64] = { 0 }, Cb_dct[64] = { 0 }, Cr_dct[64] = { 0 };
			int index=0;
			//ѹ��Yͨ��
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

			//Yͨ����Huffman����
			Code* HuffCode_Y = new(nothrow)Code[128];//�����64����ÿһ�����ֳ�2����
			if (!HuffCode_Y) {
				cerr << "failed to allocate!" << endl;
				exit(-1);
			}
			index=Huff_encoding(Y_flag, Y_dct, HuffCode_Y,Y_flag);//index?????????????
			//cout << endl << " Y- encoding is over! now let's write!";
			//getchar();
			write_bits(HuffCode_Y,index);
			delete[] HuffCode_Y;


			//ѹ��Cbͨ��
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


			//ѹ��Crͨ��
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
	//��λ�������
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
//д�ļ�ͷ
void JPEG::writeJPEGhead(ofstream& outfile){
	//SOI
	write_word(0xffd8, outfile);
	//APPO
	write_word(0xffe0, outfile);
	write_word(16, outfile);//���ݳ���16
	//��ʶ��"JFIF"
	char JFIF_str[5] = "JFIF";
	outfile.write((char*)JFIF_str, 5);

	//�汾
	unsigned char version = 1;
	outfile.write((char*)&version, sizeof(version));
	//�汾
	outfile.write((char*)&version, sizeof(version));

	outfile << (unsigned char)0;//X��Y���ܶȵ�λ
	write_word(1, outfile);//X���������ܶ�
	write_word(1, outfile);//Y���������ܶ�
	write_word(0, outfile);//����ͼˮƽ�ʹ�ֱ������Ŀ��������ͼ
	
	
	//DQT
	write_word(0xffdb, outfile);//��ʶ��
	write_word(132, outfile);//���ݳ���
	//���Ⱥ�����
	outfile.put(0);
	//zigzagд���һ��������-Y
	unsigned char Qtable[64] = { 0 };
	for (int i = 0; i < 64; i++) 
		Qtable[ZigzagMat[i]] = Luminance_Quantization_Table[i];
	outfile.write((char*)Qtable, 64);


	outfile.put(1);
	//zigzagд��ڶ���������CbCr
	for (int i = 0; i < 64; i++)
		Qtable[ZigzagMat[i]] = Chrominance_Quantization_Table[i];
	outfile.write((char*)Qtable, 64);

	//SOF0
	write_word(0xffc0, outfile);//��Ƿ�
	write_word(17, outfile);//���ݳ���
	//ͼ�񾫶�
	outfile.put(8);
	write_word(img_height & 0xffff, outfile);//ͼ��߶�
	write_word(img_width & 0xffff, outfile);//ͼ����


	//��ɫ��������Ϊ3
	outfile.put(3);
	//3����ɫ����YCbCr
	//Y
	outfile.put(1);//��ɫ����ID-1-Y��־
	outfile.put(0x11);//ˮƽ�ʹ�ֱ��������
	outfile.put(0);//��ǰ����ʹ�õ�������ID YTable id=0
	//Cb
	outfile.put(2);//��ɫ����ID-2-Cb
	outfile.put(0x11);//ˮƽ�ʹ�ֱ��������
	outfile.put(1);//��ǰ����ʹ�õ�������ID CTable id=1

	//Cr
	outfile.put(3);//��ɫ����ID-2-Cb
	outfile.put(0x11);//ˮƽ�ʹ�ֱ��������
	outfile.put(1);//��ǰ����ʹ�õ�������ID CTable id=1


	//DHT
	write_word(0xffc4, outfile);//��ʶ��
	//����DHT�����ݳ���
	int len[4] = { 0 };
	for (int i = 0; i < 16; i++) {
		len[0] += DC_Y_NRCodes[i];
		len[1] += DC_C_NRCodes[i];
		len[2] += AC_Y_NRCodes[i];
		len[3] += AC_C_NRCodes[i];
	}
	int length = len[0] + len[1] + len[2] + len[3] + 2 + 4 + 4 * 16;
	//���ݳ��ȼ������
	write_word(length, outfile);//д�����ݳ���
	
	//��Y��CbCr�ֱ��DC,AC��
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
	write_word(0xffda, outfile);//��ʶ��
	write_word(12, outfile);//���ݳ���
	//��ɫ������Ycbcr=3
	outfile.put(3);

	//��ɫ������Ϣ
	outfile.put(1); outfile.put(0x00);//y
	outfile.put(2); outfile.put(0x11);//cb
	outfile.put(3); outfile.put(0x11);//cr

	//ѹ��ͼ������
	outfile.put(0x00);//��ѡ��ʼ
	outfile.put(0x3f);//��ѡ�����
	outfile.put(0x00);//��ѡ��

}
//д�ļ�β
void JPEG::writeJPEGtail(ofstream& outfile) {
	if (bitcount > 0 && bitcount < 8) {
		byte <<= (8 - bitcount);
		outstr += byte;
	}
	outfile << outstr;
	//EOI
	write_word(0xffd9, outfile);
}


/****************************���벿��******************************/
//��ȡ�ļ�ͷ�洢4��������AC DC���Լ�ͼƬ�Ļ�����Ϣ������ֵΪ�Ƿ��ȡ�ɹ�
bool Decode::readJPEGhead() {
	infile.open("lena.jpg", ios::binary);
	if (!infile) {
		cerr << "failed to open compressed file!" << endl;
		return false;
	}
	//ֻ��ҪͼƬ�Ļ�����Ϣ��JPEG��ʽ���ļ�ͷʵ����Ҫ
	//���ﶨ�岻ͬ���ȵĻ���䣬�����ȡ
	unsigned char byte=0;//1�ֽ�
	unsigned short word=0;//��=2�ֽ�
	unsigned int dword=0;//˫��=4�Ľ�
	//�������������ѹ����ʱ��û�뵽�������ʱ���뵽�ˡ���

	//��ȡSOI
	infile.read((char*)&word, sizeof(word));//0xffd8
	//��ȡAPPO
	infile.read((char*)&dword, sizeof(dword));//0xffe0,0x16
	//��ȡ��ʶ��JFIF
	char JFIF[5] = { 0 };
	infile.read(JFIF, 5);
	//�汾
	infile.read((char*)&byte, sizeof(byte));//version1
	infile.read((char*)&byte, sizeof(byte));//version2
	//X,Y�ܶȵ�λ
	infile.read((char*)&byte, sizeof(byte));
	infile.read((char*)&word, sizeof(word));//X���������ܶ�
	infile.read((char*)&word, sizeof(word));//Y���������ܶ�
	infile.read((char*)&word, sizeof(word));//����ͼˮƽ�ʹ�ֱ������Ŀ��������ͼ

	//��ȡDQT
	infile.read((char*)&word, sizeof(word));//��ʶ��
	infile.read((char*)&word, sizeof(word));//���ݳ���

	//���Ⱥ�����
	infile.read((char*)&byte, sizeof(byte));//0
	unsigned char Qtable_z[64] = { 0 };//�ݴ��ȡ����zigzag֮���������
	//��zigzag��ԭY������
	for (int i = 0; i < 64; i++) {
		infile.read((char*)&byte, sizeof(byte));
		Qtable_z[i] = (unsigned char)byte;
	}
	//��zigzag��ֵ��Y������
	for (int i = 0; i < 64; i++)
		Luminance_Quantization_Table[i] = Qtable_z[ZigzagMat[i]];


	infile.read((char*)&byte, sizeof(byte));//1
	//��zigzag��ԭC������
	for (int i = 0; i < 64; i++) {
		infile.read((char*)&byte, sizeof(byte));
		Qtable_z[i] = (unsigned char)byte;
	}
	//��zigzag��ֵ��C������
	for (int i = 0; i < 64; i++)
		Chrominance_Quantization_Table[i] = Qtable_z[ZigzagMat[i]];

	//��ȡSOFO
	infile.read((char*)&word, sizeof(word));//0xffc0
	infile.read((char*)&word, sizeof(word));//length of data=17
	//ͼ�񾫶�
	infile.read((char*)&byte, sizeof(byte));//8
	infile.read((char*)&word, sizeof(word));//ͼ��߶�
	img_height = (UINT)cgto_num(word);
	
	infile.read((char*)&word, sizeof(word));//ͼ����
	img_width = (UINT)cgto_num(word);


	//��ɫ����YCbCr
	infile.read((char*)&byte, sizeof(byte));//��ɫ������=3
	//Y
	infile.read((char*)&byte, sizeof(byte)); //��ɫ����ID-1-Y��־
	infile.read((char*)&byte, sizeof(byte));//ˮƽ�ʹ�ֱ��������0x11
	infile.read((char*)&byte, sizeof(byte));//��ǰ����ʹ�õ�������ID YTable id=0
	//Cb
	infile.read((char*)&byte, sizeof(byte)); //��ɫ����ID-2-Cb��־
	infile.read((char*)&byte, sizeof(byte));//ˮƽ�ʹ�ֱ��������0x11
	infile.read((char*)&byte, sizeof(byte));//��ǰ����ʹ�õ�������ID CTable id=1
	//Cr
	infile.read((char*)&byte, sizeof(byte)); //��ɫ����ID-2-Cr��־
	infile.read((char*)&byte, sizeof(byte));//ˮƽ�ʹ�ֱ��������0x11
	infile.read((char*)&byte, sizeof(byte));//��ǰ����ʹ�õ�������ID CTable id=1

	//��ȡDHT
	infile.read((char*)&word, sizeof(word));//��ʶ��0xffc4
	infile.read((char*)&word, sizeof(word));//���ݳ���

	//��ȡ4��������ACDC��
	for (int i = 0; i < 4; i++) {
		//��ǰ��ı�ʶ�Ǻ�type=0~3
		//�ֱ��Ӧ:0-y_dc,1-c_dc,2-y_ac,3-c_ac
		infile.read((char*)&byte, sizeof(byte));
		unsigned char tag = (unsigned char)byte;
		readHuffTable(tag);//����ACDC��
	}
	//��ȡSOS
	infile.read((char*)&word, sizeof(word));//��ʶ��0xffda
	infile.read((char*)&word, sizeof(word));//���ݳ���12

	infile.read((char*)&byte, sizeof(byte));//��ɫ������ycbcr=3

	//��ɫ������Ϣ
	infile.read((char*)&word, sizeof(word));//y
	infile.read((char*)&word, sizeof(word));//cb
	infile.read((char*)&word, sizeof(word));//cr

	//ѹ��ͼ������
	infile.read((char*)&byte, sizeof(byte));//��ѡ��ʼ
	infile.read((char*)&byte, sizeof(byte));//��ѡ�����
	infile.read((char*)&byte, sizeof(byte));//��ѡ��

	return true;
}
void Decode::readHuffTable(int tag) {
	int len = 0;
	unsigned char nrc_codes[16] = { 0 };
	infile.read((char*)nrc_codes, 16);
	int type = 2 * (tag / 16) + tag % 16;

	for (int i = 0; i < 16; i++) {
		len += nrc_codes[i];//��values���������
		*(NRCodes_table[type] + i) = nrc_codes[i];//�洢NRCodes����
	}
	//�洢values����
	for (int i = 0; i < len; i++) {
		infile.read((char*)&byte, sizeof(byte));
		*(Values_table[type]+i)= (unsigned char)byte;
	}
}

void Decode::BuildHuff_and_Hash() {
	const unsigned char* every_code_len[4] = //ÿ�������Ĳ�ͬ�볤�¶�Ӧ��������
	{ DC_Y_NRCodes,DC_C_NRCodes,AC_Y_NRCodes,AC_C_NRCodes };
	const unsigned char* start[4] =//ÿ�������Ĳ�ͬ�볤��Ӧ��һ����Ŀ�ʼλ��
	{ DC_Y_Values,DC_C_Values,AC_Y_Values,AC_C_Values };
	//type��ʾ��ǰ�����ı��������
	//HuffTable��װ��4������������Ĵ��
	for (int type = 0; type < 4; type++) {
		int index = 0;//���Լ��ı��ж�Ӧ���±�����
		int code = 0;//����
		//Code* p = HuffTable[type];
		for (int i = 0; i < 16; i++) {
			//cout << "now length=" << i + 1 << endl;
			for (int j = 0; j < *(every_code_len[type] + i); j++) {
				//û�г���Ϊ0�ı��룬�볤��1��ʼ
				//��ʽ��������ͬһ���ȵ��볤����ֻ��1
				int value = (int)Values_table[type][index];
				string codestr = "";
				codestr = code_to_str(code, i + 1);
				//�����Ӧ�Ĺ�ϣ��
				FindinHuffTable[type].insert({codestr,value});

				code++;
				index++;
			}
			code <<= 1;//�����볤��ĳ���ȿ�ʼ�ĵ�һ�����������һ�����ȵ����һ����������һλ
		}
	}
}

void Decode::MyDecode() {

	//���ļ�ͷ
	if (readJPEGhead() == false) {
		cerr << "Failed to read��" << endl;
		return;
	}
	else
		cout << "Succeed to read��" << endl;

	BYTE* RGBdata = new(nothrow)BYTE[(int)img_width * (int)img_height * 4];
	if (!RGBdata) {
		cerr << "failed to allocate!" << endl;
		exit(-1);
	}
	data = RGBdata;

	cout << "Picture showing..." << endl;

	//�ؽ��շ��������ϣ��
	BuildHuff_and_Hash();

	for (int x = 0; x < (int)img_height; x += 8) {
		for (int y = 0; y < (int)img_width; y += 8) {
			//��ԭ����zigzag��ɺ�Ĵ���������
			int Y_zdata[64] = { 0 }, Cb_zdata[64] = { 0 }, Cr_zdata[64] = { 0 };
			//��ԭ����zigzag��ǰ�ġ�DCT����Ҷ�任��ǰ������
			double Y[64] = { 0 }, Cb[64] = { 0 }, Cr[64] = { 0 };
			//һ�ν���һ�У���һ��8*8����
			//Y
			readLine(Y_flag, Y_zdata, 0);
			reverseDCT(Y_flag, Y_zdata, Y);
			//cout << "��õ���dct֮ǰ��Y�����ǣ�" << endl;
			//for (int i = 0; i < 64; i++) {
			//	cout << setw(10)<<Y[i] << ' ';
			//	if ((i + 1) % 8 == 0)
			//		cout << endl;
			//}
			//getchar();

			//Cb
			readLine(C_flag, Cb_zdata, 1);
			//cout << "��õ���zigzag֮���cb�����ǣ�" << endl;
			//for (int i = 0; i < 64; i++) {
			//	cout << Cb_zdata[i] << ' ';
			//	if ((i + 1) % 8 == 0)
			//		cout << endl;
			//}
			//getchar();
			reverseDCT(C_flag, Cb_zdata, Cb);
			//Cr
			readLine(C_flag, Cr_zdata, 2);
			//cout << "��õ���dct֮ǰ��cr�����ǣ�" << endl;
			//for (int i = 0; i < 64; i++) {
			//	cout << Cr_zdata[i] << ' ';
			//	if ((i + 1) % 8 == 0)
			//		cout << endl;
			//}
			//getchar();
			reverseDCT(C_flag, Cr_zdata, Cr);

			//ת����RGB��ͨ������
			cgto_RGB(Y, Cb, Cr, x, y);
		}
	}


	imread.showPic(data, img_width, img_height);
	return;
}

//FLAG:0=y-dc,1=c-dc,2-y-ac,3=c-ac
void Decode::readLine(int FLAG,int zdata[],int preFlag) {
	int index = 0;//zdata������±�����
	
	/****************DC����*****************/
	//cout << "�㼴����ʼDC���룺";
	//getchar();
	int combine = GetCode(FLAG);

	int zerocnt = combine / 16;//����0������
	//cout << "����DC���ֶ�����0����=" << zerocnt;
	//getchar();
	int bit_len = combine % 16;//������ĳ���
	//cout << "����DC���ֶ�����bit�볤��=" << bit_len;
	//getchar();
	int value = 0;//�����������Ӧ��������ԭֵ
	while (zerocnt > 0) {
		zdata[index++] = 0;
		zerocnt--;
	}
	for (int i = 0; i < bit_len; i++) {
		value *= 2;
		value += read_next_bit();
		//cout << "��ǰ��forѭ���У�value=" << dec<<value;
		//getchar();
	}//get real value in zdata[]
	//cout << "����õ�value=" <<dec<< value;
	//getchar();
	//�ж��������������ĸ�λΪ1������һ������2^(len-1)
	if (value >= pow(2, bit_len - 1))//positive
		preDC[preFlag] += value;
	else
		preDC[preFlag] += value - ((int)pow(2, bit_len) - 1);
	//cout << "�����õ���DCԭֵ=" << preDC[preFlag];
	//getchar();
	//DC�洢�����ʱ���õ���2�β�ֵ����ԭ
	zdata[index++] = preDC[preFlag];
	/****************DC����*****************/

	//cout << "�㼴����ʼAC���룺";
	//getchar();
	/****************AC����*****************/
	//������������û��bit��Ĺ���������
	int combine_EOB = 0, combine_FZR = 15 * 16 + 0;
	while (index < 64) {
		combine = GetCode(FLAG+2);
		if (combine == combine_EOB) {
			//cout << "�������EOB��";
			//getchar();
			while (index < 64) {
				zdata[index++] = 0;
			}
			break;
		}//����EOBֱ������0��Ȼ���뿪ѭ�����������Ϳ���
		else if (combine == combine_FZR) {
			//cout << "�������FZR!";
			//getchar();
			int fill_zero = 0;
			while (fill_zero < 16) {
				zdata[index++] = 0;
				fill_zero++;
			}
		}//����FZR=(15,0)ֱ��������16��0���ɣ�
		else {
			//��ͨ��AC���룬�������̺�DC��������ȫ��ͬ��
			zerocnt = combine / 16;
			bit_len = combine % 16;
			//cout << "����AC���ֶ�����0����=" << zerocnt;
			//getchar();
			//cout << "����AC���ֶ�����bit�볤��=" << bit_len;
			//getchar();
			value = 0;
			while (zerocnt > 0) {
				zdata[index++] = 0;
				zerocnt--;
			}//add zero
			for (int i = 0; i < bit_len; i++) {
				//cout << "��������AC���ֵ���ֵ";
				//getchar();
				value <<= 1;
				value += read_next_bit();
			}//get real value in zdata[]
			//cout << "�����AC���ֵ���ֵ=" << value;
			//getchar();
			//�ж��������������ĸ�λΪ1������һ������2^(len-1)
			if (value >= (int)pow(2, bit_len - 1))
				zdata[index++] = value;
			else
				zdata[index++] = value - ((int)pow(2, bit_len) - 1);
		}
	}
}

//���ļ��ж�ȡ��һ�����벢���ظñ����Ӧ��ʵ��ֵ
int Decode::GetCode(int FLAG) {
	bool toRead = true;
	int length = 0;
	int code = 0;
	int value = 0;//����ֵ����ѯ����������Ӧ��ʵ��ֵ
	string tosearch_code = "";
	while (toRead) {
		//cout << "������toReadѭ����read_next_bit" << endl;
		int bit= read_next_bit();
		code <<= 1;
		code += bit;//��ȡ�ļ��е���һ������λ
		length++;
		tosearch_code= code_to_str(code, length);
		//cout << "��ǰ�������ɵ�str=" << tosearch_code;
		//getchar();

		//�ڹ�ϣ��Ѱ�������ֵ
		auto myCode = FindinHuffTable[FLAG].find(tosearch_code);
		if (myCode != FindinHuffTable[FLAG].end()) {
			//cout << "�ҵ��ˣ���������" << endl;
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

//���ļ������¶�ȡ1������λ
int Decode::read_next_bit() {
	//�����һ�ζ�ȡ����byte��û��������Ҫ���������֧
	if (bitcount == 0) {//byte���Ѿ�û���������������ļ��ж�
		infile.read((char*)&byte, sizeof(byte));
		bitcount = 8;//���ÿ������ĵı���λ
		if (byte == (char)0xff) {
			char zero = 0;
			infile.read((char*)&zero, sizeof(zero));
			//FF������ŵ�00��Ҫֱ���̵�
		}
	}
	int bit = byte & 128;
	//cout << "�㵱ǰ��readbit������õ���bit=" << (bool)bit;
	//getchar();
	byte <<= 1;//ÿ�ζ��ǰ�λ��Ƚ�byte�����λ��Ϊ��ǰ���ļ��ж�ȡ���ı���λ
	bitcount--;//����1�������ؼ�����-1
	if (bit)
		return 1;
	else
		return 0;
}

void Decode::reverseDCT(int FLAG,int zdata[], double YCbCr[]) {
	double before_zigzag[64] = { 0 };
	//���ȷ�zigzag
	for (int i = 0; i < 64; i++)
		before_zigzag[i] = zdata[ZigzagMat[i]];
	
	//������
	unsigned char* QTable = NULL;
	if (FLAG == Y_flag)
		QTable = Luminance_Quantization_Table;
	else
		QTable = Chrominance_Quantization_Table;
	for (int i = 0; i < 64; i++)
		before_zigzag[i] *= QTable[i];

	//��DCT
	const double pi = 3.1415926;
	for (int x = 0; x < 8; x++) {
		for (int y = 0; y < 8; y++) {
			//ÿһ���ݼ�Ϊtmp
			double tmp = 0;
			//�˴���˫����ۼ�
			for (int u = 0; u < 8; u++) {
				for (int v = 0; v < 8; v++) {
					//����Cϵ��
					double c_u = (u == 0) ? 1.f / sqrt(8.f) : 0.5f;;
					double c_v = (v == 0) ? 1.f / sqrt(8.f) : 0.5f;;
					//��ǰbefore_zigzag�����ж�Ӧ��ֵ
					double cur_data = before_zigzag[u * 8 + v];
					//��DCT���任�Ļ��任ϵ����ʽ
					cur_data *= cos((x + 0.5) * pi * u / 8) * cos((y + 0.5) * pi * v / 8) * c_u * c_v;
					tmp += cur_data;//���ۼ�
				}
			}
			YCbCr[x * 8 + y] = tmp;
		}
	}
}

//��YCbCr����ת��ΪRGB
void Decode::cgto_RGB(double Y[], double Cb[], double Cr[],int x,int y) {
	//(x,y)�Ǿ������Ͻǵ�һ���������λ��
	int first = x * img_width + y;//first�ǵ�һ���������������е�λ��
	int index = 0;;
	int i = 0;
	//��1��8*8�����Ӧ��ycbcrֵ��ԭ��rgb
	for (int x = 0; x < 8; x++) {//row
		for (int y = 0; y < 8; y++) {//col
			//��data�е�����
			index = (first + x * img_width + y)*4;
			i = x * 8 + y;//��ycbcr�����е�����
			int R = (int)round(1.000200*(Y[i]+128) - 0.000992 * Cb[i]+1.402126*Cr[i]);
			int G = (int)round(1.000200*(Y[i]+128) - 0.345118 * Cb[i] - 0.714010 * Cr[i]);
			int B = (int)round(1.000200*(Y[i]+128) + 1.771018 * Cb[i]+0.000143*Cr[i]);
			//����RGB����ֵ�����½����
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
			////RGBA��A
			data[index+3] = 255;
		}
	}
}

//�����ļ��ж����ĸߵ�λ�������������������������
int Decode::cgto_num(short word) {
	int num_low= word >> 8;
	int num_high = (int)((char)word);
	int num = (num_high << 8) + num_low;
	return num;
}
//���������codeֵת��Ϊstring��
string Decode::code_to_str(int code,int code_len) {
	string str = "";
	int bit;
	//��λ�������
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