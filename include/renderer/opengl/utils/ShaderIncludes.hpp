#ifndef SHADER_INCLUDES_H
#define SHADER_INCLUDES_H

#include <filesystem>
#include <string>
#include <vector>

/**
 * \brief This is the result of unwrapped #include directive from a GLSL source code;
 */
class ShaderIncluder
{
public:
	/**
	 * \param source_code glsl code with #include directives
	 * \param source_path glsl code file path (used to find glsl include relatively to this file)
	 */
	ShaderIncluder(const std::string& source_code, const std::filesystem::path& source_path);

	/**
	 * \brief This class is used to retrieve the original error line and file from an unwrapped code
	 * (because #includes will make error recovery harder without it)
	 */
	class ShaderBlock
    {
	public:
	    /**
	     * \brief Get the file and the line corresponding to the input global line (global from parent's block point of view)
	     */
	    void get_location_from_global_line(size_t global_line, std::filesystem::path& file_path, size_t& local_line) const;

	    std::vector<ShaderBlock> inside_blocks;
        std::filesystem::path block_path;
        size_t size = 0;
        size_t start = 0;
    };

    ShaderBlock root_block;
	std::string unwrapped_code;

    void print_error(const std::string& context, const std::string& error) const;
private:
	[[nodiscard]] std::string parse_glsl_error_line(const std::string& error) const;
    std::string unwrap_internal(const std::string& source_code, const std::filesystem::path& source_path, ShaderBlock& out_block);
};



#endif // SHADER_INCLUDES_