#include "io/gui.hpp"
#include "io/file_browser.hpp"
#include "io/select_table.hpp"
#include "io/IconsFontAwesome4.h"

#include <cstring>

using namespace ImGui;
namespace fileSys = std::filesystem;

namespace naku { //GUI

std::unique_ptr<Sampler> GUI::_defaultSampler{ nullptr };

GUI::GUI(
	Engine& engine,
	Renderer& renderer,
	RenderPass& renderpass,
	uint32_t subpass)
	:_engine{ engine }, _renderer{ renderer }, _device{ *engine.pDevice }, _resources{ engine.resources }, _window{ *engine.pWindow }{
	_dpi = _window._dpi;
	//1: initilize components
	if (!_defaultSampler) {
		auto samplerCreateInfo = Sampler::getDefaultSamplerCreateInfo();
		_defaultSampler = std::make_unique<Sampler>(_device, samplerCreateInfo);
	}


	// 2: initialize imgui library

	//this initializes the core structures of imgui
	CreateContext();

	//this initializes imgui for Vulkan
	ImGui_ImplGlfw_InitForVulkan(_engine.pWindow->pWindow(), true);

	//this initializes imgui for Vulkan
	ImGui_ImplVulkan_InitInfo initInfo = {};
	initInfo.Instance = _device.instance();
	initInfo.PhysicalDevice = _device.physicalDevice();
	initInfo.Device = engine.device();
	initInfo.Queue = _device.graphicsQueue();
	initInfo.DescriptorPool = _engine.pDescriptorSetPool->getPool();
	initInfo.MinImageCount = _device.getSwapChainSupport().capabilities.minImageCount;
	initInfo.ImageCount = _renderer.getImageCount();
	initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
	initInfo.Subpass = subpass;
	//TODO: getPostRenderPass
	ImGui_ImplVulkan_Init(&initInfo, renderpass.renderPass());

	// 3. Style
	_font_size = FONT_SIZE * _dpi;
	_icon_size = ICON_SIZE * _dpi;
	_indent_spacing = INDENT_SPACING * _dpi;
	_default_label_padding = DEFAULT_LABEL_PADDING * _dpi;
	_left_column_width = _window._left_padding;
	_right_column_width = _window._right_padding;
	_thumbnail_width = _right_column_width - 20.f;
	_bottom_box_height = _window._bottom_padding;
	_bottom_box_height = _window._bottom_padding;
	//execute a gpu command to upload imgui font textures
	ImGuiIO& io = GetIO();
	ImVector<ImWchar> ranges;
	const ImWchar range1[] =
	{
		0x0020, 0x00FF, // Basic Latin + Latin Supplement
		0x2000, 0x206F, // General Punctuation
		0x2190, 0x21FF, // Arrows
		0x2500, 0x257F, // Boxes
		0,
	};
	const ImWchar range2[] =
	{
		0x3000, 0x30FF, // CJK Symbols and Punctuations, Hiragana, Katakana
		0x31F0, 0x31FF, // Katakana Phonetic Extensions
		0xFF00, 0xFFEF, // Halfwidth and Fullwidth Forms
		0x4e00, 0x9FAF, // CJK Ideograms
		0,
	};
	const ImWchar range3[] = { ICON_MIN_FA, ICON_MAX_FA, 0 }; //icons
	ImFontConfig config;
	config.MergeMode = true;
	std::cout << "Loading fonts...";
	_defaultFont = io.Fonts->AddFontFromFileTTF("res/miscellenous/JetBrainsMono-Light.ttf", _font_size, nullptr, range1);
	_cjkFont = io.Fonts->AddFontFromFileTTF("res/miscellenous/WenQuanYiMicroHeiMonoLight.ttf", _font_size, &config, range2);
	_iconFont = io.Fonts->AddFontFromFileTTF("res/miscellenous/fontawesome-webfont.ttf", _font_size, &config, range3);
	IM_ASSERT(_defaultFont != NULL);
	IM_ASSERT(_cjkFont != NULL);
	IM_ASSERT(_iconFont != NULL);
	io.Fonts->Build();
	auto cmd = _device.beginSingleTimeCommands();
	ImGui_ImplVulkan_CreateFontsTexture(cmd);
	_device.endSingleTimeCommands(cmd);

	//clear font textures from cpu data
	ImGui_ImplVulkan_DestroyFontUploadObjects();
	std::cout << "Done." << std::endl;

	StyleColorsDark();
	//_style = std::make_shared<ImGuiStyle>(GetStyle());
	auto& style = GetStyle();
	//_style->WindowBorderSize = 0;
	//style.WindowBorderSize = 0;
	style.FrameRounding = FRAME_ROUNDING;
	style.IndentSpacing = _indent_spacing;
	style.Colors[ImGuiCol_WindowBg] = ImVec4(0.2f, 0.2f, 0.2f, 1.0f);
	style.ScaleAllSizes(_dpi);
}

GUI::~GUI() {
	ImGui_ImplVulkan_Shutdown();
	if (_defaultSampler)
		_defaultSampler.reset(nullptr);
}

void GUI::beginBottomBox() {
	ImGui::Begin("##bottom_box",
		nullptr,
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoResize);
	ImGui::SetWindowPos(ImVec2(_left_column_width, _window.height() - _bottom_box_height));
	ImGui::SetWindowSize(ImVec2(_window.width() - _left_column_width - _right_column_width, _bottom_box_height));
}

void GUI::beginLeftColumn() {
	ImGui::Begin("##left_column",
		nullptr,
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoResize);
	ImGui::SetWindowPos(ImVec2(0, 0));
	ImGui::SetWindowSize(ImVec2(_left_column_width, _window.height()));
}
void GUI::beginRightColumn() {
	ImGui::Begin("##right_column",
		nullptr,
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoResize);
	ImGui::SetWindowPos(ImVec2(_window.width() - _right_column_width, 0));
	ImGui::SetWindowSize(ImVec2(_right_column_width, _window.height()));
}

void GUI::LeftLabel(const char* text, float padding) {
	Text(text);
	SameLine(padding);
}
void GUI::LeftLabel(const char* text) {
	LeftLabel(text, _default_label_padding);
}
void GUI::LeftLabel(const std::string& text, float padding) {
	LeftLabel(text.c_str(), padding);
}

void GUI::LeftLabel(const std::string& text) {
	LeftLabel(text, _default_label_padding);
}

void GUI::showMonitor() {
	if (_firstLaunch) SetNextItemOpen(true);
	if (CollapsingHeader("System Monitor")) {
		showFPS();
	}
}

void GUI::showGraphicsControl() {
	if (_firstLaunch) SetNextItemOpen(true);
	if (CollapsingHeader("Graphics Settings")) {
		LeftLabel("Alpha");
		DragFloat("##alpha", &_engine.alpha, 0.01f, 0.01f, 10.f);
		LeftLabel("Gamma");
		DragFloat("##gamma", &_engine.gamma, 0.01f, 0.01f, 10.f);
		LeftLabel("Environment");
		ColorEdit4("##environment", glm::value_ptr(_engine.globalUbo.environment), ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR);
		showPresentModes();
		showPresentAttachments();
	}
}

void GUI::showPresentModes() {
	static int _chosenPresentMode{ 0 };
	//RadioButton("V-sync", &_chosenPresentMode, 0); SameLine();
	//RadioButton("Mail box", &_chosenPresentMode, 1); SameLine();
	//RadioButton("Immediate", &_chosenPresentMode, 2);

	static const char* items[] = { "V-sync", "Mail box", "Immediate" };
	LeftLabel("V-sync Mode");
	if (Combo("##present_mode", &_chosenPresentMode, items, IM_ARRAYSIZE(items))) {
		switch (_chosenPresentMode)
		{
		case 0: _renderer.changePresentMode(VK_PRESENT_MODE_FIFO_KHR); break;
		case 1: _renderer.changePresentMode(VK_PRESENT_MODE_MAILBOX_KHR); break;
		case 2: _renderer.changePresentMode(VK_PRESENT_MODE_IMMEDIATE_KHR); break;
		default: break;
		}
	}
}
void GUI::showPresentAttachments() {
	LeftLabel("Present");
	static const char* items[] = { "Shaded", "Normal", "Albedo", "Depth", "World Position", "Metalness", "Roughness", "Occlusion", "Emission"/*, "Object"*/};
	Combo("##present_attachment", &_engine.presentAttachment, items, IM_ARRAYSIZE(items));
}

void GUI::showObjectSelect() {
	Object* selectedObj{ nullptr };
	if (_firstLaunch) SetNextItemOpen(true);

	if (CollapsingHeader("Objects \xef\x86\xb2")) {
		if (!_highlightedObj) selectedObj = nullptr;
		else selectedObj = _highlightedObj->ptr.get();
		PushID("object_list");
		for (ResId id = 0; id < _resources.size<Object>(); id++) {
			auto obj = _resources.get<Object>(id);
			if (obj->type() == Object::Type::MESH) {
				if (Selectable(obj->name().c_str(), selectedObj == obj.get())) {
					clearObjectInspector();
					_highlightedObj = std::make_unique<ObjectReflect>();
					_highlightedObj->build(obj);
					selectedObj = obj.get();
				}
			}
		}
		PopID();
	}
	if (_firstLaunch) SetNextItemOpen(true);
	if (CollapsingHeader("Lights \xef\x83\xab")) {
		if (!_highlightedObj) selectedObj = nullptr;
		else selectedObj = _highlightedObj->ptr.get();
		PushID("light_list");
		for (ResId id = 0; id < _resources.size<Light>(); id++) {
			auto light = _resources.get<Light>(id);
			auto obj = _resources.get<Object>(light->objId());

			if (Selectable(obj->name().c_str(), selectedObj == obj.get())) {
				clearObjectInspector();
				_highlightedObj = std::make_unique<ObjectReflect>();
				_highlightedObj->build(obj);
				selectedObj = obj.get();
			}
		}
		PopID();
	}
	if (_firstLaunch) SetNextItemOpen(true);
	if (CollapsingHeader("Cameras \xef\x80\xb0")) {
		if (!_highlightedObj) selectedObj = nullptr;
		else selectedObj = _highlightedObj->ptr.get();
		PushID("camera_list");
		for (ResId id = 0; id < _resources.size<Camera>(); id++) {
			auto cam = _resources.get<Camera>(id);
			auto obj = _resources.get<Object>(cam->objId());

			if (Selectable(obj->name().c_str(), selectedObj == obj.get())) {
				clearObjectInspector();
				_highlightedObj = std::make_unique<ObjectReflect>();
				_highlightedObj->build(obj);
				selectedObj = obj.get();
			}
		}
		PopID();
	}
}

void GUI::showResourceSelect() {
	static enum Catogery {
		NONE,
		MODEL,
		IMAGE,
		MATERIAL,
	} catogery{ Catogery::NONE };
	static float left_col_width{ 100 * _dpi };
	static SelectTable<Material> mtlTable{ _resources.getResource<Material>(), 4, false };
	static SelectTable<Model> mdlTable{ _resources.getResource<Model>(), 4, false };
	static SelectTable<Image2D> imgTable{ _resources.getResource<Image2D>(), 4, false };
	static FileBrowser fileBrowser{};
	static char selectedFile[512];
	static bool showBrowser{ false };
	BeginChild("##resource_catogeries", { left_col_width, GetContentRegionAvail().y }, false);
	if (Selectable("Images", catogery == Catogery::IMAGE)) {
		catogery = Catogery::IMAGE;
		if (_highlightedImg) imgTable.selected = _highlightedImg->id;
	}
	if (Selectable("Materials", catogery == Catogery::MATERIAL)) {
		catogery = Catogery::MATERIAL;
		if (_highlightedMtl) mtlTable.selected = _highlightedMtl->id;
	}
	if (Selectable("Models", catogery == Catogery::MODEL)) {
		catogery = Catogery::MODEL;
	}
	EndChild();

	ImGui::SameLine();

	ImGui::BeginChild("##resources", { 0, GetContentRegionAvail().y }, true);
	if (catogery == Catogery::MATERIAL) {
		if (!_highlightedMtl) mtlTable.selected = ERROR_RES_ID;
		//PushID("material_select");
		if (mtlTable.showSelectTable(nullptr, false)) {
			clearMaterialInspector();
			_highlightedMtl = std::make_unique<MaterialReflect>();
			_highlightedMtl->build(_resources.get<Material>(mtlTable.selected));
		}
		//PopID();
	}
	if (catogery == Catogery::IMAGE) {
		if (!_highlightedImg) imgTable.selected = ERROR_RES_ID;
		//PushID("image_select");
		if (Button("Import")) {
			showBrowser = true;
		}
		if (imgTable.showSelectTable(nullptr, false)) {
			clearResourceInspector();
			_highlightedImg = std::make_unique<ImageReflect>();
			_highlightedImg->build(_resources.get<Image2D>(imgTable.selected));
		}
		if (showBrowser) {
			if (fileBrowser.showBrowser(selectedFile, &showBrowser)) {
				_engine.createImage(selectedFile);
			}
		}

		//PopID();
	}
	if (catogery == Catogery::MODEL) {
		if (!_highlightedMdl) mdlTable.selected = ERROR_RES_ID;
		//PushID("model_select");
		if (Button("Import")) {
			showBrowser = true;
		}
		if (mdlTable.showSelectTable(nullptr, false)) {
			clearResourceInspector();
			_highlightedMdl = std::make_unique<ModelReflect>();
			_highlightedMdl->build(_resources.get<Model>(mdlTable.selected));
		}
		if (showBrowser) {
			if (fileBrowser.showBrowser(selectedFile, &showBrowser)) {
				_engine.createModel(getFileName(selectedFile), selectedFile);
			}
		}

		//PopID();
	}
	EndChild();
}

void GUI::showInspector() {
	if (_firstLaunch) SetNextItemOpen(true);
	if (CollapsingHeader("Inspector - Object")) {
		if (_highlightedObj) _highlightedObj->showInspector(*this);
	}
	if (_firstLaunch) SetNextItemOpen(true);
	if (CollapsingHeader("Inspector - Material")) {
		if (_highlightedMtl) _highlightedMtl->showInspector(*this);
	}
	if (_firstLaunch) SetNextItemOpen(true);
	if (CollapsingHeader("Inspector - Resource")) {
		if (_highlightedImg) _highlightedImg->showInspector(*this);
		if (_highlightedMdl) _highlightedMdl->showInspector(*this);
	}

}

void GUI::showFPS() {
	static float fpsRate{ 0.f };
	static char fps[16];
	static uint32_t fpsCounter{ 0 };
	static float fpsRates[50];
	static int offset = 0;
	fpsCounter += 1;
	if (shouldRefresh()) {
		fpsRate = fpsCounter / _refreshTimer;
		fpsCounter = 0;
		fpsRates[offset] = fpsRate;
		offset = (offset + 1) % 50;
	}
	static bool showPlot{ true };
	//if (SmallButton("FPS")) {
	//	showPlot = !showPlot;
	//}
	//if (ImGui::IsItemHovered())
	//	ImGui::SetTooltip("Click to show/hide FPS line.");
	//SameLine();
	LeftLabel("FPS");
	if (!showPlot) Text("%.1f", fpsRate);
	else {
		float maxFps{ 0.f };
		for (int i = 0; i < 50; i++) {
			if (fpsRates[i] > maxFps) maxFps = fpsRates[i];
		}
		sprintf(fps, "%.1f", fpsRate);
		PlotLines("##fpsRates", fpsRates, 50, offset, fps, 0.f, maxFps);
	}
}

void GUI::beginFrame() {
	_refreshTimer += _engine.deltaTime;

	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();

	NewFrame();
}

void GUI::draw() {

	Render();

	if (shouldRefresh()) {
		_refreshTimer = 0.f;
	}

	if (_firstLaunch) _firstLaunch = false;
}

void GUI::showDemoWindow() {
	ShowDemoWindow();
}

void GUI::render(VkCommandBuffer cmd) {
	ImGui_ImplVulkan_RenderDrawData(GetDrawData(), cmd);
}

void GUI::HelpMarker(const char* desc)
{
	if (showHint) {
		SameLine();
		ImGui::TextDisabled(ICON_FA_QUESTION_CIRCLE_O);
		if (ImGui::IsItemHovered())
		{
			ImGui::BeginTooltip();
			ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
			ImGui::TextUnformatted(desc);
			ImGui::PopTextWrapPos();
			ImGui::EndTooltip();
		}
	}
}

}

