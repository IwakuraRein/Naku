#ifndef SCENE_HPP
#define SCENE_HPP

#include "utils/engine.hpp"

#include <string>
#include <unordered_map>

namespace naku {

class Scene {
public:
	Scene(Engine& engine, std::string filePath);
	~Scene() {}
	Scene(const Scene&) = delete;
	void operator=(const Scene&) = delete;

	void load();
	void save(bool forceWrite = true);

	uint32_t modelCount() const { return _modelCount; }
	uint32_t objectCount() const { return _objectCount; }
	uint32_t vertexCount() const { return _vertCount; }
	uint32_t faceCount() const { return _faceCount; }

	void clear();

	std::string filePath;
	bool echo{ false };
	std::unordered_map<std::string, std::shared_ptr<Model>> InternalMeshs;

private:
	Engine& _engine;
	std::shared_ptr<Shader> _opaqueVert;
	std::shared_ptr<Shader> _opaqueFrag;
	std::shared_ptr<Shader> _transparentVert;
	std::shared_ptr<Shader> _transparentFrag;
	nlohmann::json _j;
	uint32_t _modelCount{ 0 }, _objectCount{ 0 }, _vertCount{ 0 }, _faceCount{ 0 };


	void loadGraphics();
	void loadCamera();
	void loadObjects();
	void loadLights();
	void loadMaterials();
};

}

#endif