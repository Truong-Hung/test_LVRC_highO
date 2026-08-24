#ifndef FILE_PICKER_H_
#define FILE_PICKER_H_

#include <vector>
#include <filesystem>
#include <optional>



class FilePicker final
{
public:
	FilePicker() = delete;
	static std::optional<std::filesystem::path> select(const std::filesystem::path& rootPath, const std::vector<std::string>& allowedExtensions = {});
};

#endif // FILE_PICKER_H_
