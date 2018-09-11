#include <stdlib.h>
#include "bmp.h"
#include <string>
#include <fstream>
#include <stdio.h>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <vector>
#include <bitset>

using namespace std;



int main(int argc, char* argv[])
{
	int width = 200;
	int height = 32;
	ofstream log("overlay_test.txt");
	ofstream log2("overlay_test2.txt");

	ofstream enc("withencoding.bin");
	ofstream raw("withoutencoding.bin"); 
	char msg[13] = "fdafdadd\0";
	Bitmap *b = bm_make_text(width, height, bm_atoi("black"), bm_atoi("white"),
                           255, msg);
	
	ostringstream ss;
	raw.write((char *)b->data, width*height*4);
	

	int len = width*height*4;
	vector<bool> bits;
	for (int i = 0; i < len; i+=4) {
		if ((b->data[i]) == 0) {
			bits.push_back(false);
		} else {
			bits.push_back(true);
		}
	}
	while (bits.size()%8 != 0) {bits.push_back(false);}

	int bufferLen = bits.size()/8;
	char buffer[bufferLen];
	for (int i = 0; i < bufferLen; i++) {
		std::bitset<8> bits8;
		for (int j = 0; j < 8; j++) {
			bits8[j] = bits[i*8 + j];
		}
		unsigned char oneByte = (char)bits8.to_ulong();
		buffer[i] = oneByte;
	}

	for (int i = 0; i < bufferLen; i++) {
		if (buffer[i] )
	}


	enc.write((char *)buffer, bits.size()/8);






	for (int i = 0; i < bufferLen; i++) {
		bitset<8> bit(buffer[i]);
		for (int x = 0; x < 8; x++) {
			if (bit[x]) {
				log << (char)(255) << (char)(255) << (char)(255) << (char)(255);
			} else {
				log << (char)(0) << (char)(0) << (char)(0) << (char)(255);
			}
			log << endl;

			if (i < bufferLen-1) {
				int index = i*32 + x*4;
				log2 << b->data[index] << b->data[index + 1] << b->data[index + 2] << b->data[index + 3] << endl;
			}
		}
	}

	
	log.close();
	log2.close();
	cout << "done" << endl;
}
