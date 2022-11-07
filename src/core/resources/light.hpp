#ifndef LIGHT_HPP
#define LIGHT_HPP

#include "naku.hpp"
#include "resources/object.hpp"
#include "resources/camera.hpp"
#include "utils/render_pass.hpp"

namespace naku {

class Light : public Resource, public Object {
public:
	enum Type {
		POINT       = 0,
		SPOT        = 1,
		DIRECTIONAL = 2
	};

	struct LightInfo {
		alignas(16) glm::vec3 position{ 0.f }; // for all
		alignas(16) glm::vec4 emission{ 1.f }; // for all
		alignas(16) glm::vec3 direction{ 0.f, 1.f, 0.f }; // for spot and directional
		alignas(16) glm::mat4 projViewMat{ 1.f }; // for spot and directional
		int type{ 0 };
		int shadowmap{ -1 };
		float contactShadow{ -1.f };
		float radius{ 10.f }; // for point and spot
		float outerAngle{ 60.f }; // for spot
		float innerAngleRatio{ 0.5f }; // for spot
	}lightInfo;

	static std::shared_ptr<Image2D> normalShadowmaps;
	static std::shared_ptr<Image2D> omniShadowmaps;
	static void copyShadowmap(
		Device& device,
		VkCommandBuffer commandBuffer,
		FrameBufferAttachment& attachment,
		uint32_t no,
		int cubeFace = -1);
	float importance{1.f};

	Light(
		Device& device,
		std::string name,
		Type type);
	Light() = delete;
	~Light();
	Type type() const { return _lightType; }
	virtual void setId(ResId id);
	void setObjId(ResId id) { Object::setId(id); }
	ResId id() const { return Resource::_id; }
	ResId objId() const { return Object::_id; }
	std::string name() const { return Resource::_name; }

	virtual void update();
	void updateProjection();

	void setDirection(const glm::vec3& direction);
	void setDirection(float directionX, float directionY, float directionZ) {
		setDirection(glm::vec3{ directionX, directionY, directionZ });
	}

	void setEmission(const glm::vec4& emission) {
		lightInfo.emission = emission;
	}
	void setEmission(float emissionX, float emissionY, float emissionZ, float emissionW) {
		setEmission(glm::vec4{ emissionX, emissionY, emissionZ, emissionW });
	}
	void setOuterAngle(float angle) {
		lightInfo.outerAngle = angle;
		updateProjection();
	}
	void setRadius(float radius) {
		lightInfo.radius = radius;
		updateProjection();
	}


	//void createShadowmap();
	//bool hasShadowmap{ false };
	//std::unique_ptr<Shadowmap> shadowmap;

	Projector projector;
	std::unique_ptr<glm::mat4> cubeProjector0;
	std::unique_ptr<glm::mat4> cubeProjector1;
	std::unique_ptr<glm::mat4> cubeProjector2;
	std::unique_ptr<glm::mat4> cubeProjector3;
	std::unique_ptr<glm::mat4> cubeProjector4;
	std::unique_ptr<glm::mat4> cubeProjector5;

	friend class GUI;
	friend class Scene;

private:
	Type _lightType;

	static size_t _instanceCount;
};

}

#endif