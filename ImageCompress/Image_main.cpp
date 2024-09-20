/*����� 2252429 ������*/
#include "PicReader.h"
#include "Compress.h"
#include "Decode.h"

/*
tools.cpp�Ĵ������۵�����ע����debug��ʱ��д�������ʾ��
��Ϊ��δ���ҵdebug�����̫ʹ����
��������Ƕ�ɾ���Ļ����޷���������������һ�ܵ�ʹ�����
��˰���Щע�Ͷ�������
Lena����ʹ�Ŀ̹�����
*/


int main(int argc,char**argv) {
	cout << "Zipper 0.002! Author:2252429" << endl;
	cout << "cmdline:[-read][-compress]" << endl;//����ĸ��Ի���ʾ
	if (argc != 3) {//������������
		cerr << "Please make sure the number of parameters is correct!" << endl;
		return -1;
	}
	if (strcmp(argv[1], "-read") && strcmp(argv[1], "-compress")) {//���������������
		cerr<<"Invalid parameter!"<<endl;
		return -1;
	}
	if (!strcmp(argv[1], "-compress") ){
		/*********ԭͼ��ȡ**********/
		PicReader imread;
		BYTE* data = nullptr;
		UINT img_width, img_height;
		imread.readPic(argv[2]);
		imread.getData(data, img_width, img_height);//ÿ�����ص��RGBA�洢��data
		/*********ԭͼ��ȡ**********/
		cout << "Compressing..." << endl;
		JPEG my_jpeg(data, img_width, img_height,argv[2]);//���첢��ʼ��һ��JPEG��
		my_jpeg.MyCompress();//���ø����compress����
		cout << "[notice]:compressed filename \"lena.jpg\"" << endl;
		cout << "Complete!" << endl;
	}
	else if (!strcmp(argv[1], "-read")) {
		cout << "Reading..." << endl;
		cout << "[notice]:You are using [PicReader] to show picture [lena.jpg]!" << endl;
		Decode my_decode;
		my_decode.MyDecode();
		cout << "Complete!" << endl;
	}
	return 0;
}


