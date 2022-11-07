#ifndef ENGINE_HPP
#define ENGINE_HPP

#include "naku.hpp"
#include "utils/descriptors.hpp"
#include "utils/device.hpp"
#include "resources/resource.hpp"
#include "resources/object.hpp"
#include "resources/camera.hpp"
#include "resources/light.hpp"

namespace naku {

struct GlobalUbo { // ubo1
	float time{ 0.f };
	float tanFov{ 0.f };
	int height;
	int width;
	alignas(16) glm::mat4 projView{ 1.f };
	alignas(16) glm::mat4 projInv{ 1.f };
	alignas(16) glm::mat4 viewInv{ 1.f };
	alignas(16) glm::vec4 viewPort{ 0.f }; // x, y, w, h
	alignas(16) glm::vec4 clip{ 0.f }; // min depth, max depth, near clip, far clip
	alignas(16) glm::vec4 camPos{ 1.f };
	alignas(16) glm::vec3 camDir{ 1.f };
	alignas(16) glm::vec3 clearColor{ 0.192f, 0.302f, 0.476f };
	alignas(16) glm::vec4 environment{ 1.0f, 1.0f, 1.0f, 0.1f };
};

struct LightUbo {
	int lightNum{ 0 };
	alignas(16) Light::LightInfo infos[MAX_LIGHT_NUM];
};

extern class GUI;
extern class Renderer;

class Engine {
// in order to ease the control of the engine, this class contains no privates
public:
	static struct Garbage {
		std::shared_ptr<void> ptr;
		uint32_t frameIdx;
		uint32_t imageIdx;
	};
	static struct LateUpdate {
		DescriptorWriter& writer;
		VkDescriptorSet& set;
		uint32_t frameIdx;
		uint32_t imageIdx;
	};

	std::list<Garbage> garbages;
	std::list<LateUpdate> updateQueue;
	
	template<class T>
	inline void addGarbage(const std::shared_ptr<T>& ptr) {
		garbages.push_back({ std::move(ptr), frameIdx, imageIdx });
	}

	inline void addLateUpdate(DescriptorWriter& writer, VkDescriptorSet& set) {
		updateQueue.push_back({ writer, set, frameIdx, imageIdx });
	}

	uint32_t frameIdx, imageIdx;

	Engine(uint32_t w, uint32_t h, std::string w_name, float dpi=1.f);
	~Engine();
	Engine(const Engine&) = delete;
	void operator=(const Engine&) = delete;

	VkDevice device() const { return pDevice->device(); }

	// resources
		std::shared_ptr<Image2D> createImage(const std::string& name, const std::string& filePath);
		std::shared_ptr<Image2D> createImage(const std::string& filePath);
		std::shared_ptr<Object> createObject(const std::string& name, Object::Type type);
		std::shared_ptr<Light> createLight(const std::string& name, Light::Type type);
		std::shared_ptr<Camera> createCamera(const std::string& name);
		std::shared_ptr<Camera> createCamera(const std::string& name, float left, float right, float top, float bottom, float near, float far);
		std::shared_ptr<Camera> createCamera(const std::string& name, float fov_y, float near, float far);
		uint32_t pointOffset{ 0 }, spotOffset{ 0 }, directionalOffset{ 0 };
		std::shared_ptr<Model> createModel(const std::string& name, const Mesh & Mesh);
		std::shared_ptr<Model> createModel(const std::string& name, const std::string objFilePath);
		std::shared_ptr<Shader> createShader(
			const std::string& name,
			const std::string & filePath,
			VkShaderStageFlagBits stage);
		std::shared_ptr<Shader> createShader(
			const std::string & filePath,
			VkShaderStageFlagBits stage);
		std::shared_ptr<Material> createMaterial(
			const std::string& name,
			Material::Type type,
			std::shared_ptr<Shader> pVertShader,
			std::shared_ptr<Shader> pFragShader);

		bool changeMaterial(ResId objId, ResId mtlId);
		void changeMaterial(std::shared_ptr<Object> object, std::shared_ptr<Material> material);

	int run();
	void prepareUbos();
	void prepareDescriptorPool();
	void prepareResources();
	void arrangeGlobal(Renderer& renderer);
	void arrangeLights();
	void arrangeTransparents();
	void handleKeyBoardInput();
	void setupGUI(GUI& guiSystem);

	std::unique_ptr<Window> pWindow;
	std::unique_ptr<Device> pDevice;

	std::shared_ptr<Camera> pMainCamera;
	glm::vec3 globalUp;

	// resources
	ResourceManager resources;

	// ubos and descriptors
	std::shared_ptr<DescriptorPool> pDescriptorSetPool;
	std::shared_ptr<DescriptorSetLayout> pGlobalSetLayout;

	GlobalUbo globalUbo{};
	LightUbo lightUbo{};

	std::map<float, std::shared_ptr<Light>> lightMap;
	std::set<ResId> transparents;
	std::map<float, std::shared_ptr<Object>> transparentMap;
	std::vector<VkDescriptorSet> globalSets;

	std::vector<std::unique_ptr<Buffer>> globalUboBuffers;
	std::vector<std::unique_ptr<Buffer>> lightUboBuffers;

	float deltaTime{ 0.f };
	float runningTime{ 0.f };
	float alpha{ 1.0f };
	float gamma{ 1.0f };
	int presentAttachment{ 0 };

	bool showGUI{ true };
	
};

}

#endif