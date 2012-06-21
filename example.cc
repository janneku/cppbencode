/*
 * cppbencode - Bencode (de)serialization library for C++ and STL
 *
 * Copyright 2012 Janne Kulmala <janne.t.kulmala@iki.fi>
 *
 * Program code is licensed with GNU LGPL 2.1. See COPYING.LGPL file.
 *
 * Displays the contents of a .torrent file.
 */
#include "cppbencode.h"
#include <fstream>

#define FOR_EACH_CONST(type, i, cont)		\
	for (type::const_iterator i = (cont).begin(); i != (cont).end(); ++i)

int main(int argc, char **argv)
try {
	if (argc < 2) {
		printf("Usage: %s [.torrent]\n", argv[0]);
		return 0;
	}
	std::ifstream f(argv[1]);
	if (!f) {
		throw std::runtime_error(std::string("Unable to open ") +
					 argv[1]);
	}

	ben::Value torrent;
	torrent.load_all(f);

	ben::Value info = torrent.get("info");
	std::string name = info.get("name").as_string();
	ben::Value files = info.get("files");
	if (files.type() != ben::BEN_NULL) {
		/* multiple files */
		FOR_EACH_CONST(std::vector<ben::Value>, i, files.as_array()) {
			std::string ent_name = name;
			ben::Value path = i->get("path");
			FOR_EACH_CONST(std::vector<ben::Value>, j, path.as_array()) {
				ent_name += "/" + j->as_string();
			}
			int length = i->get("length").as_integer();
			printf("%s (%d kB)\n", ent_name.c_str(), length / 1024);
		}
	} else {
		/* single file */
		int length = info.get("length").as_integer();
		printf("%s (%d kB)\n", name.c_str(), length / 1024);
	}

	return 0;
} catch (const std::runtime_error &e) {
	fprintf(stderr, "Load error: %s\n", e.what());
	return 1;
}
