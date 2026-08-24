#include "ui/FilePicker.hpp"

#include <iostream>
#include <string>

#include "imgui.h"

std::optional<std::filesystem::path> FilePicker::select(const std::filesystem::path& rootPath,
                                                        const std::vector<std::string>& allowedExtensions)
{
	std::optional<std::filesystem::path> selected_file = {};
	for (const auto& file : std::filesystem::directory_iterator(rootPath))
	{
		if (file.is_regular_file())
		{
			if (!allowedExtensions.empty())
			{
				bool foundExt = false;
				for (const auto& ext : allowedExtensions)
				{
					if (file.path().extension().string() == ext)
					{
						foundExt = true;
						break;
					}
				}
				if (!foundExt)
					continue;
			}
			if (ImGui::MenuItem(
				(file.path().filename().string() + " (" + std::to_string(file.file_size() / 1000000) + "Mo)").c_str()))
				selected_file = file.path();
		}
		else if (file.is_directory())
		{
			if (ImGui::BeginMenu(file.path().filename().string().c_str()))
			{
				if (const auto innerFile = select(file.path(), allowedExtensions))
					selected_file = innerFile;
				ImGui::EndMenu();
			}
		}
	}

	return selected_file;
}
