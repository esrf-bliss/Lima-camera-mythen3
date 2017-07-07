#include <iostream>
#include <stdint.h>
#include <limits.h>
#include <string>
#include <map>

void decode(int nbits, uint32_t* buff, int width);

int main () {
	int width = 64;
	int nbits = 8;
	int buff[width];
	for (int i=0; i<width; i++)
		buff[i] = 0;
	int* bptr = buff;
	int8_t* ptr = reinterpret_cast<int8_t*>(bptr);
	for (int i=0; i<width; i++) {
		*ptr++ = i;
	}
	for (int i=0; i<width; i++) {
		std::cout << "buff[" << i << "] = " << buff[i] << std::endl;
	}

	decode(nbits, (uint32_t*)buff, width);

	for (int i=0; i<width; i++) {
		std::cout << "buff[" << i << "] = " << buff[i] << std::endl;
	}
}
void decode(int nbits, uint32_t* buff, int width) {
	int chansPerPoint = CHAR_BIT * sizeof(int) / nbits;
	uint32_t mask = 0xffffffff >> ((nbits*(chansPerPoint-1)));
	int size = width/chansPerPoint;
	uint32_t *bptr = buff+width-1;
	uint32_t *iptr = buff+size-1;
	for (int j=size-1; j>=0; j--, iptr--) {
		for (int i=chansPerPoint-1; i>=0; i--) {
			uint32_t shift = nbits*i;
			uint32_t shiftmask = mask << shift;
			*bptr-- = ((*iptr & shiftmask) >> shift) & mask;
		}
	}
}
