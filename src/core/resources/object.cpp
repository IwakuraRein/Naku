#include "resources/object.hpp"
#include "utils/swap_chain.hpp"

#include <iostream>
#include <exception>

namespace naku {

Object::ModelInfo* Object::modelUbo{nullptr};
std::vector<std::unique_ptr<Buffer>> Object::modelUboBuffers{};
std::map<ResId, uint32_t> Object::changedObjects{};
size_t Object::_objectCount {0};
size_t Object::dynamicAlignment{0};

void Object::prepareObjectUbo(Device& device) {
	const size_t minUboAlignment = device.properties.limits.minUniformBufferOffsetAlignment;
	dynamicAlignment = Buffer::getAlignment(sizeof(Object::ModelInfo), minUboAlignment);
	const size_t bufferSize = MAX_OBJECT_NUM * dynamicAlignment;
	modelUbo = (Object::ModelInfo*)alignedAlloc(bufferSize, dynamicAlignment);
	modelUboBuffers.resize(MAX_FRAMES_IN_FLIGHT);
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		// do not specify the VK_MEMORY_PROPERTY_HOST_COHERENT_BIT flag. 
		// We will only update the parts of the dynamic buffer that actually 
		// changed (e.g. only objects that moved since the last frame) and 
		// do a manual flush of the updated buffer memory for better performance.
		modelUboBuffers[i] = std::make_unique<Buffer>(
			device,
			dynamicAlignment,
			MAX_OBJECT_NUM,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		//modelUboBuffers[i]->map();
	}
}

Object::Object(
	Device& device,
	const std::string& name,
	Type type)
	: Resource(device, name), _type{type} {

	_position = glm::vec3{ 0.f };
	_scale = glm::vec3{ 1.f };
	_rotation = glm::vec3{ 0.f };

	_objectCount++;
}

Object::~Object() {
	if (--_objectCount == 0) {
		if (modelUbo)
			alignedFree(modelUbo);
		for (size_t i = 0; i < modelUboBuffers.size(); i++) {
			modelUboBuffers[i].reset(nullptr);
		}
	}
}

void Object::setId(ResId ID) {
	_id = ID;
	_transformMat = &((modelUbo + _id)->transformMat);
	_normalMat = &((modelUbo + _id)->normalMat);
	_rotMat = &((modelUbo + _id)->rotMat);
	*_transformMat = glm::mat4{ 1.f };
	*_normalMat = glm::mat4{ 1.f };
	*_rotMat = glm::mat4{ 1.f };
	(modelUbo + _id)->objId = _id;
	(modelUbo + _id)->receiveShadow = 1;
}
void Object::writeToObjectBuffer(size_t frameIdx) {
	modelUboBuffers[frameIdx]->writeToIndex(modelUbo + _id, _id);
	if (MapHas(changedObjects, _id)) {
		--changedObjects[_id];
	}
}

void Object::writeToObjectBuffer() {
	for (int i=0;i< modelUboBuffers.size();i++)
		modelUboBuffers[i]->writeToIndex(modelUbo + _id, _id);

	if (MapHas(changedObjects, _id))
		changedObjects.erase(_id);
}

void Object::move(const glm::vec3& deltaPos) {
	_position += deltaPos;
	update();
}
void Object::setPosition(const glm::vec3& newPos) {
	_position = newPos;
	update();
}
void Object::rotate(const glm::vec3& axisAngle) {
	_rotation += axisAngle;
	_rotation = glm::mod(_rotation, glm::vec3(360.0f, 360.0f, 360.0f));
	update();
}
void Object::setRotation(const glm::vec3& newRot) {
	_rotation = newRot;
	_rotation = glm::mod(_rotation, glm::vec3(360.0f, 360.0f, 360.0f));
	update();
}
void Object::setScale(const glm::vec3& newScale) {
	_scale = newScale;
	update();
}

const glm::mat4& Object::getTransformMat(const glm::vec3& position, const glm::vec3& scale, const glm::vec3& rotation) {
	const float c3 = glm::cos(glm::radians(rotation.z));
	const float s3 = glm::sin(glm::radians(rotation.z));
	const float c2 = glm::cos(glm::radians(rotation.x));
	const float s2 = glm::sin(glm::radians(rotation.x));
	const float c1 = glm::cos(glm::radians(rotation.y));
	const float s1 = glm::sin(glm::radians(rotation.y));
	glm::mat4 transformMat {
			{   scale.x * (c1 * c3 + s1 * s2 * s3),
				scale.x * (c2 * s3),
				scale.x * (c1 * s2 * s3 - c3 * s1),
				0.0f,
			},
			{   scale.y * (c3 * s1 * s2 - c1 * s3),
				scale.y * (c2 * c3),
				scale.y * (c1 * c3 * s2 + s1 * s3),
				0.0f,
			},
			{   scale.z * (c2 * s1),
				scale.z * (-s2),
				scale.z * (c1 * c2),
				0.0f,
			},
			{ position.x, position.y, position.z, 1.0f }
	};
	return transformMat;
}

const glm::mat3& Object::getNormalMat(const glm::vec3& scale, const glm::vec3& rotation) {
	const float c3 = glm::cos(glm::radians(rotation.z));
	const float s3 = glm::sin(glm::radians(rotation.z));
	const float c2 = glm::cos(glm::radians(rotation.x));
	const float s2 = glm::sin(glm::radians(rotation.x));
	const float c1 = glm::cos(glm::radians(rotation.y));
	const float s1 = glm::sin(glm::radians(rotation.y));
	const glm::vec3 invScale = 1.0f / scale;

	return glm::mat3{
		{
			invScale.x * (c1 * c3 + s1 * s2 * s3),
			invScale.x * (c2 * s3),
			invScale.x * (c1 * s2 * s3 - c3 * s1),
		},
		{
			invScale.y * (c3 * s1 * s2 - c1 * s3),
			invScale.y * (c2 * c3),
			invScale.y * (c1 * c3 * s2 + s1 * s3),
		},
		{
			invScale.z * (c2 * s1),
			invScale.z * (-s2),
			invScale.z * (c1 * c2),
		},
	};
}

const glm::mat3& Object::getRotMat(const glm::vec3& rotation) {
	const float c3 = glm::cos(glm::radians(rotation.z));
	const float s3 = glm::sin(glm::radians(rotation.z));
	const float c2 = glm::cos(glm::radians(rotation.x));
	const float s2 = glm::sin(glm::radians(rotation.x));
	const float c1 = glm::cos(glm::radians(rotation.y));
	const float s1 = glm::sin(glm::radians(rotation.y));

	return glm::mat3{
		{
			(c1 * c3 + s1 * s2 * s3),
			(c2 * s3),
			(c1 * s2 * s3 - c3 * s1),
		},
		{
			(c3 * s1 * s2 - c1 * s3),
			(c2 * c3),
			(c1 * c3 * s2 + s1 * s3),
		},
		{
			(c2 * s1),
			(-s2),
			(c1 * c2),
		},
	};
}

void Object::update() {
	*_rotMat = getRotMat(_rotation);
	*_transformMat = glm::mat4{
		{   _scale.x * (*_rotMat)[0][0],
			_scale.x * (*_rotMat)[0][1],
			_scale.x * (*_rotMat)[0][2],
			0.0f,
		},
		{   _scale.y * (*_rotMat)[1][0],
			_scale.y * (*_rotMat)[1][1],
			_scale.y * (*_rotMat)[1][2],
			0.0f,
		},
		{   _scale.z * (*_rotMat)[2][0],
			_scale.z * (*_rotMat)[2][1],
			_scale.z * (*_rotMat)[2][2],
			0.0f,
		},
		{ _position.x, _position.y, _position.z, 1.0f }
	};
	const glm::vec3 invScale = 1.0f / _scale;
	*_normalMat = glm::mat3{
		{
			invScale.x * (*_rotMat)[0][0],
			invScale.x * (*_rotMat)[0][1],
			invScale.x * (*_rotMat)[0][2],
		},
		{
			invScale.y * (*_rotMat)[1][0],
			invScale.y * (*_rotMat)[1][1],
			invScale.y * (*_rotMat)[1][2],
		},
		{
			invScale.z * (*_rotMat)[2][0],
			invScale.z * (*_rotMat)[2][1],
			invScale.z * (*_rotMat)[2][2],
		},
	};
	if (parent) {
		*_transformMat = *parent->_transformMat * *_transformMat;
		*_normalMat = *parent->_normalMat * *_normalMat;
		*_rotMat = *parent->_rotMat * *_rotMat;
	}

	for (auto& obj : children) {
		obj.update();
	}
	changedObjects[_id] = MAX_FRAMES_IN_FLIGHT;
	//writeToBuffer();
}

const glm::mat4& Object::positionMat() const {
	const glm::mat4 identity(1.0f);
	return glm::translate(identity, _position);
}
const glm::mat4& Object::scaleMat() const {
	const glm::mat4 identity(1.0f);
	return glm::scale(identity, _scale);
}

}