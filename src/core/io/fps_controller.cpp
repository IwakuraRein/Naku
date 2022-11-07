#include "io/fps_controller.hpp"

#include <limits>
#include <iostream>

namespace naku {
FPSController::FPSController(
	Window& window,
	float moveSpeed,
	float moveSpeed2,
	float lookSpeed,
	float zoomSpeed,
	float translateSpeed,
	float rotateSpeed,
	glm::vec2 pitchLimit,
	glm::vec2 fovLimit)
	:_window{ window },
	moveSpeed{ moveSpeed },
	moveSpeed2{ moveSpeed2 },
	lookSpeed{ lookSpeed },
	zoomSpeed{ zoomSpeed },
	translateSpeed{ translateSpeed },
	rotateSpeed{ rotateSpeed },
	pitchLimit{ pitchLimit },
	fovLimit{ fovLimit } {
	lastX = _window.extent().width / 2;
	lastY = _window.extent().height / 2;
}

bool FPSController::movingRight() {
	return glfwGetKey(_window.pWindow(), inputMappings.moveRightKey) == GLFW_PRESS;
}
bool FPSController::movingLeft() {
	return glfwGetKey(_window.pWindow(), inputMappings.moveLeftKey) == GLFW_PRESS;
}
bool FPSController::movingUp() {
	return glfwGetKey(_window.pWindow(), inputMappings.moveUpKey) == GLFW_PRESS;
}
bool FPSController::movingDown() {
	return glfwGetKey(_window.pWindow(), inputMappings.moveDownKey) == GLFW_PRESS;
}
bool FPSController::movingForward() {
	return glfwGetKey(_window.pWindow(), inputMappings.moveForwardKey) == GLFW_PRESS;
}
bool FPSController::movingBackward() {
	return glfwGetKey(_window.pWindow(), inputMappings.moveBackwardKey) == GLFW_PRESS;
}
bool FPSController::lookingAround() {
	return (
		glfwGetMouseButton(_window.pWindow(), inputMappings.lookUnlockBtn) == GLFW_PRESS
		&& glfwGetKey(_window.pWindow(), inputMappings.lookUnlockKey) == GLFW_PRESS);
}
bool FPSController::zooming() {
	return (
		glfwGetMouseButton(_window.pWindow(), inputMappings.zoomUnlockBtn) == GLFW_PRESS
		&& glfwGetKey(_window.pWindow(), inputMappings.zoomUnlockKey) == GLFW_PRESS);
}
bool FPSController::translating() {
	return (
		glfwGetMouseButton(_window.pWindow(), inputMappings.translateUnlockBtn) == GLFW_PRESS
		/*and glfwGetKey(_window.pWindow(), inputMappings.translateUnlockKey) == GLFW_PRESS*/);
}

void FPSController::handleMouseInput(Camera* pCam, const glm::vec3& globalUp) {
	if (_window.isFocused()) {

		// cursor
		static double xPos, yPos;
		static float xOffset, yOffset;
		if (initFlag) {
			_window.getCursorPos(&xPos, &yPos);
			lastX = xPos;
			lastY = yPos;
			//lastX = _window.extent().width / 2;
			//lastY = _window.extent().height / 2;
			//_window.setCursorPos(lastX, lastY);
		}
		else {
			_window.getCursorPos(&xPos, &yPos);
			if (xPos > _window.viewPortCoord().x &&
				xPos < (_window.viewPortCoord().x + _window.viewPortWidth()) &&
				yPos > _window.viewPortCoord().y &&
				yPos < (_window.viewPortCoord().y + _window.viewPortHeight())) {

				xOffset = lastX - xPos;
				yOffset = -(lastY - yPos);

				if (!isZero(xOffset) || !isZero(yOffset)) {
					lastX = xPos;
					lastY = yPos;
					if (lookingAround()) {
						glm::vec3 forward{ pCam->forwardDir() }, up{ pCam->upDir() };
						const auto pitchMat = glm::rotate(glm::mat4{ 1.f }, -yOffset * lookSpeed, pCam->rightDir());
						forward = pitchMat * glm::vec4{ forward, 1.f };
						up = pitchMat * glm::vec4{ up, 1.f };
						const auto yawMat = glm::rotate(glm::mat4{ 1.f }, xOffset * lookSpeed, globalUp);
						forward = yawMat * glm::vec4{ forward, 1.f };
						up = yawMat * glm::vec4{ up, 1.f };
						pCam->orientate(forward, up);
						//pCam->rotate(-yOffset * lookSpeed, xOffset * lookSpeed, 0.f);
					}
					if (zooming()) {
						float fov = pCam->fov() + yOffset * zoomSpeed;
						fov = glm::clamp(fov, fovLimit.x, fovLimit.y);
						pCam->setFov(fov);
					}
					if (translating()) {
						const glm::vec3& forwardDir = pCam->forwardDir();
						const glm::vec3& rightDir = pCam->rightDir();
						const glm::vec3& upDir = pCam->upDir();

						glm::vec3 move = rightDir * translateSpeed * xOffset;
						move += upDir * translateSpeed * yOffset;
						pCam->move(move);
					}
				}
			}
			else {
				lastX = xPos;
				lastY = yPos;
			}
		}
		
		// scroll
		static double lastScrollY;
		static float scrollOffset;
		if (initFlag) {
			lastScrollY = _window.scroll_y;
		}
		else {
			if (xPos > _window.viewPortCoord().x &&
				xPos < (_window.viewPortCoord().x + _window.viewPortWidth()) &&
				yPos > _window.viewPortCoord().y &&
				yPos < (_window.viewPortCoord().y + _window.viewPortHeight())) {
				scrollOffset = -(lastScrollY - _window.scroll_y);
				if (!isZero(scrollOffset)) {
					lastScrollY = _window.scroll_y;
					const glm::vec3& forwardDir = pCam->forwardDir();
					if (!isZero(glm::dot(forwardDir, forwardDir))) {
						pCam->move(moveSpeed2 * scrollOffset * glm::normalize(forwardDir));
					}
				}
			}
			else lastScrollY = _window.scroll_y;
		}

		if (initFlag) {
			initFlag = false;
			return;
		}
	}

	else if (!initFlag) {
		initFlag = true;
	}
}

void FPSController::handleKeyBoardInput(Camera* pCam, float deltaTime) {
	if (_window.isFocused()) {
		const glm::vec3& forwardDir = pCam->forwardDir();
		const glm::vec3& rightDir = pCam->rightDir();
		const glm::vec3& upDir = pCam->upDir();

		glm::vec3 moveDir{ 0.f };
		if (movingForward()) moveDir += forwardDir;
		if (movingBackward()) moveDir -= forwardDir;
		if (movingRight()) moveDir += rightDir;
		if (movingLeft()) moveDir -= rightDir;
		if (movingUp()) moveDir += upDir;
		if (movingDown()) moveDir -= upDir;

		if (!isZero(glm::dot(moveDir, moveDir))) {
			pCam->move(moveSpeed * deltaTime * glm::normalize(moveDir));
		}
	}

}

}