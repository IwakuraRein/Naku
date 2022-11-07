#ifndef CAMERA_HPP
#define CAMERA_HPP

#include "naku.hpp"
#include "resources/object.hpp"

namespace naku {

struct Projector {
    glm::vec3 position{ 0.f };
    glm::vec3 forwardDir{ 1.f };
    glm::vec3 rightDir{ 1.f };
    glm::vec3 upDir{ 1.f };
    glm::mat4 projMat{ 1.f };
    glm::mat4 viewMat{ 1.f };
    float left, right, top, bottom, fov, aspect, near, far;

    static glm::mat4 getOrthoProjMat(
        float left, float right, float top, float bottom, float near, float far);
    void setOrthoProjection(
        float left, float right, float top, float bottom, float near, float far);
    static glm::mat4 getPerspProjMat(float fov_y, float aspect, float near, float far);
    void setPerspProjection(float fov_y, float aspect, float near, float far);
    void setPerspProjection() {
        setPerspProjection(fov, aspect, near, far);
    }
    void setOrthoProjection() {
        setOrthoProjection(left, right, top, bottom, near, far);
    }
    static glm::mat4 getViewMatrixFromDirection(
        const glm::vec3& position, const glm::vec3& direction, const glm::vec3& up = glm::vec3{ 0.0f, -1.0f, 0.0f });
    static glm::mat4 getViewMatrix(const glm::vec3& position, const glm::vec3& rotation);
    void setViewMatrix(const glm::vec3& position, const glm::vec3& rotation);
    void orientate(const glm::vec3& direction, const glm::vec3& up);
    static glm::vec3 getRotationFromOrientation(const glm::vec3& direction, const glm::vec3& up);

};

class Camera : public Resource, public Object {

public:
    static constexpr float TrigonometricEpsilon= 0.001f;

    enum Type {
        ORTHO,
        PERSPECTIVE
    };

    Camera(
        Device& device,
        std::string name);
    Camera() = delete;
    virtual void setId(ResId id) {
        Resource::_id = id;
    }
    virtual void setObjId(ResId id) {
        Object::setId(id);
    }
    ResId id() const { return Resource::_id; }
    ResId objId() const { return Object::_id; }
    std::string name() const { return Resource::_name; }
    void setOrthoProjection(
        float left, float right, float top, float bottom, float near, float far);

    void setPerspProjection(float fov_y, float aspect, float near, float far);
    void setPerspProjection(float fov_y, glm::vec2 aspect, float near, float far);

    const glm::mat4& projMat() const { return _projector.projMat; }
    const glm::mat4& viewMat() const { return _projector.viewMat; }
    const glm::vec3& rightDir() const { return _projector.rightDir; }
    const glm::vec3& forwardDir() const { return _projector.forwardDir; }
    const glm::vec3& upDir() const { return _projector.upDir; }
    float fov() const;
    float tanFov() const;
    float near() const { return _projector.near; };
    float far() const { return _projector.far; };

    void updateProjectionMat();
    void updateViewMat();

    void orientate(
        const glm::vec3& direction,
        const glm::vec3& up = glm::vec3{ 0.f, -1.f, 0.f });

    void lookAt(
        glm::vec3 lookAt, glm::vec3 up = glm::vec3{ 0.f, -1.f, 0.f });

    void setAspect(float aspect);
    void setAspect(glm::vec2 aspect);
    void setFov(float fov_y);

    virtual void update();

    friend class GUI;
    friend class Scene;
    friend class FPSController;

private:
    Type _type;
    Projector _projector;
};

}

#endif