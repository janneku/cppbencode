/*
 * cppbencode - Bencode (de)serialization library for C++ and STL
 *
 * Copyright 2012 Janne Kulmala <janne.t.kulmala@iki.fi>
 *
 * Program code is licensed with GNU LGPL 2.1. See COPYING.LGPL file.
 */
#ifndef __cppbencode_h
#define __cppbencode_h

#include <string>
#include <map>
#include <vector>
#include <istream>
#include <ostream>
#include <stdexcept>
#include <stdint.h>
#include <assert.h>

namespace ben {

class decode_error: public std::runtime_error {
public:
	decode_error(const std::string &what) :
		std::runtime_error(what)
	{}
};

class type_error: public std::runtime_error {
public:
	type_error(const std::string &what) :
		std::runtime_error(what)
	{}
};

/* Must use the same order as type_names[] */
enum Type {
	BEN_NULL, /* can not be serialized */
	BEN_STRING,
	BEN_INTEGER,
	BEN_BOOLEAN,
	BEN_DICT,
	BEN_ARRAY,
};

class Value;
typedef std::map<std::string, Value> dict_map_t;

class Value {
public:
	Value(Type type = BEN_NULL);
	Value(const std::string &s);
	Value(const char *s);
	Value(int i);
	Value(bool b);
	Value(const dict_map_t &dict);
	Value(const std::vector<Value> &array);
	Value(const Value &from);
	~Value();

	Type type() const { return m_type; }

	std::string as_string() const
	{
		verify_type(BEN_STRING);
		return *m_value.string;
	}
	int as_integer() const
	{
		verify_type(BEN_INTEGER);
		if (int(m_value.integer) != m_value.integer) {
			throw type_error("Too large a integer");
		}
		return m_value.integer;
	}
	int64_t as_int64() const
	{
		verify_type(BEN_INTEGER);
		return m_value.integer;
	}
	bool as_boolean() const
	{
		verify_type(BEN_BOOLEAN);
		return m_value.boolean;
	}
	const dict_map_t &as_dict() const
	{
		verify_type(BEN_DICT);
		return *m_value.dict;
	}
	const std::vector<Value> &as_array() const
	{
		verify_type(BEN_ARRAY);
		return *m_value.array;
	}
	const dict_map_t &as_const_dict()
	{
		verify_type(BEN_DICT);
		return *m_value.dict;
	}
	const std::vector<Value> &as_const_array()
	{
		verify_type(BEN_ARRAY);
		return *m_value.array;
	}
	dict_map_t &as_dict()
	{
		verify_type(BEN_DICT);
		return *m_value.dict;
	}
	std::vector<Value> &as_array()
	{
		verify_type(BEN_ARRAY);
		return *m_value.array;
	}

	Value get(const std::string &s) const
	{
		verify_type(BEN_DICT);
		dict_map_t::const_iterator i = m_value.dict->find(s);
		if (i == m_value.dict->end()) {
			/* return null */
			return Value();
		}
		return i->second;
	}
	void set(const std::string &s, const Value &val)
	{
		verify_type(BEN_DICT);
		m_value.dict->insert(std::make_pair(s, val));
	}

	void append(const Value &val)
	{
		verify_type(BEN_ARRAY);
		m_value.array->push_back(val);
	}

	void operator = (const Value &from);

	bool operator == (const Value &other) const;
	bool operator != (const Value &other) const;

	void load(std::istream &is);
	void load_all(std::istream &is);

	void write(std::ostream &os) const;

private:
	Type m_type;
	union {
		std::string *string;
		int64_t integer;
		bool boolean;
		dict_map_t *dict;
		std::vector<Value> *array;
	} m_value;

	void destroy();

	void verify_type(Type type) const;
};

}

#endif
