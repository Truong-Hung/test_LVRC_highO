#ifndef LVRC_HPP
#define LVRC_HPP

#include <optional>

#include "ui/Window.hpp"
#include "renderer/Renderer.hpp"
#include "mesh/Mesh.hpp"

class LVRC final
{
private:
	std::shared_ptr<Mesh> mesh_;
	Window window_;

public:
	explicit LVRC(
		uint32_t width, 
		uint32_t height);
	~LVRC();

	void launch();
	void reload_with_mesh(const std::string& file_path);

private:
	std::shared_ptr<Camera> globalCamera;
	void load_renderer();
};

#endif
