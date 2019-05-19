#include "DataReader.h"
#include <fstream>

long ReadData(const char* filename, char*& bytes)
{
	FILE *fl;
	fopen_s(&fl, filename, "rb");
	fseek(fl, 0, SEEK_END);
	long len = ftell(fl);
	bytes = (char*)malloc(len);
	fseek(fl, 0, SEEK_SET);
	fread(bytes, 1, len, fl);
	fclose(fl);
	return len;
}