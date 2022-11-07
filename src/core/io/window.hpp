#ifndef WINDOW_HPP
#define WINDOW_HPP

#include "naku.hpp"

namespace naku {

constexpr int MIN_WINDOW_WIDTH = 1280;
constexpr int MIN_WINDOW_HEIGHT = 720;

constexpr float LEFT_PADDING = 180.f;
constexpr float RIGHT_PADDING = 270.f;
constexpr float BOTTOM_PADDING = 240.f;
constexpr float TOP_PADDING = 0.f;

class Window {
public:
	Window(const std::string& name, uint32_t w, uint32_t h, float dpi = 1.f);
	~Window();
	Window(const Window&) = delete;
	Window& operator=(const Window&) = delete;
	Window() = default;
	bool shouldClose();
	bool wasResized();
	void resetResizeFlag();
	VkExtent2D extent();
	VkExtent2D viewPortExtent();
	glm::vec2 viewPortCoord();
	uint32_t width() const { return _width; }
	uint32_t height() const{ return _height; }
	uint32_t viewPortWidth() const;
	uint32_t viewPortHeight() const;
	void createWindowSurface(VkInstance instance, VkSurfaceKHR* surface);
	bool isFocused() const { return glfwGetWindowAttrib(_pWindow, GLFW_FOCUSED); }
	GLFWwindow* pWindow() const { return _pWindow; }

	void setWindowName(const std::string& newName);
	void getCursorPos(double* xpos, double* ypos) const {
		glfwGetCursorPos(_pWindow, xpos, ypos);
	}
	void setCursorPos(double xpos, double ypos) const {
		glfwSetCursorPos(_pWindow, xpos, ypos);
	}
	void getScrollPos(double* scrollx, double* scrolly) const {
		*scrollx = scroll_x;
		*scrolly = scroll_y;
	}

	static double scroll_x, scroll_y;

	friend class GUI;

private:
	uint32_t _width;
	uint32_t _height;
	std::string _windowName;
	float _dpi, _left_padding, _right_padding, _bottom_padding, _top_padding;
	bool _framebufferResized = false;
	GLFWwindow* _pWindow;

	void initWindow();

	static void framebufferResizeCallback(GLFWwindow* pWindow, int w, int h);
	static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
};
}

#endif