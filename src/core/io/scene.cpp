#include "io/scene.hpp"

#include <iostream>

namespace naku {

Scene::Scene(Engine& engine, std::string path)
	: _engine{ engine }, filePath{ path } {

	_opaqueVert = _engine.createShader("res/shader/opaque.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
	_opaqueFrag = _engine.createShader("res/shader/opaque.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
	//TODO
	_transparentVert = _engine.createShader("res/shader/transparent.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
	_transparentFrag = _engine.createShader("res/shader/transparent.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
	InternalMeshs.emplace(std::string("quad"), _engine.createModel("quad", "res/model/quad.obj"));
	InternalMeshs.emplace(std::string("teapot"), _engine.createModel("teapot", "res/model/teapot.obj"));
	InternalMeshs.emplace(std::string("sphere"), _engine.createModel("sphere", "res/model/sphere.obj"));
	InternalMeshs.emplace(std::string("cube"), _engine.createModel("cube", "res/model/cube.obj"));
}

void Scene::load() {
	clear();
	_j = readJson(filePath+"/scene.json");

	loadGraphics();
	loadCamera();
	loadMaterials(); //must load materials before objects
	loadObjects();
	loadLights();

}

void Scene::loadGraphics() {
	for (auto& item : _j["graphics"].items()) {
		if (item.key() == "environment") {
			_engine.globalUbo.environment = glm::vec4(item.value()[0], item.value()[1], item.value()[2], item.value()[3]);
		}
		if (item.key() == "alpha") {
			_engine.alpha = item.value();
		}
		if (item.key() == "gamma") {
			_engine.gamma = item.value();
		}
	}
}
void Scene::loadCamera() {
	int count{ 0 };
	for (auto& item : _j["cameras"].items()) {
		auto pCam = _engine.createCamera(item.key());
		auto& values = item.value();
		if (MapHas(values, "active"))
			pCam->_active = values["active"];
		if (MapHas(values, "position")) {
			pCam->setPosition(values["position"][0], values["position"][1], values["position"][2]);
		}
		if (MapHas(values, "rotation")) {
			pCam->setRotation(values["rotation"][0], values["rotation"][1], values["rotation"][2]);
		}
		if (MapHas(values, "fov")) {
			pCam->setFov(values["fov"]);
		}
		if (MapHas(values, "forward")) {
			glm::vec3 up{ 0.f, -1.f, 0.f };
			glm::vec3 forward{ values["forward"][0], values["forward"][1], values["forward"][2] };
			if (MapHas(values, "up") != 0)
				up = glm::vec3{ values["up"][0], values["up"][1], values["up"][2] };
			pCam->orientate(forward, up);
		}
		else {
			if (MapHas(values, "look_at")) {
				glm::vec3 up{ 0.f, -1.f, 0.f };
				glm::vec3 lookAt{ values["look_at"][0], values["look_at"][1], values["look_at"][2] };
				if (MapHas(values, "up") != 0)
					up = glm::vec3{ values["up"][0], values["up"][1], values["up"][2] };
				pCam->lookAt(lookAt, up);
			}
		}
		if (echo) {
			auto pos = pCam->position();
			auto rotation = pCam->rotation();
			std::cout << "\tCamera " << pCam->name() << " loaded. position: "
				<< " (" << pos.x << " " << pos.y << " " << pos.z << "). rotation: "
				<< " (" << rotation.x << " " << rotation.y << " " << rotation.z << ")" << std::endl;
		}
		if (count == 0) _engine.pMainCamera = pCam;
		count++;
	}
	if (count == 0) { //create a default camera
		_engine.pMainCamera = _engine.createCamera("MainCamera");
	}
}

void Scene::loadObjects() {
	for (auto& item : _j["objects"].items()) {
		std::string name = item.key();
		//std::string type = item.value()["type"];
		auto& values = item.value();
		auto pObject = _engine.createObject(name, Object::Type::MESH);
		ResId id = pObject->id();

		if (MapHas(values, "active"))
			pObject->_active = values["active"];
		if (MapHas(values, "position"))
			pObject->setPosition(values["position"][0], values["position"][1], values["position"][2]);
		if (MapHas(values, "rotation"))
			pObject->setRotation(values["rotation"][0], values["rotation"][1], values["rotation"][2]);
		if (MapHas(values, "scale"))
			pObject->setScale(values["scale"][0], values["scale"][1], values["scale"][2]);
		if (MapHas(values, "mesh")) {
			std::string modelPath = values["mesh"];
			std::string modelName = getFileName(modelPath);

			std::shared_ptr<Model> pModel;
			if (!MapHas(InternalMeshs, modelName)) {
				if (!doesFileExist(modelPath)) {
					if (doesFileExist(filePath + "/" + modelPath))
						modelPath = filePath + "/" + modelPath;
					else
						throw std::runtime_error(std::string("Error: Mesh ") + modelName + " does not exist.");
				}
				pModel = _engine.createModel(modelName, modelPath);
			}
			else
				pModel = InternalMeshs[modelName];
			pObject->model = pModel;
			_vertCount += pModel->vertexCount();
			_faceCount += pModel->indexCount() / 3;
			_modelCount += 1;
		}
		if (MapHas(values, "material")) {
			std::string mtlName = values["material"];
			if (!_engine.resources.exist<Material>(mtlName)) {
				mtlName = "Error: material " + mtlName;
				mtlName = mtlName + " isn't loaded.";
				throw std::runtime_error(mtlName);
			}
			auto pMaterial = _engine.resources.get<Material>(mtlName);
			pObject->material = pMaterial;
			_engine.resources.addCollect<Material, Object>(pMaterial->id(), pObject->id());
			if (pMaterial->type() == Material::Type::TRANSPARENT)
				_engine.transparents.insert(pObject->id());
		}
		_objectCount += 1;
		if (echo) {
			auto pos = pObject->position();
			auto scale = pObject->scale();
			auto rotation = pObject->rotation();
			std::cout << "\tObject " << name << " loaded. position: ("
				<< pos.x << " " << pos.y << " " << pos.z << "). scale: ("
				<< scale.x << " " << scale.y << " " << scale.z << "). rotation: "
				<< rotation.x << " " << rotation.y << " " << rotation.z << ")" << std::endl;
		}
	}
}

void Scene::loadMaterials() {
	for (auto& mtl : _j["materials"].items()) {
		std::string name = mtl.key();
		auto& Value = mtl.value();

		std::shared_ptr<Material> pMaterial;
		if (MapHas(Value, "type")) {
			if (Value["type"] == "Opaque")
				pMaterial = _engine.createMaterial(name, Material::Type::OPAQUE, _opaqueVert, _opaqueFrag);
			else if (Value["type"] == "Transparent")
				pMaterial = _engine.createMaterial(name, Material::Type::TRANSPARENT, _transparentVert, _transparentFrag);
			else {
				std::cerr << "Warning: Unknown material type: " << Value["type"] << std::endl;
				pMaterial = _engine.createMaterial(name, Material::Type::OPAQUE, _opaqueVert, _opaqueFrag);
			}
		}
		else {
			std::cerr << "Warning: Material type unspecified." << std::endl;
			pMaterial = _engine.createMaterial(name, Material::Type::OPAQUE, _opaqueVert, _opaqueFrag);
		}
		if (MapHas(Value, "offset")) {
			pMaterial->pushConstants.offsetTilling.x = Value["offset"][0];
			pMaterial->pushConstants.offsetTilling.y = Value["offset"][1];
		}
		if (MapHas(Value, "tilling")) {
			pMaterial->pushConstants.offsetTilling.z = Value["tilling"][0];
			pMaterial->pushConstants.offsetTilling.w = Value["tilling"][1];
		}
		if (MapHas(Value, "albedo")) {
			pMaterial->pushConstants.albedo.x = Value["albedo"][0];
			pMaterial->pushConstants.albedo.y = Value["albedo"][1];
			pMaterial->pushConstants.albedo.z = Value["albedo"][2];
			if (Value["type"] == "Transparent")
				pMaterial->pushConstants.albedo.z = Value["albedo"][3];
		}
		if (MapHas(Value, "emission")) {
			pMaterial->pushConstants.emission.x = Value["emission"][0];
			pMaterial->pushConstants.emission.y = Value["emission"][1];
			pMaterial->pushConstants.emission.z = Value["emission"][2];
			pMaterial->pushConstants.emission.w = Value["emission"][3];
		}
		if (MapHas(Value, "baseTex")) {
			std::string path = Value["baseTex"];
			if (!doesFileExist(path)) {
				if (doesFileExist(filePath + "/" + path))
					path = filePath + "/" + path;
				else
					throw std::runtime_error(std::string("Error: ") + path + " does not exist.");
			}
			auto pImage = _engine.createImage(path);
			auto pTex = std::make_shared<Texture>(
				*_engine.pDevice,
				"base",
				pImage,
				true);
			pMaterial->changeTexture(0, pTex);
		}
		if (MapHas(Value, "normalTex")) {
			std::string path = Value["normalTex"];
			if (!doesFileExist(path)) {
				if (doesFileExist(filePath + "/" + path))
					path = filePath + "/" + path;
				else
					throw std::runtime_error(std::string("Error: ") + path + " does not exist.");
			}
			auto pImage = _engine.createImage(path);
			auto pTex = std::make_shared<Texture>(
				*_engine.pDevice,
				"base",
				pImage,
				false);
			pMaterial->changeTexture(1, pTex);
		}
		if (echo) {
			std::cout << "\tmaterial " << name << " loaded." << std::endl;
		}
	}
}

void Scene::loadLights() {
	float importance = MAX_LIGHT_NUM;
	for (auto& item : _j["lights"].items()) {
		std::string name = item.key();
		std::string type = item.value()["type"];
		auto& values = item.value();
		std::shared_ptr<Light> pLight;
		if (type == "point") {
			pLight = _engine.createLight(name, Light::Type::POINT);
		}
		if (type == "spot") {
			pLight = _engine.createLight(name, Light::Type::SPOT);
		}
		if (type == "directional") {
			pLight = _engine.createLight(name, Light::Type::DIRECTIONAL);
		}
		if (MapHas(values, "shadowmap")) {
			if (values["shadowmap"]) {
				pLight->lightInfo.shadowmap = 1;
			}
			else pLight->lightInfo.shadowmap = -1;
		}
		if (MapHas(values, "contact shadow")) {
			pLight->lightInfo.contactShadow = values["contact shadow"];
		}
		if (MapHas(values, "importance")) {
			pLight->importance = values["importance"];
		}
		else pLight->importance = importance--;
		if (MapHas(values, "active"))
			pLight->_active = values["active"];
		if (MapHas(values, "radius")) {
			pLight->setRadius(values["radius"]);
		}
		if (MapHas(values, "outer angle")) {
			pLight->setOuterAngle(values["outer angle"]);
			if (MapHas(values, "inner angle"))
				pLight->lightInfo.innerAngleRatio = glm::clamp((float)values["inner angle"] / (float)values["outer angle"], 0.f, 1.f);
		}
		if (MapHas(values, "inner angle ratio"))
			pLight->lightInfo.innerAngleRatio = values["innerAngleRatio"];
		if (MapHas(values, "position"))
			pLight->setPosition(values["position"][0], values["position"][1], values["position"][2]);
		if (MapHas(values, "rotation"))
			pLight->setRotation(values["rotation"][0], values["rotation"][1], values["rotation"][2]);
		if (MapHas(values, "direction"))
			pLight->setDirection(values["direction"][0], values["direction"][1], values["direction"][2]);
		if (MapHas(values, "emission"))
			pLight->setEmission(values["emission"][0], values["emission"][1], values["emission"][2], values["emission"][3]);
		if (echo) {
			auto pos = pLight->position();
			std::cout << "\tLight " << name << " loaded. position: ("
				<< pos.x << " " << pos.y << " " << pos.z << ")." << std::endl;
		}
	}
}

void Scene::clear() {
	//TODO
}

}