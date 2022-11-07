#ifndef GUI_HPP
#define GUI_HPP

#include "naku.hpp"
#include "utils/descriptors.hpp"
#include "render_systems/renderer.hpp"
#include "utils/engine.hpp"

#define IM_VEC2_CLASS_EXTRA constexpr ImVec2(const glm::vec2& f) : x(f.x), y(f.y) {} operator glm::vec2() const { return glm::vec2(x,y); }
#define IM_VEC4_CLASS_EXTRA constexpr ImVec4(const glm::vec4& f) : x(f.x), y(f.y), z(f.z), w(f.w) {} operator glm::vec4() const { return glm::vec4(x,y,z,w); }
#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

namespace naku {

constexpr float FRAME_ROUNDING = 2;
constexpr float FONT_SIZE = 14;
constexpr float ICON_SIZE = 26;
constexpr float DEFAULT_LABEL_PADDING = 90;
constexpr float INDENT_SPACING = 8;

class GUI {
public:
	GUI(Engine& engine, Renderer& renderer, RenderPass& renderpass, uint32_t subpass);
	~GUI();

	GUI() = delete;
	GUI(const GUI&) = delete;
	GUI& operator=(const GUI&) = delete;
	GUI(GUI&&) = delete;
	GUI& operator=(GUI&&) = delete;

	void showFPS();
	void beginFrame();
	void draw();
	void endFrame() { draw(); }
	void render(VkCommandBuffer cmd);
	void beginLeftColumn();
	void endLeftColumn() { ImGui::End(); }
	void beginRightColumn();
	void endRightColumn() { ImGui::End(); }
	void beginBottomBox();
	void endBottomBox() { ImGui::End(); }
	void showDemoWindow();
	void showMonitor();
	void showGraphicsControl();
	void showObjectSelect();
	void showResourceSelect();
	void showInspector();
	void showPresentModes();
	void showPresentAttachments();
	void LeftLabel(const std::string& text, float padding);
	void LeftLabel(const std::string& text);
	void LeftLabel(const char* text, float padding);
	void LeftLabel(const char* text);

	VkDevice device() const { return _engine.device(); }

	bool showHint{ true };

private:
	struct ObjectReflect {
		// basic
		std::shared_ptr<Object> ptr{ nullptr };
		ResId id;
		std::string name;
		Object::Type type;
		float* pos{ nullptr };
		float* rot{ nullptr };
		float* scale{ nullptr };
		//bool receiveShadow{ true };

		// light
		Light::Type lightType;
		float *direction;
		float *radius;
		float *emission;
		float *outerAngle;
		float *innerAngleRatio;
		bool shadowmap;
		bool hasContactShadow;
		float* contactShadow;

		// camera
		float* fov{ nullptr };
		float* near{ nullptr };
		float* far{ nullptr };
		//float* up{ nullptr };
		//float* lookAt{ nullptr };

		void build(const std::shared_ptr<Object>& obj);
		void update() {
			ptr->update();
		}
		void showInspector(GUI& gui);

		std::string LightTypeName() {
			switch (lightType)
			{
			case Light::Type::POINT:
				return "Point Light"; break;
			case Light::Type::SPOT:
				return "Spot Light"; break;
			case Light::Type::DIRECTIONAL:
				return "Directional Light"; break;
			default:
				break;
			}
		}
	};
	struct MaterialReflect {
		ResId id;
		std::shared_ptr<Material> ptr{ nullptr };
		Material::Type type;
		std::string name;
		float* offset, * tilling, *albedo, *roughness, *metalness, *emission, *ior;

		void build(const std::shared_ptr<Material>& Material);
		//void update() { ptr->update(false); }
		void changeTexture(
			Engine& engine,
			MaterialTextures texture,
			const ResId& image);
		void removeTexture(Engine& engine, MaterialTextures texture);
		void showInspector(GUI& gui);
		bool hasTexture();
		std::string typeName() {
			switch (type)
			{
			case Material::Type::OPAQUE:
				return "Opaque"; break;
			case Material::Type::TRANSPARENT:
				return "Transparent"; break;
			default:
				break;
			}
		}
		void showTextureInfo(
			GUI& gui,
			bool* showImgTable,
			MaterialTextures* selectedTexture,
			const char* name,
			const char* id,
			MaterialTextures texture);
	};
	struct ImageReflect {
		ResId id;
		std::shared_ptr<Image2D> ptr{ nullptr };
		std::string name;
		uint32_t w, h;
		void build(const std::shared_ptr<Image2D>& Image);
		void showInspector(GUI& gui);
	};
	struct ModelReflect {
		ResId id;
		std::shared_ptr<Model> ptr{ nullptr };
		std::string name;
		uint32_t verts, faces;
		void build(const std::shared_ptr<Model>& Model);
		void showInspector(GUI& gui);
	};

	std::unique_ptr<ObjectReflect> _highlightedObj{ nullptr };
	std::unique_ptr<MaterialReflect> _highlightedMtl{ nullptr };
	std::unique_ptr<ImageReflect> _highlightedImg{ nullptr };
	std::unique_ptr<ModelReflect> _highlightedMdl{ nullptr };

	Engine& _engine;
	Device& _device;
	Window& _window;
	ResourceManager& _resources;
	Renderer& _renderer;
	static std::unique_ptr<Sampler> _defaultSampler;

	bool _firstLaunch{ true };

	// styles
	float _dpi, _font_size, _icon_size, _default_label_padding, 
		_left_column_width, _right_column_width, _thumbnail_width,
		_bottom_box_height,_indent_spacing;
	ImFont* _defaultFont{ nullptr };
	ImFont* _iconFont{ nullptr };
	ImFont* _cjkFont{ nullptr };

	// limiting refresh rate
	float _refreshTimer{ 0 };
	float _refreshLimit{ 0.1 };
	bool shouldRefresh() { return _refreshTimer >= _refreshLimit; }

	// helpers
	void HelpMarker(const char* desc);
	void clearObjectInspector() {
		_highlightedObj.reset(nullptr);
	}
	void clearMaterialInspector() {
		_highlightedMtl.reset(nullptr);
	}
	void clearResourceInspector() {
		_highlightedImg.reset(nullptr);
		_highlightedMdl.reset(nullptr);
	}
};

}

#endif