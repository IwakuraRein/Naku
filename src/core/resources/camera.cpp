#include "naku.hpp"
#include "resources/camera.hpp"
#include "utils/swap_chain.hpp"

#include <iostream>

namespace naku { //projector

glm::mat4 Projector::getOrthoProjMat(
    float Left, float Right, float Top, float Bottom, float Near, float Far)
{
    glm::mat4 ProjMat{ 1.f };
    ProjMat[0][0] = 2.f / (Right - Left);
    ProjMat[1][1] = 2.f / (Bottom - Top);
    ProjMat[2][2] = 1.f / (Far - Near);
    ProjMat[3][0] = -(Right + Left) / (Right - Left);
    ProjMat[3][1] = -(Bottom + Top) / (Bottom - Top);
    ProjMat[3][2] = -Near / (Far - Near);
    return ProjMat;
}

void Projector::setOrthoProjection(float Left, float Right, float Top, float Bottom, float Near, float Far)
{
    left = Left;
    right = Right;
    top = Top;
    bottom = Bottom;
    near = Near;
    far = Far;
    projMat = getOrthoProjMat(Left, Right, Top, Bottom, Near, Far);
}

glm::mat4 Projector::getPerspProjMat(float fov_y, float Aspect, float Near, float Far)
{
    assert(Aspect > 0.f && "Error: Camera aspect cannot be negetive.");
    glm::mat4 ProjMat{ 1.f };
    const float tanHalfFovy = glm::tan(glm::radians(fov_y / 2.f));
    ProjMat = glm::mat4{ 0.0f };
    ProjMat[0][0] = 1.f / (Aspect * tanHalfFovy);
    ProjMat[1][1] = 1.f / (tanHalfFovy);
    ProjMat[2][2] = Far / (Far - Near);
    ProjMat[2][3] = 1.f;
    ProjMat[3][2] = -(Far * Near) / (Far - Near);
    return ProjMat;
}

void Projector::setPerspProjection(float fov_y, float Aspect, float Near, float Far)
{
    assert(Aspect > 0.f && "Error: Camera aspect cannot be negetive.");
    fov = fov_y;
    aspect = Aspect;
    near = Near;
    far = Far;
    projMat = getPerspProjMat(fov_y, Aspect, Near, Far);
}

glm::mat4 Projector::getViewMatrixFromDirection(const glm::vec3& Position, const glm::vec3& ForwardDir, const glm::vec3& UpDir) {
    glm::mat4 ViewMat;
    const glm::vec3 RightDir = glm::cross(ForwardDir, UpDir);
    ViewMat = glm::mat4{ 1.f };
    ViewMat[0][0] = RightDir.x;
    ViewMat[1][0] = RightDir.y;
    ViewMat[2][0] = RightDir.z;
    ViewMat[0][1] = -UpDir.x;
    ViewMat[1][1] = -UpDir.y;
    ViewMat[2][1] = -UpDir.z;
    ViewMat[0][2] = ForwardDir.x;
    ViewMat[1][2] = ForwardDir.y;
    ViewMat[2][2] = ForwardDir.z;
    ViewMat[3][0] = -glm::dot(RightDir, Position);
    ViewMat[3][1] = -glm::dot(-UpDir, Position);
    ViewMat[3][2] = -glm::dot(ForwardDir, Position);
    return ViewMat;
}

glm::mat4 Projector::getViewMatrix(const glm::vec3& Position, const glm::vec3& Rotation)
{
    glm::mat4 ViewMat;
    const float c3 = glm::cos(glm::radians(Rotation.z));
    const float s3 = glm::sin(glm::radians(Rotation.z));
    const float c2 = glm::cos(glm::radians(Rotation.x));
    const float s2 = glm::sin(glm::radians(Rotation.x));
    const float c1 = glm::cos(glm::radians(Rotation.y));
    const float s1 = glm::sin(glm::radians(Rotation.y));
    auto RightDir = glm::vec3{ (c1 * c3 + s1 * s2 * s3), (c2 * s3), (c1 * s2 * s3 - c3 * s1) };
    auto UpDir = glm::vec3{ -(c3 * s1 * s2 - c1 * s3), -(c2 * c3), -(c1 * c3 * s2 + s1 * s3) };
    auto ForwardDir = glm::vec3{ (c2 * s1), (-s2), (c1 * c2) };
    ViewMat = glm::mat4{ 1.f };
    ViewMat[0][0] = RightDir.x;
    ViewMat[1][0] = RightDir.y;
    ViewMat[2][0] = RightDir.z;
    ViewMat[0][1] = -UpDir.x;
    ViewMat[1][1] = -UpDir.y;
    ViewMat[2][1] = -UpDir.z;
    ViewMat[0][2] = ForwardDir.x;
    ViewMat[1][2] = ForwardDir.y;
    ViewMat[2][2] = ForwardDir.z;
    ViewMat[3][0] = -glm::dot(RightDir, Position);
    ViewMat[3][1] = -glm::dot(-UpDir, Position);
    ViewMat[3][2] = -glm::dot(ForwardDir, Position);

    return ViewMat;
}

void Projector::setViewMatrix(const glm::vec3& Position, const glm::vec3& Rotation)
{
    position = Position;
    const float c3 = glm::cos(glm::radians(Rotation.z));
    const float s3 = glm::sin(glm::radians(Rotation.z));
    const float c2 = glm::cos(glm::radians(Rotation.x));
    const float s2 = glm::sin(glm::radians(Rotation.x));
    const float c1 = glm::cos(glm::radians(Rotation.y));
    const float s1 = glm::sin(glm::radians(Rotation.y));
    rightDir = glm::vec3{ (c1 * c3 + s1 * s2 * s3), (c2 * s3), (c1 * s2 * s3 - c3 * s1) };
    upDir = glm::vec3{ -(c3 * s1 * s2 - c1 * s3), -(c2 * c3), -(c1 * c3 * s2 + s1 * s3) };
    forwardDir = glm::vec3{ (c2 * s1), (-s2), (c1 * c2) };
    viewMat = glm::mat4{ 1.f };
    viewMat[0][0] = rightDir.x;
    viewMat[1][0] = rightDir.y;
    viewMat[2][0] = rightDir.z;
    viewMat[0][1] = -upDir.x;
    viewMat[1][1] = -upDir.y;
    viewMat[2][1] = -upDir.z;
    viewMat[0][2] = forwardDir.x;
    viewMat[1][2] = forwardDir.y;
    viewMat[2][2] = forwardDir.z;
    viewMat[3][0] = -glm::dot(rightDir, position);
    viewMat[3][1] = -glm::dot(-upDir, position);
    viewMat[3][2] = -glm::dot(forwardDir, position);
}

void Projector::orientate(const glm::vec3& direction, const glm::vec3& up)
{
    forwardDir = glm::normalize(direction);
    upDir = glm::normalize(up);
    rightDir = glm::cross(forwardDir, upDir);
    viewMat[0][0] = rightDir.x;
    viewMat[1][0] = rightDir.y;
    viewMat[2][0] = rightDir.z;
    viewMat[0][1] = -upDir.x;
    viewMat[1][1] = -upDir.y;
    viewMat[2][1] = -upDir.z;
    viewMat[0][2] = forwardDir.x;
    viewMat[1][2] = forwardDir.y;
    viewMat[2][2] = forwardDir.z;
    viewMat[3][0] = -glm::dot(rightDir, position);
    viewMat[3][1] = -glm::dot(-upDir, position);
    viewMat[3][2] = -glm::dot(forwardDir, position);
}

glm::vec3 Projector::getRotationFromOrientation(const glm::vec3& direction, const glm::vec3& up)
{
    static const float Epsilon = 0.00001f;
    glm::vec3 forwardDir = glm::normalize(direction);
    glm::vec3 upDir = glm::normalize(up);
    glm::vec3 rightDir = glm::cross(forwardDir, upDir);
    const float sinX = -forwardDir.y;

    const float xx = asin(sinX); // -90 ~ 90
    float x;
    if (forwardDir.z >= 0 && forwardDir.y >= 0) // -90 ~ 0
        x = xx;
    if (forwardDir.z >= 0 && forwardDir.y < 0) // 0 ~ 90
        x = xx;
    if (forwardDir.z < 0 && forwardDir.y < 0) // 90 ~ 180
        x = glm::pi<float>() - xx;
    if (forwardDir.z < 0 && forwardDir.y >= 0) // -180 ~ -90
        x = -xx - glm::pi<float>();

    const float cosX = cos(x);

    float cosY, sinY, y, cosZ, sinZ, z;

    if (glm::epsilonEqual(cosX, 0.f, Epsilon)) {
        glm::vec3 newForward = Object::getRotMat({ -glm::degrees(x), 0.f, 0.f }) * forwardDir;
        glm::vec3 ForwardXOZ{ newForward.x, 0.f, newForward.z };
        ForwardXOZ = glm::normalize(ForwardXOZ);
        cosY = glm::dot(ForwardXOZ, { 0.f, 0.f, 1.f });
        sinY = glm::length(glm::cross(ForwardXOZ, { 0.f, 0.f, 1.f }));
        y = atan2(sinY, cosY);

        glm::vec3 newUp = Object::getRotMat({ -glm::degrees(x), -glm::degrees(y), 0.f }) * upDir;
        glm::vec3 upXOY{ newForward.x, newForward.y, 0.f };
        upXOY = glm::normalize(upXOY);
        cosZ = glm::dot(upXOY, { 0.f, -1.f, 0.f });
        sinZ = glm::length(glm::cross(upXOY, { 0.f, -1.f, 0.f }));
        z = atan2(sinZ, cosZ);
    }
    else { // cosX != 0
        cosY = glm::clamp(forwardDir.z / cosX, -1.f, 1.f);
        sinY = glm::clamp(forwardDir.x / cosX, -1.f, 1.f);
        y = atan2(sinY, cosY);
        sinZ = glm::clamp(rightDir.y / cosX, -1.f, 1.f);
        cosZ = glm::clamp(-upDir.y / cosX, -1.f, 1.f);
        z = atan2(sinZ, cosZ);
    }
    return glm::vec3(glm::degrees(x), glm::degrees(y), glm::degrees(z));
}

}

