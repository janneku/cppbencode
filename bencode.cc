/*
 * cppbencode - Bencode (de)serialization library for C++ and STL
 *
 * Copyright 2012 Janne Kulmala <janne.t.kulmala@iki.fi>
 *
 * Program code is licensed with GNU LGPL 2.1. See COPYING.LGPL file.
 */
#include "cppbencode.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#define FOR_EACH_CONST(type, i, cont)		\
	for (type::const_iterator i = (cont).begin(); i != (cont).end(); ++i)

namespace ben {

/* Format a string, similar to sprintf() */
const std::string strf(const char *fmt, ...)
{
	va_list vl;
	va_start(vl, fmt);
	char *buf = NULL;
	if (vasprintf(&buf, fmt, vl) < 0) {
		va_end(vl);
		throw std::bad_alloc();
	}
	va_end(vl);
	std::string s(buf);
	free(buf);
	return s;
}

static const char *type_names[] = {
	"null",
	"string",
	"integer",
	"boolean",
	"dictionary",
	"array",
};

Value::Value(Type type) :
	m_type(type)
{
	switch (m_type) {
	case BEN_DICT:
		m_value.dict = new dict_map_t;
		break;
	case BEN_ARRAY:
		m_value.array = new std::vector<Value>;
		break;
	case BEN_NULL:
		break;
	default:
		/* other types can not be constructed */
		assert(0);
	}
}

Value::Value(const std::string &s) :
	m_type(BEN_STRING)
{
	m_value.string = new std::string(s);
}

Value::Value(const char *s) :
	m_type(BEN_STRING)
{
	m_value.string = new std::string(s);
}

Value::Value(int i) :
	m_type(BEN_INTEGER)
{
	m_value.integer = i;
}

Value::Value(bool b) :
	m_type(BEN_BOOLEAN)
{
	m_value.boolean = b;
}

Value::Value(const dict_map_t &dict) :
	m_type(BEN_DICT)
{
	m_value.dict = new dict_map_t(dict);
}

Value::Value(const std::vector<Value> &array) :
	m_type(BEN_ARRAY)
{
	m_value.array = new std::vector<Value>(array);
}

Value::Value(const Value &from) :
	m_type(BEN_NULL)
{
	*this = from;
}

Value::~Value()
{
	destroy();
}

void Value::destroy()
{
	switch (m_type) {
	case BEN_STRING:
		delete m_value.string;
		break;
	case BEN_DICT:
		delete m_value.dict;
		break;
	case BEN_ARRAY:
		delete m_value.array;
		break;
	case BEN_NULL:
	case BEN_BOOLEAN:
	case BEN_INTEGER:
		break;
	}
	m_type = BEN_NULL;
}

void Value::verify_type(Type expected) const
{
	if (expected != m_type) {
		throw type_error(strf("Expected type %s, but got %s",
				 type_names[expected], type_names[m_type]));
	}
}

void Value::operator = (const Value &from)
{
	if (this == &from)
		return;
	destroy();
	m_type = from.m_type;
	switch (m_type) {
	case BEN_NULL:
		break;
	case BEN_STRING:
		m_value.string = new std::string(*from.m_value.string);
		break;
	case BEN_DICT:
		m_value.dict = new dict_map_t(*from.m_value.dict);
		break;
	case BEN_ARRAY:
		m_value.array = new std::vector<Value>(*from.m_value.array);
		break;
	case BEN_INTEGER:
		m_value.integer = from.m_value.integer;
		break;
	case BEN_BOOLEAN:
		m_value.boolean = from.m_value.boolean;
		break;
	default:
		assert(0);
	}
}

bool Value::operator == (const Value &other) const
{
	if (m_type != other.m_type)
		return false;
	switch (m_type) {
	case BEN_NULL:
		return true;
	case BEN_STRING:
		return *m_value.string == *other.m_value.string;
	case BEN_DICT:
		return *m_value.dict == *other.m_value.dict;
	case BEN_ARRAY:
		return *m_value.array == *other.m_value.array;
	case BEN_INTEGER:
		return m_value.integer == other.m_value.integer;
	case BEN_BOOLEAN:
		return m_value.boolean == other.m_value.boolean;
	default:
		assert(0);
	}
}

bool Value::operator != (const Value &other) const
{
	return !(*this == other);
}

std::string load_string(std::istream &is)
{
	int c = is.peek();
	if (is.eof()) {
		throw decode_error("Unexpected end of input");
	}
	if (c < '0' || c > '9') {
		throw decode_error("Expected a digit");
	}
	size_t len = 0;
	is >> len;
	if (!is) {
		throw decode_error("Invalid string length");
	}
	if (len != 0 && c == '0') {
		throw decode_error("String length has leading zeroes");
	}
	if (is.get() != ':') {
		throw decode_error("Expected ':'");
	}
	std::string str(len, 0);
	if (len) {
		is.read(&str[0], len);
		if (is.eof()) {
			throw decode_error("Unexpected end of input");
		}
	}
	return str;
}

void Value::load(std::istream &is)
{
	destroy();
	int c = is.peek();
	if (is.eof()) {
		throw decode_error("Unexpected end of input");
	}
	switch (c) {
	case 'd':
		is.get();
		m_type = BEN_DICT;
		m_value.dict = new dict_map_t;
		while (is.peek() != 'e') {
			std::string key = load_string(is);
			/*
			 * To avoid a copy, first insert an empty value to
			 * the container and then load it from the input.
			 */
			std::pair<dict_map_t::iterator, bool> res =
				m_value.dict->insert(std::make_pair(key, Value()));
			if (!res.second) {
				throw decode_error("Duplicate key in dictionary");
			}
			res.first->second.load(is);
		}
		is.get();
		break;
	case 'l':
		is.get();
		m_type = BEN_ARRAY;
		m_value.array = new std::vector<Value>;
		while (is.peek() != 'e') {
			m_value.array->push_back(Value());
			m_value.array->back().load(is);
		}
		is.get();
		break;
	case 'i':
		is.get();
		c = is.peek();
		if (is.eof()) {
			throw decode_error("Unexpected end of input");
		}
		if ((c < '0' || c > '9') && c != '-') {
			throw decode_error("Expected a digit or '-'");
		}
		m_type = BEN_INTEGER;
		is >> m_value.integer;
		if (!is) {
			throw decode_error("Invalid integer");
		}
		if (m_value.integer != 0 && c == '0') {
			throw decode_error("Integer has leading zeroes");
		}
		if (m_value.integer == 0 && c == '-') {
			throw decode_error("Zero with a minus sign");
		}
		if (is.get() != 'e') {
			throw decode_error("Expected 'e'");
		}
		break;
	case 'b':
		is.get();
		m_type = BEN_BOOLEAN;
		c = is.get();
		if (is.eof()) {
			throw decode_error("Unexpected end of input");
		}
		switch (c) {
		case '1':
			m_value.boolean = true;
			break;
		case '0':
			m_value.boolean = false;
			break;
		default:
			throw decode_error("Expected '0' or '1'");
			break;
		}
		break;
	default:
		if (c >= '0' && c <= '9') {
			m_value.string = new std::string(load_string(is));
			m_type = BEN_STRING;
		} else {
			throw decode_error("Unknown character in input");
		}
	}
}

void Value::load_all(std::istream &is)
{
	load(is);
	is.peek();
	if (!is.eof()) {
		throw decode_error("Left over data in input");
	}
}

void Value::write(std::ostream &os) const
{
	switch (m_type) {
	case BEN_STRING:
		os << m_value.string->size();
		os.put(':');
		os << *m_value.string;
		break;
	case BEN_DICT:
		os.put('d');
		FOR_EACH_CONST(dict_map_t, i, *m_value.dict) {
			os << i->first.size();
			os.put(':');
			os << i->first;
			i->second.write(os);
		}
		os.put('e');
		break;
	case BEN_ARRAY:
		os.put('l');
		FOR_EACH_CONST(std::vector<Value>, i, *m_value.array) {
			i->write(os);
		}
		os.put('e');
		break;
	case BEN_INTEGER:
		os.put('i');
		os << m_value.integer;
		os.put('e');
		break;
	case BEN_BOOLEAN:
		os.put('b');
		os.put(m_value.boolean ? '1' : '0');
		break;
	default:
		assert(0);
	}
}

}
