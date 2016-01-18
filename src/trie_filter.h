#pragma once
#ifdef __cplusplus
extern "C" {
#endif	

#ifndef _TrieFilter_H_
#define _TrieFilter_H_

// #include <string>

void LoadMaskWordFile(const char* path);
void LoadMaskNameFile(const char* path);

int TrieHasBadName(const char* text, int len);

int TrieReplaceBadWord(const char* text, int len, char* outbuffer);

// std::string MsgFilter(const std::string& text);


#endif // _TrieFilter_H_

#ifdef __cplusplus
}
#endif 
