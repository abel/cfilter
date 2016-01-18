#include <stdint.h>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>

#include "trie_filter.h"

const int primes[] =
{
	3, 7, 11, 0x11, 0x17, 0x1d, 0x25, 0x2f, 0x3b, 0x47, 0x59, 0x6b, 0x83, 0xa3, 0xc5, 0xef,
	0x125, 0x161, 0x1af, 0x209, 0x277, 0x2f9, 0x397, 0x44f, 0x52f, 0x63d, 0x78b, 0x91d, 0xaf1, 0xd2b, 0xfd1, 0x12fd,
	0x16cf, 0x1b65, 0x20e3, 0x2777, 0x2f6f, 0x38ff, 0x446f, 0x521f, 0x628d, 0x7655, 0x8e01, 0xaa6b, 0xcc89, 0xf583, 0x126a7, 0x1619b,
	0x1a857, 0x1fd3b, 0x26315, 0x2dd67, 0x3701b, 0x42023, 0x4f361, 0x5f0ed, 0x72125, 0x88e31, 0xa443b, 0xc51eb, 0xec8c1, 0x11bdbf, 0x154a3f, 0x198c4f,
	0x1ea867, 0x24ca19, 0x2c25c1, 0x34fa1b, 0x3f928f, 0x4c4987, 0x5b8b6f, 0x6dda89
};


struct HashHelpers
{
public:
	static int GetPrime(int min)
	{
		for (int p : primes)
		{
			if (p >= min)
			{
				return p;
			}
		}
		for (int i = min | 1; i < 0x7fffffff; i += 2)
		{
			if (IsPrime(i))
			{
				return i;
			}
		}
		return min;
	}

	static bool IsPrime(int candidate)
	{
		if ((candidate & 1) == 0)
		{
			return (candidate == 2);
		}
		int num = (int)sqrt((double)candidate);
		for (int i = 3; i <= num; i += 2)
		{
			if ((candidate % i) == 0)
			{
				return false;
			}
		}
		return true;
	}
};

class TrieNode;

struct TrieSlot
{
	bool end;
	unsigned char key;
	int next;
	TrieNode* value;
};

class TrieNode
{
private:
	int _capacity;
	int _size;        //已使用
	int* _buckets;    //用于根据hash定位m_slots中的索引
	TrieSlot* _slots;

public:

	TrieNode()
	{
		memset(this, 0, sizeof(*this));
	}

	~TrieNode()
	{
		if (_slots != nullptr)
		{
			for (int i = 0; i < _size; ++i)
			{
				delete _slots[i].value;
			}
			delete[] _buckets;
			delete[] _slots;
		}
	}

private:
	//扩容
	void IncreaseCapacity(int capacity)
	{
		int size = _size;
		int prime = HashHelpers::GetPrime(capacity);
		TrieSlot* newSlots = new TrieSlot[prime];
		if (_slots != nullptr)
		{
			memcpy(newSlots, _slots, size * sizeof(TrieSlot));
		}
		memset(newSlots + size, 0, (prime - size)*sizeof(TrieSlot));

		int* newBuckets = new int[prime];
		memset(newBuckets, 0, prime * sizeof(int));

		//重新设置m_buckets和next
		for (int i = 0; i < size;)
		{
			int index = newSlots[i].key % prime;
			newSlots[i].next = newBuckets[index];
			newBuckets[index] = ++i;
		}

		delete[] _slots;
		delete[] _buckets;
		_slots = newSlots;
		_buckets = newBuckets;
		_capacity = prime;
	}

	int GetNodeIndex(unsigned char key)
	{
		if (this->_capacity > 0)
		{
			int index = key % this->_capacity;
			int i = this->_buckets[index];
			while (i > 0 && i <= this->_size)
			{
				i -= 1;
				if (this->_slots[i].key == key)
				{
					return i + 1;
				}
				i = this->_slots[i].next;
			}
		}
		return 0;
	}