namespace naku { //reflects

void GUI::ObjectReflect::build(const std::shared_ptr<Object>& obj) {
	id = obj->id();
	name = obj->name();
	type = obj->type();
	ptr = obj;
	pos = glm::value_ptr(ptr->_position);
	rot = glm::value_ptr(ptr->_rotation);
	scale = glm::value_ptr(ptr->_scale);

	if (type == Object::Type::LIGHT) {
		Light* pLight = (Light*)ptr.get();
		emission = glm::value_ptr(pLight->lightInfo.emission);
		this->lightType = pLight->type();
		radius = &pLight->lightInfo.radius;
		direction = glm::value_ptr(pLight->projector.forwardDir);
		innerAngleRatio = &pLight->lightInfo.innerAngleRatio;
		outerAngle = &pLight->lightInfo.outerAngle;
		direction = glm::value_ptr(pLight->lightInfo.direction);
		shadowmap = pLight->lightInfo.shadowmap > 0;
		hasContactShadow = pLight->lightInfo.contactShadow > 0;
		contactShadow = &pLight->lightInfo.contactShadow;
	}
	if (type == Object::Type::CAMERA) {
		Camera* pCam = (Camera*)ptr.get();
		fov = &pCam->_projector.fov;
		near = &pCam->_projector.near;
		far = &pCam->_projector.far;
		//up = glm::value_ptr(pCam->_upDir);
		//lookAt = glm::value_ptr(pCam->_forwardDir);
	}
}

void GUI::MaterialReflect::changeTexture(
	Engine& engine,
	MaterialTextures texture,
	const ResId& image) {
	const uint32_t binding = static_cast<uint32_t>(texture);
	engine.addGarbage(ptr->_textures[binding]);
	std::string name;
	bool anisotropic_filtering{ false };
	if (texture == MaterialTextures::BASE) {
		name = "baseTex";
		anisotropic_filtering = true;
	}
	if (texture == MaterialTextures::NORMAL) {
		name = "normal";
	}
	if (texture == MaterialTextures::METALNESS) {
		name = "metalness";
	}
	if (texture == MaterialTextures::ROUGHNESS) {
		name = "roughness";
	}
	if (texture == MaterialTextures::OCCLUSION) {
		name = "occlusion";
	}
	if (texture == MaterialTextures::EMISSION) {
		name = "emission";
		anisotropic_filtering = true;
	}
	auto pTex = std::make_shared<Texture>(
		*engine.pDevice,
		name,
		engine.resources.get<Image2D>(image),
		anisotropic_filtering);
	ptr->changeTexture(binding, pTex, false);
	engine.addLateUpdate(*ptr->_writer, ptr->_set);
}
bool GUI::MaterialReflect::hasTexture() {
	return ptr->pushConstants.hasTexture != 0;
}
void GUI::MaterialReflect::removeTexture(Engine& engine, MaterialTextures texture) {
	const uint32_t binding = static_cast<uint32_t>(texture);
	engine.addGarbage(ptr->_textures[binding]);
	ptr->removeTexture(binding, false);
	engine.addLateUpdate(*ptr->_writer, ptr->_set);
}

void GUI::MaterialReflect::build(const std::shared_ptr<Material>& Material) {
	id = Material->id();
	type = Material->type();

	name = Material->name();
	ptr = Material;
	offset = glm::value_ptr(Material->pushConstants.offsetTilling);
	tilling = glm::value_ptr(Material->pushConstants.offsetTilling) + 2;
	albedo = glm::value_ptr(Material->pushConstants.albedo);
	emission = glm::value_ptr(Material->pushConstants.emission);
	roughness = &Material->pushConstants.roughness;
	metalness = &Material->pushConstants.metalness;
	ior = &Material->pushConstants.ior;
}

void GUI::ImageReflect::build(const std::shared_ptr<Image2D>& Image) {
	id = Image->id();
	ptr = Image;
	name = Image->name();
	w = Image->width();
	h = Image->height();
	if (!ptr->ImGuiImageId) {
		ptr->ImGuiImageId = std::make_unique<VkDescriptorSet>();
		*ptr->ImGuiImageId = ImGui_ImplVulkan_AddTexture(_defaultSampler->sampler(), ptr->defaultImageView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}
}

void GUI::ModelReflect::build(const std::shared_ptr<Model>& Model) {
	id = Model->id();
	ptr = Model;
	name = Model->name();
	verts = Model->vertexCount();
	faces = Model->indexCount();
}

void GUI::MaterialReflect::showTextureInfo(
	GUI& gui,
	bool* showImgTable,
	MaterialTextures* selectedTexture,
	const char* name,
	const char* id,
	MaterialTextures texture) {
	const uint32_t binding = static_cast<uint32_t>(texture);
	gui.LeftLabel(name);
	PushID(id);
	if ((ptr->pushConstants.hasTexture & (1<<binding)) != 0) {
		auto tex = ptr->_textures[binding];
		if (Button(tex->fileName().c_str())) {
			gui.clearResourceInspector();
			gui._highlightedImg = std::make_unique<ImageReflect>();
			gui._highlightedImg->build(tex->_pImage);
			PopID();
			return;
		} SameLine();
	}
	if (SmallButton(ICON_FA_FOLDER_OPEN_O)) {
		*showImgTable = true;
		*selectedTexture = static_cast<MaterialTextures>(binding);
	} SameLine();
	if (SmallButton(ICON_FA_TIMES)) {
		this->removeTexture(gui._engine, texture);
	}
	PopID();
}

void GUI::MaterialReflect::showInspector(GUI& gui) {
	static bool showImgTable{ false };
	static char* selectedFilePath{ nullptr };
	static SelectTable<Image2D> imgTable{ gui._resources.getResource<Image2D>() };
	static MaterialTextures selectedTexture;

	int flag{ 0 };
	Text(this->name.c_str());
	Indent();
	gui.LeftLabel("Type"); Text(typeName().c_str());
	gui.LeftLabel("Albedo");
	if (ptr->type() == Material::Type::TRANSPARENT) {
		if (ColorEdit4("##albedo", albedo, ImGuiColorEditFlags_AlphaBar)) {
			//this->update();
		};
	}
	else {
		if (ColorEdit3("##albedo", albedo)) {
			//this->update();
		};
	}
	gui.LeftLabel("Emission");
	if (ColorEdit4("##emission", emission, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR)) {
		//this->update();
	};
	gui.LeftLabel("Roughness");
	if (DragFloat("##roughness", roughness, 0.01f, 0.f, 1.f)) {
		//this->update();
	};
	gui.LeftLabel("Metalness");
	if (DragFloat("##metalness", metalness, 0.01f, 0.f, 1.f)) {
		//this->update();
	};
	if (type == Material::Type::TRANSPARENT) {
		gui.LeftLabel("Ior");
		if (DragFloat("##ior", ior, 0.01f, 0.f, 1.f)) {
			//this->update();
		};
	}

	Separator();
	gui.LeftLabel("Offset");
	if (DragFloat2("##Offset", this->offset, 0.01f)) {
		//this->update();
	}
	gui.LeftLabel("Tilling");
	if (DragFloat2("##tilling", this->tilling, 0.01f)) {
		//this->update();
	}
	showTextureInfo(gui, &showImgTable, &selectedTexture, "Base", "##base_tex", MaterialTextures::BASE);
	showTextureInfo(gui, &showImgTable, &selectedTexture, "Normal", "##normal_tex", MaterialTextures::NORMAL);
	showTextureInfo(gui, &showImgTable, &selectedTexture, "Metalness", "##metalness_tex", MaterialTextures::METALNESS);
	showTextureInfo(gui, &showImgTable, &selectedTexture, "Roughness", "##roughness_tex", MaterialTextures::ROUGHNESS);
	showTextureInfo(gui, &showImgTable, &selectedTexture, "Occlusion", "##occlusion_tex", MaterialTextures::OCCLUSION);
	showTextureInfo(gui, &showImgTable, &selectedTexture, "Emission", "##emission_tex", MaterialTextures::EMISSION);
	if (showImgTable) {
		if (imgTable.showSelectTable(&showImgTable, "Images")) {
			this->changeTexture(gui._engine, selectedTexture, imgTable.selected);
		}
	}
	Unindent();
}

void GUI::ImageReflect::showInspector(GUI& gui) {
	static char imgName[256];
	strcpy(imgName, this->name.c_str());
	Text(strcat(imgName, " \xef\x87\x85"));
	gui.LeftLabel("Resolution");
	Text(u8"%d × %d", this->w, this->h);
	gui.LeftLabel("Path");
	TextWrapped(ptr->filePath().c_str());
	ImGui::Image((ImTextureID)*ptr->ImGuiImageId, { gui._thumbnail_width, gui._thumbnail_width });
}

void GUI::ModelReflect::showInspector(GUI& gui) {
	static char mdlName[256];
	strcpy(mdlName, this->name.c_str());
	Text(strcat(mdlName, " \xef\x86\xb2"));
	gui.LeftLabel("Vertices");
	Text("%d", this->verts);
	gui.LeftLabel("Triangles");
	Text("%d", this->faces);
	gui.LeftLabel("Path");
	TextWrapped(this->ptr->filePath().c_str());
}
void GUI::ObjectReflect::showInspector(GUI& gui) {
	static char objName[256];
	strcpy(objName, this->name.c_str());
	if (this->type == Object::Type::MESH) {
		Checkbox("##active", &this->ptr->_active); 
		SameLine(); Text(strcat(objName, " \xef\x86\xb2"));
		Indent();
		gui.LeftLabel("Cast Shadow");
		Checkbox("##cast_shadow", &ptr->_castShadow);
		gui.LeftLabel("Position");
		if (DragFloat3("##position", this->pos, 0.02f)) {
			this->update();
		};
		gui.LeftLabel("Scale");
		if (DragFloat3("##scale", this->scale, 0.02f, 0.f, std::numeric_limits<float>::max(), "%.3f", ImGuiSliderFlags_AlwaysClamp)) {
			this->update();
		};
		gui.LeftLabel("Rotation");
		if (DragFloat3("##rotation", this->rot, 1.f)) {
			if (abs(this->rot[0]) > 360.f) {
				const int x = this->rot[0];
				this->rot[0] = x / 360.f;
			}
			if (abs(this->rot[1]) > 360.f) {
				const int x = this->rot[1];
				this->rot[1] = x / 360.f;
			}
			if (abs(this->rot[2]) > 360.f) {
				const int x = this->rot[2];
				this->rot[2] = x / 360.f;
			}
			this->update();
		};

		static SelectTable<Model> mdlTable{ gui._resources.getResource<Model>() };
		static bool showMdlTable{ false };
		if (this->ptr->model) {
			PushID("model_info");
			auto pMdl = this->ptr->model;
			gui.LeftLabel("Mesh");
			if (Button(pMdl->name().c_str())) {
				gui.clearResourceInspector();
				gui._highlightedMdl = std::make_unique<ModelReflect>();
				gui._highlightedMdl->build(pMdl);
				PopID();
				Unindent();
				return;
			} SameLine();
			if (SmallButton(ICON_FA_FOLDER_OPEN_O)) {
				showMdlTable = true;
			}
			PopID();
		}
		static SelectTable<Material> mtlTable{ gui._resources.getResource<Material>() };
		static bool showMtlTable{ false };
		if (this->ptr->material) {
			PushID("material_info");
			auto pMtl = this->ptr->material;
			gui.LeftLabel("Material");
			if (Button(pMtl->name().c_str())) {
				gui.clearMaterialInspector();
				gui._highlightedMtl = std::make_unique<MaterialReflect>();
				gui._highlightedMtl->build(pMtl);
				PopID();
				Unindent();
				return;
			} SameLine();
			if (SmallButton(ICON_FA_FOLDER_OPEN_O)) {
				showMtlTable = true;
			}
			PopID();
		}
		if (showMtlTable) {
			if (mtlTable.showSelectTable(&showMtlTable, true, "Materials")) {
				if (this) {
					if (this->ptr->material) {
						ResId oldMtlId = this->ptr->material->id();
						ResId newMtlId = mtlTable.selected;
						this->ptr->material = gui._resources.get<Material>(mtlTable.selected);
						gui._resources.removeFromCollect<Material, Object>(oldMtlId, this->ptr->id());
						gui._resources.addCollect<Material, Object>(newMtlId, this->ptr->id());
					}
				}
			}
		}
		if (showMdlTable) {
			if (mdlTable.showSelectTable(&showMdlTable, true, "Models")) {
				if (this) {
					if (this->ptr->model)
						this->ptr->model = gui._resources.get<Model>(mdlTable.selected);
				}
			}
		}
	}
	if (this->type == Object::Type::LIGHT) {
		Checkbox("##active", &this->ptr->_active);
		SameLine();
		Text(strcat(objName, " \xef\x83\xab"));
		Indent();
		gui.LeftLabel("Type");
		Text(LightTypeName().c_str());
		auto pLight = (Light*)ptr.get();
		gui.LeftLabel("Shadowmap");
		if (Checkbox("##shadowmap", &this->shadowmap)) {
			if (this->shadowmap) pLight->lightInfo.shadowmap = 1;
			else pLight->lightInfo.shadowmap = -1;
		}
		gui.LeftLabel("Contact shadow");
		if (Checkbox("##contact_shadow", &this->hasContactShadow)) {
			if (this->hasContactShadow) pLight->lightInfo.contactShadow = 0.001f;
			else pLight->lightInfo.contactShadow = -1.f;
		} 
		if (this->hasContactShadow) {
			SameLine();
			InputFloat("##contact_shadow_value", this->contactShadow, 0.f, 0.f, "%.5f");
			*this->contactShadow = glm::clamp(*this->contactShadow, std::numeric_limits<float>::min(), 0.5f);
		}
		gui.LeftLabel("Position");
		if (DragFloat3("##position", this->pos, 0.02f)) {
			this->update();
		};
		gui.LeftLabel("Emission");
		if (ColorEdit4("##emission", this->emission, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR)) {
			//this->update();
		};
		gui.LeftLabel("Radius");
		if (DragFloat("##radius", this->radius, 0.05f, 0.0f, std::numeric_limits<float>::max(), "%.3f", ImGuiSliderFlags_AlwaysClamp)) {
			//this->update();
			pLight->updateProjection();
		};
		if (this->lightType == Light::Type::SPOT || this->lightType == Light::Type::DIRECTIONAL) {
			gui.LeftLabel("Rotation");
			if (DragFloat2("##rotation", this->rot, 1.f)) {
				if (abs(this->rot[0]) > 360.f) {
					const int x = this->rot[0];
					this->rot[0] = x / 360.f;
				}
				if (abs(this->rot[1]) > 360.f) {
					const int x = this->rot[1];
					this->rot[1] = x / 360.f;
				}
				this->update();
			};
			gui.LeftLabel("Direction");
			Text("%.3f %.3f %.3f", this->direction[0], this->direction[1], this->direction[2]);
		}
		if (this->lightType == Light::Type::SPOT) {
			gui.LeftLabel("Angle");
			if (DragFloat("##angle", this->outerAngle, 0.5f, 0.0f, 180.f, "%.3f", ImGuiSliderFlags_AlwaysClamp)) {
				//this->update();
				pLight->updateProjection();
			};
			gui.LeftLabel("Inner Ratio");
			if (DragFloat("##inner_ratio", this->innerAngleRatio, 0.01f, 0.0f, 1.f, "%.3f", ImGuiSliderFlags_AlwaysClamp)) {
				//this->update();
			};
		}
	}
	if (this->type == Object::Type::CAMERA) {
		Text(strcat(objName, " \xef\x80\xb0"));
		Indent();
		bool shouldUpdate{ false };
		auto id = ((Camera*)this->ptr.get())->id();
		auto cam = gui._engine.resources.get<Camera>(id);
		bool isMainCam = (cam == gui._engine.pMainCamera);
		bool setMainCam = isMainCam;
		gui.LeftLabel("Main Camera");
		Checkbox("##main_camera", &setMainCam);
		if (!isMainCam && setMainCam) {
			gui._engine.pMainCamera = cam;
			gui._engine.globalUp = cam->upDir();
		}
		gui.LeftLabel("Position");
		if (DragFloat3("##CamPosition", this->pos, 0.02f)) {
			shouldUpdate = true;
		};
		gui.LeftLabel("Rotation");
		if (DragFloat3("##CamRotation", this->rot, 1.f)) {
			shouldUpdate = true;
		};
		Indent(gui._default_label_padding - gui._indent_spacing); 
		if (Button("Reset Up Direction")) {
			gui._engine.globalUp = cam->upDir();
		} gui.HelpMarker("You may want to update and lock camera's up direction after some rolling.");
		Unindent(gui._default_label_padding - gui._indent_spacing);
		gui.LeftLabel("Fov");
		if (DragFloat("##CamFov", this->fov, 0.05f, 10.f, 179.9f, "%3.f", ImGuiSliderFlags_AlwaysClamp)) {
			shouldUpdate = true;
		};
		gui.LeftLabel("Far");
		if (DragFloat("##cam_far", this->far, 0.1f, *this->near, 99999.f, "%.3f", ImGuiSliderFlags_AlwaysClamp)) {
			shouldUpdate = true;
		};
		gui.LeftLabel("Near");
		if (DragFloat("##cam_near", this->near, 0.001f, 0.01f, *this->far, "%.4f", ImGuiSliderFlags_AlwaysClamp)) {
			shouldUpdate = true;
		};
		gui.LeftLabel("Up");
		Text("%.3f %.3f %.3f", cam->_projector.upDir.x, cam->_projector.upDir.y, cam->_projector.upDir.z);
		gui.LeftLabel("Forward");
		Text("%.3f %.3f %.3f", cam->_projector.forwardDir.x, cam->_projector.forwardDir.y, cam->_projector.forwardDir.z);
		gui.LeftLabel("Right");
		Text("%.3f %.3f %.3f", cam->_projector.rightDir.x, cam->_projector.rightDir.y, cam->_projector.rightDir.z);
		if (shouldUpdate) {
			cam->update();
			cam->updateProjectionMat();
			shouldUpdate = false;
		}
	}
	Unindent();
}

}