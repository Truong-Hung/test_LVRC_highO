#include "renderer/opengl/utils/ShaderIncludes.hpp"

#include <cstring>
#include <fstream>
#include <iostream>

static std::vector<std::string> split_string(const std::string& in, std::vector<char> delims)
{
	std::vector<std::string> result;
	std::string current;
	for (const auto chr : in)
	{
		bool found = false;
		for (const auto delim : delims)
			if (chr == delim)
			{
				if (!current.empty())
					result.push_back(current);
				current.clear();
				found = true;
				break;
			}
		if (!found)
			current += chr;
	}
	if (!current.empty())
		result.push_back(current);
	return result;
}

static std::string concat_str_list(const std::vector<std::string>& strings, char sep = ' ')
{
	std::string result;
	bool does_sep = false;
	for (const auto& string : strings)
	{
		if (!does_sep)
			does_sep = true;
		else
			result += sep;
		result += string;
	}
	return result;
}

ShaderIncluder::ShaderIncluder(const std::string& source_code, const std::filesystem::path& source_path)
{
	unwrapped_code = unwrap_internal(source_code, source_path, root_block);
}

void ShaderIncluder::ShaderBlock::get_location_from_global_line(size_t global_line,
                                                                        std::filesystem::path& file_path,
                                                                        size_t& local_line) const
{
	size_t internalSkipped = 0;
	for (const auto& inside : inside_blocks)
	{
		if (global_line >= inside.start && global_line < inside.start + inside.size)
		{
			inside.get_location_from_global_line(global_line - inside.start + 1, file_path, local_line);
			return;
		}
		internalSkipped += inside.size;
	}
	
	file_path = block_path;
	local_line = global_line - internalSkipped;
}

void ShaderIncluder::print_error(const std::string& context, const std::string& error) const
{
	std::string error_message = context + "\n";
	error_message += parse_glsl_error_line(error) + "\n";

	std::cerr << error_message << std::endl;
	throw std::runtime_error(error_message.c_str());
}

std::string ShaderIncluder::parse_glsl_error_line(const std::string& error) const
{
	std::string result;
	for (const std::string& line : split_string(error, {'\n'}))
	{
		auto res = split_string(line, {':'});
		if (res.empty())
		{
			result += line + "\n";
			continue;
		}
		// Get "line" field
		const auto line_str = split_string(res[0], {'(', ')'});
		res.erase(res.begin()); // Remove old line number
		if (line_str.size() >= 2)
		{
			char* end;
			const long found_line_number = strtol(line_str[1].c_str(), &end, 10);
			if (*end == '\0')
			{
				std::filesystem::path file_path;
				size_t error_line;
				root_block.get_location_from_global_line(found_line_number, file_path, error_line);
				result += file_path.string() + ":" + std::to_string(error_line) + " :" + concat_str_list(res, ':') + "\n";
			}
			else
			{
				result += line + "\n";
			}
		}
	}
	return result;
}

std::string ShaderIncluder::unwrap_internal(const std::string& source_code,
                                                    const std::filesystem::path& source_path, ShaderBlock& out_block)
{
	std::string output_code;
	out_block.block_path = source_path;

	// Test if the "string" is equals to the "sources" at position "index"
	const auto match = [](const std::string& sources, size_t index, const char* string)
	{
		const auto string_length = strlen(string);
		if (index + string_length > sources.length())
			return false;
		for (size_t i = 0; i < string_length; ++i)
			if (sources[index + i] != string[i])
				return false;
		return true;
	};


	// True until the first not-empty character
	bool new_line = true;
	bool is_include_line = false;
	bool reading_include = false;
	std::string current_include_path;
	for (size_t i = 0; i < source_code.length(); ++i)
	{
		if (new_line && match(source_code, i, "#include"))
		{
			is_include_line = true;
			i += strlen("#include"); // Skip the #include directive to read the content
		}
		if (is_include_line)
		{
			if (!reading_include && source_code[i] == '"') // open quote
			{
				reading_include = true;
				i += 1;
			}
			if (reading_include)
			{
				if (source_code[i] == '"') // close quote
				{
					reading_include = false;
					is_include_line = false;

					// Read and add include
					std::string included;
					const auto file_path = source_path.parent_path() / current_include_path;
					current_include_path.clear();
					std::ifstream file(file_path, std::ios::in);
					if (!file.good())
					{
						std::cerr << "Unable to open file '" << file_path << "'." << std::endl;
						throw std::runtime_error("Unable to open file '" + file_path.string() + "'.");
					}

					std::string line;
					std::string included_code;
					bool add_line_break = false;
					while (std::getline(file, line))
					{
						if (!add_line_break)
							add_line_break = true;
						else included_code += "\n";

						included_code += line;
					}
					ShaderBlock inside_block;
					output_code += unwrap_internal(included_code, file_path.string(), inside_block);
					inside_block.start = out_block.size + 1;
					out_block.size += inside_block.size;
					out_block.inside_blocks.push_back(inside_block);
				}
				else
					current_include_path += source_code[i];
			}

			continue; // Don't add #include directive to the resulting code
		}

		// Set "new_line" to true or false
		if (source_code[i] == '\n')
		{
			out_block.size++;
			new_line = true;
		}
		else if (source_code[i] != ' ' && source_code[i] != '\t')
			new_line = false;

		// Copy source code to output code
		output_code += source_code[i];
	}

	return output_code;
}