	void AddChar(unsigned char key)
	{
		int cp = this->_size + (this->_size >> 1);
		if (this->_capacity <= cp) {
			this->IncreaseCapacity(cp);
		}
		int index = key % this->_capacity;
		int next = this->_buckets[index];
		this->_slots[this->_size].key = key;
		this->_slots[this->_size].next = next;
		this->_buckets[index] = ++this->_size;
	}

public:
	void AddKeyword(const unsigned char* keys, int len, const unsigned char* trans)
	{
		if (len == 0)
		{
			return;
		}
		auto key = keys[0];
		if (key == 0)
		{
			return;
		}
		auto tran = trans[key];
		if (tran != 0)
		{
			key = tran;
		}
		int i = this->GetNodeIndex(key);
		if (i == 0) {
			this->AddChar(key);
			i = this->_size;
		}
		auto curSlot = this->_slots + (i - 1);
		if (len == 1)
		{
			curSlot->end = true;
		}
		else
		{
			if (curSlot->value == nullptr)
			{
				curSlot->value = new TrieNode();
			}
			curSlot->value->AddKeyword(keys + 1, len - 1, trans);
		}
	}

	bool ExistKeyword(const unsigned char* keys, int len, const unsigned char* trans, int* depth)
	{
		if (len == 0)
		{
			return false;
		}
		*depth += 1;
		auto key = keys[0];
		auto tran = trans[key];
		auto ignore = true;
		if (tran != 0)
		{
			ignore = false;
			key = tran;
		}
		auto i = this->GetNodeIndex(key);
		if (i == 0)
		{
			if (ignore)
			{
				return this->ExistKeyword(keys + 1, len - 1, trans, depth);
			}
			else
			{
				return false;
			}
		}
		else
		{
			auto curSlot = this->_slots + (i - 1);
			if (curSlot->end)
			{
				return true;
			}
			else
			{
				if (curSlot->value == nullptr)
				{
					return false;
				}
				return curSlot->value->ExistKeyword(keys + 1, len - 1, trans, depth);
			}
		}
	}

};

class TrieFilter
{
	static const int CharCount = 256;
	//用于字符转换(大小写转换,被忽略的字符对应位置设为0,)
	unsigned char transition[CharCount];
	TrieNode _rootNode;

public:
	TrieFilter(bool ignoreCase = true)
	{
		for (int i = 0; i < CharCount; ++i)
		{
			transition[i] = i;
		}
		//将小写转为大写字母
		if (ignoreCase)
		{
			for (unsigned char a = 'a'; a <= 'z'; ++a)
			{
				transition[a] = a - 32; //(a:97,A:65);
			}
		}
		//if (ignoreSimpTrad)
		//{
		//	  AddReplaceChars(zh_TW, zh_CN);
		//}
	}

	inline void AddIgnoreChars(const char* passChars)
	{
		AddIgnoreChars((const unsigned char*)passChars);
	}

	inline void AddReplaceChars(const char* srcChar, const char* replaceChar)
	{
		AddReplaceChars((const unsigned char*)srcChar, (const unsigned char*)srcChar);
	}

	inline void AddKeyword(const char* key, int len)
	{
		AddKeyword((const unsigned char*)key, len);
	}
	inline void AddKeyword(const std::string& key)
	{
		AddKeyword(key.c_str(), static_cast<int>(key.size()));
	}

	inline bool ExistKeyword(const char* text, int len)
	{
		return ExistKeyword((const unsigned char*)text, len);
	}

	inline bool ExistKeyword(const std::string& text)
	{
		return ExistKeyword(text.c_str(), static_cast<int>(text.size()));
	}

	inline std::string FindOne(const char* text, int len)
	{
		return FindOne((const unsigned char*)text, len);
	}

	inline std::string FindOne(const std::string& text)
	{
		return FindOne(text.c_str(), static_cast<int>(text.size()));
	}

	inline int FindAll(const char* text, int len, std::vector<std::string>* outlist)
	{
		return FindAll((const unsigned char*)text, len, outlist);
	}

	inline int FindAll(const std::string& text, std::vector<std::string>* outlist)
	{
		return FindAll(text.c_str(), static_cast<int>(text.size()), outlist);
	}

	int Replace(const char* text, int len, char* outbuffer, char mask = '*')
	{
		return Replace((const unsigned char*)text, len, (unsigned char*)outbuffer, (unsigned char)mask);
	}

	inline int Replace(const std::string& text, char* outbuffer, char mask = '*')
	{
		return Replace(text.c_str(), static_cast<int>(text.size()), outbuffer, mask);
	}

	//增加忽略字符(参数以空结尾)
	void AddIgnoreChars(const unsigned char* passChars)
	{
		for (int i = 0; i < CharCount; ++i)
		{
			unsigned char src = passChars[i];
			if (src == 0)
			{
				return;
			}
			transition[src] = 0;
		}
	}

