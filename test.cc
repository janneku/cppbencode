#include "bencode.h"
#include <sstream>
#include <stdio.h>

void verify(const ben::Value &value, const char *s)
{
	std::ostringstream ss;
	value.write(ss);

	assert(ss.str() == s);

	std::istringstream parser(ss.str());
	ben::Value value2;
	value2.load(parser);

	assert(value == value2);
}

int main()
{
	verify(1234, "i1234e");
	verify(-1234, "i-1234e");
	verify(std::string("foobar"), "6:foobar");
	verify(std::string(""), "0:");
	verify(true, "b1");
	verify(false, "b0");

	std::vector<ben::Value> arr;
	arr.push_back(std::string("foo"));
	arr.push_back(1234);
	arr.push_back(true);
	verify(arr, "l3:fooi1234eb1e");

	ben::dict_map_t dict;
	dict.insert(std::make_pair("bar", arr));
	dict.insert(std::make_pair("foo", std::string("test")));
	verify(dict, "d3:barl3:fooi1234eb1e3:foo4:teste");

	printf("ok\n");
	return 0;
}
