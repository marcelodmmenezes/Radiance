#include "objParser.hpp"

#include <fstream>
#include <iostream>
#include <map>
#include <sstream>

bool parseOBJ(
	std::string const& file_path,
	std::vector<BufferInfo<float>>& buffers,
	std::vector<unsigned>& indices) {

	std::ifstream file(file_path);

	if (!file) {
		std::cerr << "ERROR: Could not read " << file_path << "\n\n";
		return false;
	}

	buffers.resize(3);

	buffers[0].n_components = 3; // v
	buffers[1].n_components = 3; // vn
	buffers[2].n_components = 2; // vt

	std::vector<float> temporary_pos;
	std::vector<float> temporary_nor;
	std::vector<float> temporary_uvs;

	std::map<std::tuple<int, int, int>, int> indices_map;
	std::string in_str;
	char in_char;
	float in_float;
	size_t vertex_it = 0u;

	while (file >> in_str) {
		if (in_str == "v") {
			for (int i = 0; i < 3; ++i) {
				file >> in_float;
				temporary_pos.emplace_back(in_float);
			}
		}
		else if (in_str == "vn") {
			for (int i = 0; i < 3; ++i) {
				file >> in_float;
				temporary_nor.emplace_back(in_float);
			}
		}
		else if (in_str == "vt") {
			for (int i = 0; i < 2; ++i) {
				file >> in_float;
				temporary_uvs.emplace_back(in_float);
			}
		}
		else if (in_str == "f") {
			int pos_id, nor_id, uvs_id;

			for (size_t i = 0; i < 3; ++i) {
				file >> pos_id >> in_char >> uvs_id >> in_char >> nor_id;

				std::tuple<int, int, int> face {
					pos_id - 1,
					nor_id - 1,
					uvs_id - 1
				};

				auto it = indices_map.find(face);

				if (it == indices_map.end()) {
					indices_map[face] = vertex_it;
					indices.emplace_back(vertex_it++);
				}
				else
					indices.emplace_back(it->second);
			}
		}

		getline(file, in_str);
	}

	file.close();

	/// The buffers are filled after in case
	/// one face is specified before a vertex
	buffers[0].values.resize(buffers[0].n_components * indices_map.size());
	buffers[1].values.resize(buffers[1].n_components * indices_map.size());
	buffers[2].values.resize(buffers[2].n_components * indices_map.size());

	for (auto& it : indices_map) {
		for (size_t i = 0; i < buffers[0].n_components; ++i)
			buffers[0].values[buffers[0].n_components * it.second + i] =
				temporary_pos[buffers[0].n_components * std::get<0>(it.first) + i];

		for (size_t i = 0; i < buffers[1].n_components; ++i)
			buffers[1].values[buffers[1].n_components * it.second + i] =
				temporary_nor[buffers[1].n_components * std::get<1>(it.first) + i];

		for (size_t i = 0; i < buffers[2].n_components; ++i)
			buffers[2].values[buffers[2].n_components * it.second + i] =
				temporary_uvs[buffers[2].n_components * std::get<2>(it.first) + i];
	}

	return true;
}