namespace naku { //camera

Camera::Camera(
    Device& device,
    std::string name)
    : Resource{ device, name }, Object {
    device, name, Object::Type::CAMERA
} {
    setPerspProjection(60.0f, 1.77777777f, 0.1f, 100.0f);
}

void Camera::setOrthoProjection(
    float left, float right, float top, float bottom, float near, float far){
    _type = Type::ORTHO;
    _projector.setOrthoProjection(
        left, right, top, bottom, near, far);
}

void Camera::setPerspProjection(float fov_y, float aspect, float near, float far) {
    assert(glm::abs(aspect - std::numeric_limits<float>::epsilon()) > 0.0f);
    _type = Type::PERSPECTIVE;
    _projector.setPerspProjection(fov_y, aspect, near, far);
}

float Camera::fov() const {
    float fov = 1 / _projector.projMat[1][1];
    fov = glm::degrees(glm::atan(fov)) * 2.f;
    return fov;
}

float Camera::tanFov() const {
    float fov = 1 / _projector.projMat[1][1];
    return fov;
}

void Camera::setPerspProjection(float fov_y, glm::vec2 aspect, float near, float far) {
    setPerspProjection(fov_y, aspect.x / aspect.y, near, far);
}

void Camera::orientate(const glm::vec3& direction, const glm::vec3& up) {
    _projector.orientate(direction, up);
    _rotation = _projector.getRotationFromOrientation(direction, up);
    Object::update();
}

void Camera::lookAt(glm::vec3 lookAt, glm::vec3 up) {
    orientate(lookAt - _position, up);
}

void Camera::updateProjectionMat() {
    if (_type == Type::PERSPECTIVE) {
        _projector.setPerspProjection();
    }
    else if (_type == Type::ORTHO) {
        _projector.setOrthoProjection();
    }
}

void Camera::updateViewMat() {
    _projector.setViewMatrix(_position, _rotation);
}

void Camera::setFov(float fov_y) {
    _projector.fov = fov_y;
    updateProjectionMat();
}

void Camera::setAspect(float aspect) {
    _projector.aspect = aspect;
    updateProjectionMat();
}

void Camera::setAspect(glm::vec2 aspect) {
    setAspect(aspect.x / aspect.y);
}

void Camera::update() {
    Object::update();
    updateViewMat();
}

}