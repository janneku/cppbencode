#include "cppbencode.h"
#include <sstream>
#include <stdio.h>

void verify(const ben::Value &value, const char *s)
{
	std::ostringstream ss;
	value.write(ss);

	assert(ss.str() == s);

	std::istringstream parser(ss.str());
	ben::Value value2;
	value2.load_all(parser);

	assert(value == value2);
}

void verify_error(const char *s, const char *error)
{
	ben::Value val;
	std::istringstream ss(s);
	try {
		val.load(ss);
		/* should always raise an exception */
		assert(0);
	} catch (const ben::decode_error &e) {
		assert(e.what() == std::string(error));
	}
}

int main()
{
	verify(0, "i0e");
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

	verify_error("i1234", "Expected 'e'");
	verify_error("dx", "Expected a digit");
	verify_error("d-5", "Expected a digit");
	verify_error("d123", "Expected ':'");
	verify_error("i", "Unexpected end of input");
	verify_error("i 1e", "Expected a digit or '-'");
	verify_error("i1111111111111111111111e", "Invalid integer");
	verify_error("i- 1e", "Invalid integer");
	verify_error("i-0e", "Zero with a minus sign");
	verify_error("i05e", "Integer has leading zeroes");
	verify_error("06:foobar", "String length has leading zeroes");
	verify_error("123", "Expected ':'");
	verify_error("5:foo", "Unexpected end of input");
	verify_error("l", "Unexpected end of input");

	try {
		ben::Value val;
		std::istringstream ss("d3:bari123ee");
		val.load_all(ss);
		int i = val.get("foo").as_integer();
		(void) i;
	} catch (const ben::type_error &e) {
		assert(e.what() == std::string("Expected type integer, but got undefined"));
	}

	try {
		ben::Value val;
		std::istringstream ss("d3:bari123e3:foob1e");
		val.load_all(ss);
		int i = val.get("foo").as_integer();
		(void) i;
	} catch (const ben::type_error &e) {
		assert(e.what() == std::string("Expected type integer, but got boolean"));
	}

	printf("ok\n");
	return 0;
}
