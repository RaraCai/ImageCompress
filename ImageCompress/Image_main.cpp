/*计算机 2252429 蔡宇轩*/
#include "PicReader.h"
#include "Compress.h"
#include "Decode.h"

/*
tools.cpp的代码中折叠掉的注释是debug的时候写的输出提示，
因为这次大作业debug真的是太痛苦了
如果把它们都删掉的话，无法让我铭记这整整一周的痛苦回忆
因此把这些注释都留下了
Lena让我痛的刻骨铭心
*/


int main(int argc,char**argv) {
	cout << "Zipper 0.002! Author:2252429" << endl;
	cout << "cmdline:[-read][-compress]" << endl;//输出的个性化提示
	if (argc != 3) {//参数个数错误
		cerr << "Please make sure the number of parameters is correct!" << endl;
		return -1;
	}
	if (strcmp(argv[1], "-read") && strcmp(argv[1], "-compress")) {//参数内容输入错误
		cerr<<"Invalid parameter!"<<endl;
		return -1;
	}
	if (!strcmp(argv[1], "-compress") ){
		/*********原图读取**********/
		PicReader imread;
		BYTE* data = nullptr;
		UINT img_width, img_height;
		imread.readPic(argv[2]);
		imread.getData(data, img_width, img_height);//每个像素点的RGBA存储在data
		/*********原图读取**********/
		cout << "Compressing..." << endl;
		JPEG my_jpeg(data, img_width, img_height,argv[2]);//构造并初始化一个JPEG类
		my_jpeg.MyCompress();//调用该类的compress函数
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


