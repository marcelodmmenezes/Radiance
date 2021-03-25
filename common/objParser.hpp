#ifndef OBJ_PARSER_HPP
#define OBJ_PARSER_HPP

#include "glContext.hpp"

#include <string>
#include <vector>

/*
 * Basic obj parser for now.
 * Object must contain v, vn, and vt
 */
bool parseOBJ(
	std::string const& file_path,
	std::vector<BufferInfo<float>>& buffers,
	std::vector<unsigned>& indices);

/*
 * Mesh must be composed of triangles
 * @tangents must be zeroed
 */
void generateTangentVectors(
	std::vector<unsigned> const& indices,
	std::vector<float> const& positions,
	std::vector<float> const& uvs,
	std::vector<float>& tangents);

#endif // OBJ_PARSER_HPP

