
#ifndef UINT8
typedef unsigned char uint8;
#endif

int saveImg(uint8* img, int height, int width, int channel, std::string filename);
int readImg(uint8*& img, int& height, int& width, int& channel, char* filename);