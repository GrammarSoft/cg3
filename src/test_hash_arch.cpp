#include "stdafx.h"

int main(int argc, char* argv[]) {
	char *buffer = "letters go here";
	
	printf("Hash: %u\n", hash_sdbm_char(buffer));
	
	return 0;
}
