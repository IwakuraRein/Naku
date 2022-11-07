#ifndef FPS_CONTROLLER
#define FPS_CONTROLLER

#include "resources/object.hpp"
#include "io/window.hpp"
#include "resources/camera.hpp"

namespace naku {

class FPSController {
public:
    struct InputMappings {
        int moveLeftKey = GLFW_KEY_A;
        int moveRightKey = GLFW_KEY_D;
        int moveForwardKey = GLFW_KEY_W;
        int moveBackwardKey = GLFW_KEY_S;
        int moveUpKey = GLFW_KEY_SPACE;
        int moveDownKey = GLFW_KEY_C;
        //int lookLeftKey = GLFW_KEY_LEFT;
        //int lookRightKey = GLFW_KEY_RIGHT;
        //int lookUpKey = GLFW_KEY_UP;
        //int lookDownKey = GLFW_KEY_DOWN;
        int lookUnlockKey = GLFW_KEY_LEFT_ALT;
        int lookUnlockBtn = GLFW_MOUSE_BUTTON_LEFT;
        int zoomUnlockBtn = GLFW_MOUSE_BUTTON_RIGHT;
        int zoomUnlockKey = GLFW_KEY_LEFT_ALT;
        //int translateUnlockKey = GLFW_KEY_LEFT_ALT;
        int translateUnlockBtn = GLFW_MOUSE_BUTTON_MIDDLE;
        int quitKey = GLFW_KEY_ESCAPE;
    };

    FPSController(
        Window& window,
        float moveSpeed = 3.0f,
        float moveSpeed2 = 0.5f,
        float lookSpeed = 0.002f,
        float zoomSpeed = 0.1f,
        float translateSpeed = 0.01f,
        float rotateSpeed = 0.01f,
        glm::vec2 pitchLimit = glm::vec2{ -90.f, 90.f },
        glm::vec2 fovLimit = glm::vec2{10.f, 170.f});

    FPSController() = default;
    FPSController(const FPSController&) = delete;
    FPSController& operator=(const FPSController&) = delete;
    FPSController(FPSController&&) = delete;
    FPSController& operator=(FPSController&&) = delete;

    InputMappings inputMappings;

    void handleKeyBoardInput(Camera* pCam, float deltaTime);
    void handleMouseInput(Camera* pCam, const glm::vec3& globalUp);

    bool movingRight();
    bool movingLeft();
    bool movingUp();
    bool movingDown();
    bool movingForward();
    bool movingBackward();
    bool lookingAround();
    bool zooming();
    bool translating();

    float moveSpeed;
    float moveSpeed2;
    float lookSpeed;
    float translateSpeed;
    float zoomSpeed;
    float rotateSpeed;
    glm::vec2 pitchLimit;
    glm::vec2 fovLimit;

private:
    Window& _window;
    bool initFlag{ true };
    float lastX, lastY;
    //static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
};

}

#endif