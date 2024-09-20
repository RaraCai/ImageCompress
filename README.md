# ImageCompress/图像压缩
* Tongji University-2023 OOP course-project homework-ImageCompress  
* 同济大学计算机科学与技术系-2023年度oop课程大作业-图像压缩
## Disclaimers/免责声明
> The code and materials contained in this repository are intended for personal learning and research purposes only and may not be used for any commercial purposes. Other users who download or refer to the content of this repository must strictly adhere to the principles of academic integrity and must not use these materials for any form of homework submission or other actions that may violate academic honesty. I am not responsible for any direct or indirect consequences arising from the improper use of the contents of this repository. Please ensure that your actions comply with the regulations of your school or institution, as well as applicable laws and regulations, before using this content. If you have any questions, please contact me via [email](mailto:cyx_yuxuan@outlook.com).  
> 本仓库包含的代码和资料仅用于个人学习和研究目的，不得用于任何商业用途。请其他用户在下载或参考本仓库内容时，严格遵守学术诚信原则，不得将这些资料用于任何形式的作业提交或其他可能违反学术诚信的行为。本人对因不恰当使用仓库内容导致的任何直接或间接后果不承担责任。请在使用前务必确保您的行为符合所在学校或机构的规定，以及适用的法律法规。如有任何问题，请通过[电子邮件](mailto:cyx_yuxuan@outlook.com)与我联系。
## Running Tips/运行提示
### About image size/关于图片尺寸
* Due to the limitation of Lena's image size, this program can currently only process images that are `multiples of 8 in length and width`  
* 由于受限于lena的图片尺寸，本程序目前只可以处理`长宽均为8的倍数`的图片  
### About compression/关于压缩信息
* The default compression ratio of this program is `1`, version `(1,1)`, without thumbnail  
* 本程序默认`压缩比为1`，版本`(1,1)`，无缩略图
### About file name/关于文件名
* Due to @ RaraCai's laziness, the file name was not decomposed, and the compressed file was directly named `lena.jpg`.If your image is called another name, the compressed image file name produced by this program will still be `lena.jpg`.  
* 由于@RaraCai的懒惰，未对文件名做分解，即直接将压缩文件命名为lena.jpg，如果你的图片叫别的名字，本程序压出来的压缩图片文件名还是会叫lena.jpg  
## File Structure/文件架构
### PicReader.h/PicReader.cpp
* **Due to the fixed format of the compressed image format. jpg file heade**r, PicReader is used to `read the original image file header` and `write the compressed image file header`.  
* **因压缩图片格式.jpg有其固定格式的文件头**，PicReader用于实现`读取原图文件头`和`写入压缩图片文件头`
### Compress.h
* header file of class `JPEG`, used to compress the original picture
* 压缩图像类`JPEG`头文件  
### Decode.h
* header file of class `Decode`, used to decode an compressed picture
* 解压图像类`Decode`头文件  
### Image_tools.cpp
* the realization of the two classes and related member functions above
* `JPEG`和`Deocde`类成员函数的实现  
### Image_main.cpp
* 主函数(the main function)
### lena.tiff
* 待压缩的原图文件  
* the original file to be compressed
### report
* 实验报告，仅供参考  
* project report for reference only
