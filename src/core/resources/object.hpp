#ifndef OBJECT_HPP
#define OBJECT_HPP

#include "naku.hpp"
#include "resources/model.hpp"
#include "resources/material.hpp"
#include "resources/resource.hpp"
#include "utils/buffer.hpp"

namespace naku {

class Object : public Resource
{
public:
	struct ModelInfo {
		glm::mat4 transformMat{ 1.f };
		glm::mat4 normalMat{ 1.f };
		glm::mat4 rotMat{ 1.f };
		int objId{-1};
		int receiveShadow{ 1 };
	};

	static ModelInfo* modelUbo;
	ModelInfo* getModelInfo() {
		return modelUbo + _id;
	}
	static std::vector<std::unique_ptr<Buffer>> modelUboBuffers;
	static void prepareObjectUbo(Device& device);
	static size_t dynamicAlignment;

	static std::map<ResId, uint32_t> changedObjects;

	enum Type {
		DELETED = 0,
		EMPTY   = 1,
		MESH    = 2,
		CAMERA  = 3,
		LIGHT   = 4, 
		ERROR   = 5
	};

	Object(
		Device& device,
		const std::string& name,
		Type type);
	~Object();
	Object(const Object&&) = delete;
	Object() = delete;
	Object& operator=(const Object&&) = delete;
	virtual void setId(ResId ID);
	Object* parent{nullptr};
	std::vector<Object> children;

	std::shared_ptr<Model> model;
	std::shared_ptr<Material> material;

	uint32_t getOffset() const {
		return static_cast<uint32_t>(_id * dynamicAlignment);
	}

	const glm::vec3& position() const { return _position; }
	const glm::vec3& scale() const { return _scale; }
	const glm::vec3& rotation()  const { return _rotation; }
	const glm::mat4& transformMat() const { return *_transformMat; }
	const glm::mat4& normalMat() const { return *_normalMat; }
	const glm::mat4& positionMat() const;
	const glm::mat4& scaleMat() const;
	const glm::mat4& rotationMat() const { return *_rotMat; };

	static const glm::mat4& getTransformMat(const glm::vec3& position, const glm::vec3& scale, const glm::vec3& rotation);
	static const glm::mat3& getNormalMat(const glm::vec3& scale, const glm::vec3& rotation);
	static const glm::mat3& getRotMat(const glm::vec3& rotation);

	Type type() const { return _type; }
	bool isActive() const { return _active; }
	bool castShadow() const { return _castShadow; }
	void activate() { _active = true; }
	void deactivate() { _active = false; }
	// TODO: void rename(std::string newName);

	virtual void move(const glm::vec3& deltaPos);
	virtual void setPosition(const glm::vec3& newPos);
	virtual void rotate(const glm::vec3& axisAngle);
	virtual void setRotation(const glm::vec3& newRot);
	virtual void setScale(const glm::vec3& newScale);
	virtual void move(float deltaPosX, float deltaPosY, float deltaPosZ) {
		move(glm::vec3{ deltaPosX, deltaPosY, deltaPosZ });
	}
	virtual void setPosition(float newPosX, float newPosY, float newPosZ) {
		setPosition(glm::vec3{ newPosX, newPosY, newPosZ });
	}
	virtual void setPosition(float* newPos) {
		setPosition(glm::vec3{ newPos[0], newPos[1], newPos[2] });
	}
	virtual void rotate(float axisAngleX, float axisAngleY, float axisAngleZ) {
		rotate(glm::vec3{ axisAngleX, axisAngleY, axisAngleZ });
	}
	virtual void rotate(float* axisAngle) {
		rotate(glm::vec3{ axisAngle[0], axisAngle[1], axisAngle[2] });
	}
	virtual void setRotation(float newRotX, float newRotY, float newRotZ) {
		setRotation(glm::vec3{ newRotX, newRotY, newRotZ });
	}
	virtual void setRotation(float* newRot) {
		setRotation(glm::vec3{ newRot[0], newRot[1], newRot[2] });
	}
	virtual void setScale(float newScaleX, float newScaleY, float newScaleZ) {
		setScale(glm::vec3{ newScaleX, newScaleY, newScaleZ });
	}
	virtual void setScale(float* newScale) {
		setScale(glm::vec3{ newScale[0], newScale[1], newScale[2]});
	}

	void writeToObjectBuffer();
	void writeToObjectBuffer(size_t frameIdx);

	friend class Scene;
	friend class GUI;

	virtual void update();

protected:
	Type _type;
	bool _active{ true };
	bool _castShadow{ true };
	glm::vec3 _position{ 0.0f, 0.0f, 0.0f };
	glm::vec3 _scale{ 1.0f, 1.0f, 1.0f };
	glm::vec3 _rotation{ 0.0f, 0.0f, 0.0f };
	glm::mat4* _transformMat{nullptr};
	glm::mat4* _normalMat{nullptr};
	glm::mat4* _rotMat{nullptr};

	static size_t _objectCount;
};
}

#endif