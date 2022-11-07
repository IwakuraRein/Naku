#include "utils/engine.hpp"

#include "io/gui.hpp"
#include "io/fps_controller.hpp"
#include "render_systems/renderer.hpp"
#include "render_systems/opaque_renderer.hpp"
#include "render_systems/gbuffer_renderer.hpp"
#include "render_systems/present_renderer.hpp"
#include "render_systems/transparent_renderer.hpp"
#include "render_systems/shadowmap_renderer.hpp"

#include <vector>
#include <chrono>
#include <ctime>
#include <exception>
#include <iostream>

namespace naku {

Engine::Engine(uint32_t w, uint32_t h, std::string w_name, float dpi) {
	pWindow = std::make_unique<Window>(w_name, w, h, dpi);
	pDevice = std::make_unique<Device>(*pWindow);
	prepareResources();
	prepareDescriptorPool();
	prepareUbos();
}

Engine::~Engine() {}

void Engine::prepareResources() {
	resources.addResource<Image2D>();
	resources.addResource<Model>();
	resources.addResource<Shader>();
	resources.addResource<Material>();
	resources.addResource<Object>();
	resources.addResource<Camera>();
	resources.addResource<Light>();

	resources.createCollect<Shader, Material>();
	resources.createCollect<Material, Object>();
}

void Engine::prepareUbos() {
	 auto layoutBuilder =
		DescriptorSetLayout::Builder(*pDevice)
		.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS) // global info
		.addBinding(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_ALL_GRAPHICS) // model info
		.addBinding(2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS); // light info
	pGlobalSetLayout = layoutBuilder.build();
	globalSets.resize(MAX_FRAMES_IN_FLIGHT);
	globalUboBuffers.resize(MAX_FRAMES_IN_FLIGHT);
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		globalUboBuffers[i] = std::make_unique<Buffer>(
			*pDevice,
			sizeof(GlobalUbo),
			1,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		//globalUboBuffers[i]->map();
	}
	
	Object::prepareObjectUbo(*pDevice);

	lightUboBuffers.resize(MAX_FRAMES_IN_FLIGHT);
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		lightUboBuffers[i] = std::make_unique<Buffer>(
			*pDevice,
			sizeof(LightUbo),
			1,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		//globalUboBuffers[i]->map();
	}

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		auto gUboInfo = globalUboBuffers[i]->descriptorInfo();
		auto modelUboInfo = Object::modelUboBuffers[i]->descriptorInfo(Object::dynamicAlignment, 0);
		auto lightUboInfo = lightUboBuffers[i]->descriptorInfo();
		DescriptorWriter(*pGlobalSetLayout, *pDescriptorSetPool)
			.writeBuffer(0, gUboInfo)
			.writeBuffer(1, modelUboInfo)
			.writeBuffer(2, lightUboInfo)
			.build(globalSets[i]);
	}
}

void Engine::handleKeyBoardInput() {
	static bool insertKeyPressed{ false };
	if (glfwGetKey(pWindow->pWindow(), GLFW_KEY_INSERT) == GLFW_PRESS) insertKeyPressed = true;

	if (insertKeyPressed && glfwGetKey(pWindow->pWindow(), GLFW_KEY_INSERT) == GLFW_RELEASE) {
		showGUI = !showGUI;
		insertKeyPressed = false;
	}
}

