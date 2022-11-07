#include "io/window.hpp"

namespace naku {

double Window::scroll_x = 0.0;
double Window::scroll_y = 0.0;

Window::Window(const std::string& name, uint32_t w, uint32_t h, float dpi) : _width{ w }, _height{ h }, _windowName{ name }, _dpi{ dpi }{
	initWindow();

	_left_padding = LEFT_PADDING * _dpi;
	_right_padding = RIGHT_PADDING * _dpi;
	_bottom_padding = BOTTOM_PADDING * _dpi;
	_top_padding = TOP_PADDING * _dpi;
}

Window::~Window() {
	glfwDestroyWindow(_pWindow);
	glfwTerminate();
}

void Window::initWindow() {
	glfwInit();

	// glfwWindowHint(int hint, int value): set value to hint
	// GLFW uses OpenGL as defualt. Disable this with GLFW_NO_API
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	// Disable resizing window
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
	// The fourth parameter allows you to optionally specify a monitor to open the window on and the last parameter is only relevant to OpenGL.
	_pWindow = (glfwCreateWindow(_width, _height, _windowName.c_str(), nullptr, nullptr));
	glfwSetWindowUserPointer(_pWindow, this);
	// Register callback
	glfwSetFramebufferSizeCallback(_pWindow, framebufferResizeCallback);
	//glfwSetWindowFocusCallback(_window, windowFocusCallback);
	glfwSetScrollCallback(_pWindow, scrollCallback);
	glfwSetWindowSizeLimits(_pWindow, MIN_WINDOW_WIDTH, MIN_WINDOW_HEIGHT, GLFW_DONT_CARE, GLFW_DONT_CARE);
}
bool Window::shouldClose() {
	return glfwWindowShouldClose(_pWindow);
}

void Window::createWindowSurface(VkInstance instance, VkSurfaceKHR* surface) {
	if (glfwCreateWindowSurface(instance, _pWindow, nullptr, surface) != VK_SUCCESS) {
		throw std::runtime_error("failed to create window surface.");
	}
}

VkExtent2D Window::extent() {
	return { _width, _height };
}
uint32_t Window::viewPortWidth() const {
	if (_width > static_cast<uint32_t>(_left_padding + _right_padding))
		return _width - static_cast<uint32_t>(_left_padding + _right_padding);
	return 0;
}
uint32_t Window::viewPortHeight() const {
	if (_height > static_cast<uint32_t>(_bottom_padding + _top_padding))
		return  _height - static_cast<uint32_t>(_bottom_padding + _top_padding);
	return 0;
}
VkExtent2D Window::viewPortExtent() {
	return { viewPortWidth(), viewPortHeight() };
}
glm::vec2 Window::viewPortCoord() {
	return { _left_padding,  _top_padding };
}

bool Window::wasResized() {
	return _framebufferResized;
}

void Window::resetResizeFlag() {
	_framebufferResized = false;
}

void Window::framebufferResizeCallback(GLFWwindow* pGLFWWindow, int w, int h) {
	auto pWindow = reinterpret_cast<Window*>(glfwGetWindowUserPointer(pGLFWWindow));
	pWindow->_framebufferResized = true;
	pWindow->_width = w;
	pWindow->_height = h;
}

void Window::scrollCallback(GLFWwindow* pGLFWWindow, double xoffset, double yoffset) {
	scroll_x += xoffset;
	scroll_y += yoffset;
}

void Window::setWindowName(const std::string& newName) {
	_windowName = newName;
	glfwSetWindowTitle(_pWindow, newName.c_str());
}

}