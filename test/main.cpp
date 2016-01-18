
#include "../src/trie_filter.h"
#include <string>

int main(int argc, char** argv) {

	LoadMaskWordFile("maskWord.txt");
	LoadMaskNameFile("maskSpecial.txt");
	
	std::string strUtf2 = "maskSpecialfucktest";

	int exist = TrieHasBadName(strUtf2.c_str(), strUtf2.length());

	char outbuffer[1024] = {};
	int pre = TrieReplaceBadWord(strUtf2.c_str(), strUtf2.length(), outbuffer);

	printf("exist:%d, per:%d\r\n", exist, pre);
	printf("outbuffer:%s, \n", outbuffer);
	return 0;
}