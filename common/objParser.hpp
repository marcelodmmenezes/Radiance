#ifndef OBJ_PARSER_HPP
#define OBJ_PARSER_HPP

#include "glContext.hpp"

#include <string>
#include <vector>

/*
 * Basic obj parser for now.
 * Object must contain v, vn and vt
 */
bool parseOBJ(
	std::string const& file_path,
	std::vector<BufferInfo<float>>& buffers,
	std::vector<unsigned>& indices);

#endif // OBJ_PARSER_HPP