void Engine::prepareDescriptorPool()
{
	std::vector<VkDescriptorPoolSize> pool_sizes
	{
		{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
	};

	pDescriptorSetPool = std::make_unique<DescriptorPool>(
		*pDevice,
		1000,
		VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
		pool_sizes
		);
}

void Engine::setupGUI(GUI& guiSystem) {
	guiSystem.beginFrame();
	guiSystem.beginLeftColumn();
		guiSystem.showObjectSelect();
	guiSystem.endLeftColumn();
	guiSystem.beginRightColumn();
		guiSystem.showMonitor();
		guiSystem.showGraphicsControl();
		guiSystem.showInspector();
	guiSystem.endRightColumn();
	guiSystem.beginBottomBox();
		guiSystem.showResourceSelect();
	guiSystem.endBottomBox();
#ifndef NDEBUG
	guiSystem.showDemoWindow();
#endif
	guiSystem.endFrame();
}

void Engine::arrangeLights() {
	auto& Lights = resources.getResource<Light>();
	lightUbo.lightNum = 0;

	//TODO: avoid clearing map every frame
	lightMap.clear();
	for (ResId id = 0; id < Lights.size(); id++) {
		if (!Lights.exist(id)) continue;
		auto light = Lights[id];
		lightMap.emplace(light->importance, light);
	}
	for (auto& pair : lightMap) {
		auto light = pair.second;
		if (light->isActive())
			memcpy(&lightUbo.infos[lightUbo.lightNum++], &light->lightInfo, sizeof(Light::LightInfo));
	}
}

void Engine::arrangeTransparents() {
	//TODO: avoid clearing map every frame
	transparentMap.clear();
	auto& Objects = resources.getResource<Object>();
	for (const ResId& objId : transparents) {
		if (!Objects.exist(objId)) continue;
		auto obj = Objects[objId];
		const float dist = glm::length(obj->position() - pMainCamera->position());
		transparentMap.emplace(dist, obj);
	}
}
void Engine::arrangeGlobal(Renderer& renderer) {
	if (pWindow->viewPortWidth() > 0 && pWindow->viewPortHeight() > 0)
		pMainCamera->setAspect(renderer.getAspectRatio());
	globalUbo.height = static_cast<int>(pWindow->height());
	globalUbo.width = static_cast<int>(pWindow->width());
	globalUbo.projView = pMainCamera->projMat() * pMainCamera->viewMat();
	globalUbo.projInv = glm::inverse(pMainCamera->projMat());
	globalUbo.viewInv = glm::inverse(pMainCamera->viewMat());
	globalUbo.camPos = glm::vec4(pMainCamera->position(), 1.0);
	globalUbo.tanFov = pMainCamera->tanFov();
	if (showGUI)
		globalUbo.viewPort = { pWindow->viewPortCoord(), pWindow->viewPortExtent().width, pWindow->viewPortExtent().height };
	else
		globalUbo.viewPort = { 0.f, 0.f, static_cast<float>(renderer.getSwapChainExtent().width), static_cast<float>(renderer.getSwapChainExtent().height) };
	globalUbo.clip = { MIN_DEPTH, MAX_DEPTH, pMainCamera->near(), pMainCamera->far() };
	globalUbo.time = runningTime;
	globalUbo.camDir = pMainCamera->forwardDir();
}

int Engine::run() {
	FPSController fpsController{ *pWindow };
	Renderer renderer{ *this };
	GUI guiSystem{ *this, renderer, *renderer.postPass, 0 };

	// lock the up direction when looking around
	globalUp = pMainCamera->upDir();
	// update and write all transforms
	Object::changedObjects.clear();
	for (ResId id = 0; id < resources.getResource<Object>().size(); id++) {
		auto obj = resources.get<Object>(id);
		obj->update();
		obj->writeToObjectBuffer();
	}

	//renderer.createRenderer(renderer.gbufferRenderer, *renderer.gbufferPass, 0);
	//renderer.createRenderer(renderer.presentRenderer, *renderer.gbufferPass, 1);
	//renderer.createRenderers();

	// start rendering
	std::cout << "Start rendering..." << std::endl;
	std::chrono::steady_clock::time_point t_start, t_end, r_start;
	r_start = std::chrono::high_resolution_clock::now();
	while (
		!pWindow->shouldClose() &&
		!(glfwGetKey(pWindow->pWindow(), GLFW_KEY_ESCAPE) == GLFW_PRESS)) {
		static bool firstLaunch{ true };

		t_start = std::chrono::high_resolution_clock::now();
		runningTime = std::chrono::duration<float>(std::chrono::high_resolution_clock::now() - r_start).count();

		glfwPollEvents();

		if (auto commandBuffer = renderer.beginFrame()) {
			frameIdx = renderer.getFrameIndex();
			imageIdx = renderer.getImageIndex();

			// handleUpdate()
			for (auto itr = updateQueue.begin(); itr != updateQueue.end();) {
				if (renderer.getFenceStatus(itr->imageIdx) == VK_SUCCESS) {
					itr->writer.overwrite(itr->set);
					updateQueue.erase(itr++);
				}
				else itr++;
			}
			// handleGC()
			for (auto itr = garbages.begin(); itr != garbages.end();) {
				if (renderer.getFenceStatus(itr->imageIdx) == VK_SUCCESS) {
					garbages.erase(itr++);
				}
				else itr++;
			}

			setupGUI(guiSystem);

			{ //update ubo
				//update transform. only iterate changed objects
				for (auto pair = Object::changedObjects.begin(); pair != Object::changedObjects.end();) {
					auto& id = pair->first;
					auto obj = resources.get<Object>(id);
					obj->writeToObjectBuffer(frameIdx);
					if (Object::changedObjects[id] <= 0) {
						Object::changedObjects.erase(pair++);
					}
					else pair++;
				}
				arrangeGlobal(renderer);
				arrangeLights();
				arrangeTransparents();
				globalUboBuffers[frameIdx]->writeToBuffer(&globalUbo);
				lightUboBuffers[frameIdx]->writeToBuffer(&lightUbo);
			}

			FrameInfo frameInfo{
				frameIdx,
				imageIdx,
				runningTime,
				deltaTime,
				commandBuffer,
				{globalSets[frameIdx]},
				*pMainCamera
			};

			{
				uint32_t count1{ 0 }, count2{ 0 };
				for (auto& pair : lightMap) {
					auto light = pair.second;
					if (light->lightInfo.shadowmap > 0 && light->isActive()) {
						if (light->type() == Light::Type::POINT) {
							for (int i = 0; i < 6; i++) {
								renderer.beginShadowPass(commandBuffer, true);
								renderer.shadowmapRenderer->render(frameInfo, *light, i);
								renderer.endRenderPass(commandBuffer);
								Light::copyShadowmap(*pDevice, frameInfo.commandBuffer, renderer.shadowPass->attachments[0], count1, i);
							}
							count1++;
						}
						else {
							renderer.beginShadowPass(commandBuffer);
							renderer.shadowmapRenderer->render(frameInfo, *light);
							renderer.endRenderPass(commandBuffer);
							Light::copyShadowmap(*pDevice, frameInfo.commandBuffer, renderer.shadowPass->attachments[0], count2++);
						}
					}
				}
			}

			renderer.beginRenderPass(commandBuffer, *renderer.gbufferPass, !showGUI, false);
			renderer.gbufferRenderer->render(frameInfo);
			renderer.endRenderPass(commandBuffer);
			renderer.beginRenderPass(commandBuffer, *renderer.opaquePass, !showGUI, false);
			renderer.opaqueRenderer->render(frameInfo);
			renderer.endRenderPass(commandBuffer);
			renderer.beginRenderPass(commandBuffer, *renderer.transparentPass, !showGUI, false);
			for (auto it = transparentMap.end(); it != transparentMap.begin();) {
				it--;
				renderer.transparentRenderer->render(frameInfo, *it->second);
			}
			renderer.endRenderPass(commandBuffer);
			renderer.beginRenderPass(commandBuffer, *renderer.postPass, !showGUI, false);
			renderer.presentRenderer->render(frameInfo, alpha, gamma, presentAttachment);

			if (showGUI) guiSystem.render(commandBuffer);

			renderer.endRenderPass(commandBuffer);
			renderer.endFrame();
		}

		t_end = std::chrono::high_resolution_clock::now();
		deltaTime = std::chrono::duration<float>(t_end - t_start).count();

		handleKeyBoardInput();
		fpsController.handleKeyBoardInput(pMainCamera.get(), deltaTime);
		fpsController.handleMouseInput(pMainCamera.get(), globalUp);

		firstLaunch = false;

	}

	vkDeviceWaitIdle(device());

	return EXIT_SUCCESS;
}

}

