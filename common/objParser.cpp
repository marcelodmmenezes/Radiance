#include "objParser.hpp"

#include <fstream>
#include <iostream>
#include <map>
#include <sstream>

bool parseOBJ(
	std::string const& file_path,
	std::vector<BufferInfo<float>>& buffers,
	std::vector<unsigned>& indices)
{
	std::ifstream file(file_path);

	if (!file)
	{
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

	while (file >> in_str)
	{
		if (in_str == "v")
		{
			for (int i = 0; i < 3; ++i)
			{
				file >> in_float;
				temporary_pos.emplace_back(in_float);
			}
		}
		else if (in_str == "vn")
		{
			for (int i = 0; i < 3; ++i)
			{
				file >> in_float;
				temporary_nor.emplace_back(in_float);
			}
		}
		else if (in_str == "vt")
		{
			for (int i = 0; i < 2; ++i)
			{
				file >> in_float;
				temporary_uvs.emplace_back(in_float);
			}
		}
		else if (in_str == "f")
		{
			int pos_id, nor_id, uvs_id;

			for (size_t i = 0; i < 3; ++i)
			{
				file >> pos_id >> in_char >> uvs_id >> in_char >> nor_id;

				std::tuple<int, int, int> face {
					pos_id - 1,
					nor_id - 1,
					uvs_id - 1
				};

				auto it = indices_map.find(face);

				if (it == indices_map.end())
				{
					indices_map[face] = vertex_it;
					indices.emplace_back(vertex_it++);
				}
				else
				{
					indices.emplace_back(it->second);
				}
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

	for (auto& it : indices_map)
	{
		for (size_t i = 0; i < buffers[0].n_components; ++i)
		{
			buffers[0].values[buffers[0].n_components * it.second + i] =
				temporary_pos[buffers[0].n_components * std::get<0>(it.first) + i];
		}

		for (size_t i = 0; i < buffers[1].n_components; ++i)
		{
			buffers[1].values[buffers[1].n_components * it.second + i] =
				temporary_nor[buffers[1].n_components * std::get<1>(it.first) + i];
		}

		for (size_t i = 0; i < buffers[2].n_components; ++i)
		{
			buffers[2].values[buffers[2].n_components * it.second + i] =
				temporary_uvs[buffers[2].n_components * std::get<2>(it.first) + i];
		}
	}

	return true;
}

void generateTangentVectors(
	std::vector<unsigned> const& indices,
	std::vector<float> const& positions,
	std::vector<float> const& uvs,
	std::vector<float>& tangents)
{
	for (unsigned i = 0; i < indices.size(); i += 3)
	{
		float x[3], y[3], z[3];
		float u[3], v[3];

		for (unsigned j = 0; j < 3; ++j)
		{
			unsigned it = 3 * (indices[i + j]);

			x[j] = positions[it];
			y[j] = positions[it + 1];
			z[j] = positions[it + 2];

			it = 2 * (indices[i + j]);

			u[j] = uvs[it];
			v[j] = uvs[it + 1];
		}

		float x_0 = x[1] - x[0];
		float y_0 = y[1] - y[0];
		float z_0 = z[1] - z[0];

		float x_1 = x[2] - x[0];
		float y_1 = y[2] - y[0];
		float z_1 = z[2] - z[0];

		float u_0 = u[1] - u[0];
		float u_1 = u[2] - u[0];

		float v_0 = v[1] - v[0];
		float v_1 = v[2] - v[0];

		float r = 1.0f / (u_0 * v_1 - u_1 * v_0);

		float u_dir_x = (v_1 * x_0 - v_0 * x_1) * r;
		float u_dir_y = (v_1 * y_0 - v_0 * y_1) * r;
		float u_dir_z = (v_1 * z_0 - v_0 * z_1) * r;

		for (unsigned j = 0; j < 3; ++j)
		{
			tangents[indices[i + j] * 3] += u_dir_x;
			tangents[indices[i + j] * 3 + 1] += u_dir_y;
			tangents[indices[i + j] * 3 + 2] += u_dir_z;
		}
	}
}