	//增加替换字符.以空结尾
	void AddReplaceChars(const unsigned char* srcChar, const unsigned char* replaceChar)
	{
		for (int i = 0; i < CharCount; ++i)
		{
			unsigned char src = srcChar[i];
			unsigned char rep = replaceChar[i];
			if (src == 0 || rep == 0)
			{
				return;
			}
			transition[src] = rep;
		}
	}

	void AddKeyword(const unsigned char* keys, int len)
	{
		this->_rootNode.AddKeyword(keys, len, this->transition);
	}

	bool ExistKeyword(const unsigned char* text, int len)
	{
		for (int i = 0; i < len; ++i)
		{
			int index = this->FindKeywordIndex(text + i, len - i);
			if (index > 0)
			{
				return true;
			}
		}
		return false;
	}

	std::string FindOne(const unsigned char* text, int len)
	{
		std::string result;
		const unsigned char* head = text;
		while (len > 0)
		{
			int index = this->FindKeywordIndex(head, len);
			if (index > 0)
			{
				result.append((const char*)head, index);
				break;
			}
			--len;
			++head;
		}
		return result;
	}


	int FindAll(const unsigned char* text, int len, std::vector<std::string>* outlist)
	{
		int findCount = 0;
		const unsigned char* head = text;
		while (len > 0)
		{
			int index = this->FindKeywordIndex(head, len);
			if (index > 0)
			{
				++findCount;
				if (outlist)
				{
					outlist->push_back(std::string((const char*)head, index));
				}
				head += index;
				len -= index;
			}
			else
			{
				--len;
				++head;
			}
		}
		return findCount;
	}

private:

	int FindKeywordIndex(const unsigned char* text, int len)
	{
		int depth = 0;
		if (this->_rootNode.ExistKeyword(text, len, this->transition, &depth))
		{
			return depth;
		}
		return 0;
	}

	int Replace(const unsigned char* text, int len, unsigned char* outbuffer, unsigned char mask)
	{
		int findCount = 0;
		int outbufferIndex = 0;
		const unsigned char* head = text;
		while (len > 0)
		{
			int index = this->FindKeywordIndex(head, len);
			if (index > 0)
			{
				++findCount;
				outbuffer[outbufferIndex++] = mask;
				head += index;
				len -= index;
			}
			else
			{
				outbuffer[outbufferIndex++] = *head;
				--len;
				++head;
			}
		}
		if (outbufferIndex < len)
		{
			outbuffer[outbufferIndex] = 0;
		}
		return findCount;
	}
};

static TrieFilter trie;
static TrieFilter trie_name;


bool TrieHasBadWord(const char* text, int len)
{
	return trie.ExistKeyword(text, len);
}

int TrieHasBadName(const char* text, int len)
{
	if (trie.ExistKeyword(text, len) || trie_name.ExistKeyword(text, len))
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

int TrieReplaceBadWord(const char* text, int len, char* outbuffer)
{
	return trie.Replace(text, len, outbuffer);
}

inline void str_trim_crlf(char *str)
{
	char *p = &str[strlen(str) - 1];
	while (*p == '\r' || *p == '\n') // use \0 replace \r\n
		*p-- = '\0';
}

void LoadMaskFile(TrieFilter& filter, const char* path)
{
	std::ifstream file;
	file.open(path);
	if (!file.is_open())
	{
		return;
	}
	char line[1024]{};
	while (file.getline(line, sizeof(line)))
	{
		// 一行一行读
		if (strlen(line) != 0)
		{
			str_trim_crlf(line);
			filter.AddKeyword(line, static_cast<int>(strlen(line)));
			memset(line, 0, sizeof(line));
		}
	}
	file.clear();
	file.close();
}

void LoadMaskWordFile(const char* path)
{
	trie.AddIgnoreChars(" *&^%$#@!~,.:[]{}?+-~\"\\");
	LoadMaskFile(trie, path);
}

void LoadMaskNameFile(const char* path)
{
	LoadMaskFile(trie_name, path);
}

std::string MsgFilter(const std::string& text)
{
	std::string r;
	int len = std::min(static_cast<int>(text.length()), 65535);
	char outbuffer[65535 + 1]{};
	int findCount = TrieReplaceBadWord(text.c_str(), len, outbuffer);
	if (findCount == 0)
	{
		r = text;
	}
	else
	{
		r.append(outbuffer, len);
	}
	return r;
}