namespace naku { //resources

std::shared_ptr<Image2D> Engine::createImage(const std::string& filePath) {
	auto name = getFileName(filePath);
	return createImage(name, filePath);
}

std::shared_ptr<Image2D> Engine::createImage(const std::string& name, const std::string& filePath) {
	if (resources.exist<Image2D>(name)) {
		std::cerr << "Warning: Image " << name << " already exists." << std::endl;
		return resources.get<Image2D>(name);
	}
	try {
		auto p = Image2D::loadImageFromFile(*pDevice, name, filePath);
		ResId id = resources.push<Image2D>(name, p);
		p->setId(id);
		return resources.get<Image2D>(id);
	}
	catch (const std::exception& e) {
		std::cerr << "Error: Cannot load Image " << filePath << std::endl;
		return nullptr;
	}
}

std::shared_ptr<Shader> Engine::createShader(
	const std::string& filePath,
	VkShaderStageFlagBits stage) {
	auto name = getFileName(filePath);
	return createShader(name, filePath, stage);
}

std::shared_ptr<Shader> Engine::createShader(
	const std::string& name,
	const std::string& filePath,
	VkShaderStageFlagBits stage) {
	if (resources.exist<Shader>(name)) {
		std::cerr << "Warning: Shader " << name << " already exists." << std::endl;
		return resources.get<Shader>(name);
	}
	auto pShader = std::make_shared<Shader>(*pDevice, name, filePath, stage);
	ResId id = resources.push<Shader>(name, pShader);
	pShader->setId(id);
	return pShader;
}

std::shared_ptr<Material> Engine::createMaterial(
	const std::string& name,
	Material::Type type,
	std::shared_ptr<Shader> pVertShader,
	std::shared_ptr<Shader> pFragShader) {
	if (resources.exist<Material>(name)) {
		std::cerr << "Warning: Material " << name << " already exists." << std::endl;
		return resources.get<Material>(name);
	}
	auto p = std::make_shared<Material>(type, *pDevice, name, pVertShader, pFragShader, *pDescriptorSetPool);
	ResId id = resources.push<Material>(name, p);
	p->setId(id);
	resources.addCollect<Shader, Material>(pVertShader->id(), id);
	resources.addCollect<Shader, Material>(pFragShader->id(), id);
	return p;
}

std::shared_ptr<Object> Engine::createObject(const std::string& name, Object::Type type) {
	if (resources.exist<Object>(name)) {
		std::cerr << "Warning: Object " << name << " already exists." << std::endl;
		return resources.get<Object>(name);
	}
	if (resources.size<Object>() >= MAX_OBJECT_NUM) {
		throw std::runtime_error("Error: Failed to create object. Objects' number reach the limit.");
	}
	auto pObject = std::make_shared<Object>(*pDevice, name, type);
	ResId id = resources.push<Object>(name, pObject);
	pObject->setId(id);
	pObject->writeToObjectBuffer();
	return pObject;
}

std::shared_ptr<Camera> Engine::createCamera(const std::string& name) {
	auto ptr = std::make_shared<Camera>(*pDevice, name);
	ResId id = resources.push<Object>(name, ptr);
	ptr->setObjId(id);
	ptr->writeToObjectBuffer();
	ResId camId = resources.push<Camera>(name, ptr);
	ptr->setId(camId);
	ptr->writeToObjectBuffer();
	return ptr;
}
std::shared_ptr<Camera> Engine::createCamera(const std::string& name, float left, float right, float top, float bottom, float near, float far) {
	auto ptr = std::make_shared<Camera>(*pDevice, name);
	ResId id = resources.push<Object>(name, ptr);
	ptr->setObjId(id);
	ptr->writeToObjectBuffer();
	ResId camId = resources.push<Camera>(name, ptr);
	ptr->setId(camId);

	ptr->setOrthoProjection(left, right, top, bottom, near, far);
	ptr->writeToObjectBuffer();
	return ptr;
}
std::shared_ptr<Camera> Engine::createCamera(const std::string& name, float fov_y, float near, float far) {
	auto ptr = std::make_shared<Camera>(*pDevice, name);
	ResId id = resources.push<Object>(name, ptr);
	ptr->setObjId(id);
	ptr->writeToObjectBuffer();
	ResId camId = resources.push<Camera>(name, ptr);
	ptr->setId(camId);

	const float aspect = static_cast<float>(pWindow->viewPortWidth()) / static_cast<float>(pWindow->viewPortHeight());
	ptr->setPerspProjection(fov_y, aspect, near, far);
	ptr->writeToObjectBuffer();
	return ptr;
}

std::shared_ptr<Light> Engine::createLight(const std::string& name, Light::Type type) {
	if (resources.exist<Light>(name)) {
		std::cerr << "Warning: Light. " << name << " already exists." << std::endl;
		return resources.get<Light>(name);
	}
	if (resources.size<Object>() >= MAX_OBJECT_NUM) {
		throw std::runtime_error("Error: Failed to create object. Objects' number reach the limit.");
	}
	if (resources.size<Light>() >= MAX_LIGHT_NUM) {
		throw std::runtime_error("Error: Failed to create light. Lights' number reach the limit.");
	}
	std::shared_ptr<Light> pLight;
	pLight = std::make_shared<Light>(*pDevice, name, type);
	//lightUbo.lightNum++;

	ResId objId = resources.push<Object>(name, pLight);
	pLight->setObjId(objId);
	ResId lightId = resources.push<Light>(name, pLight);
	pLight->setId(lightId);
	pLight->writeToObjectBuffer();
	return pLight;
}

std::shared_ptr<Model> Engine::createModel(const std::string& name, const Mesh& Mesh) {
	if (resources.exist<Model>(name)) {
		std::cerr << "Warning: Model " << name << " already exists." << std::endl;
		return resources.get<Model>(name);
	}
	auto pModel = std::make_shared<Model>(*pDevice, name, Mesh);
	ResId id = resources.push<Model>(name, pModel);
	pModel->setId(id);
	return pModel;
}

std::shared_ptr<Model> Engine::createModel(const std::string& name, const std::string objFilePath) {
	if (resources.exist<Model>(name)) {
		std::cerr << "Warning: Model " << name << " already exists." << std::endl;
		return resources.get<Model>(name);
	}
	auto pModel = std::make_shared<naku::Model>(*pDevice, name, objFilePath);
	ResId id = resources.push<Model>(name, pModel);
	pModel->setId(id);
	return pModel;
}

bool Engine::changeMaterial(ResId objId, ResId mtlId) {
	if (!resources.exist<Material>(mtlId)) {
		std::cerr << "Error: Failed to change material. Material doesn't exist." << std::endl;
		return false;
	}
	auto pObj = resources.get<Object>(objId);
	auto pMtl = resources.get<Material>(mtlId);

	changeMaterial(pObj, pMtl);
	return true;
}

void Engine::changeMaterial(std::shared_ptr<Object> object, std::shared_ptr<Material> material) {
	auto origMtl = object->material;
	object->material = material;
	resources.removeFromCollect<Material, Object>(origMtl->id(), object->id());
	resources.addCollect<Material, Object>(material->id(), object->id());
}

}