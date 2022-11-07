#include "io/file_browser.hpp"
#include "io/IconsFontAwesome4.h"

#include <iostream>
#include <cstring>

using namespace ImGui;
namespace fileSys = std::filesystem;

namespace naku {

FileBrowser::FileBrowser(bool autoClose, bool showWarning)
	: _autoClose{ autoClose }, _showWarning{ showWarning } {
	_currentPath = fileSys::path{ fileSys::current_path() };
	_selectedEntry = fileSys::directory_entry{ _currentPath };
	updateList();
}

const char* FileBrowser::getIcon(const std::string& fileName) {
	// image
	if (strEndWith(fileName, ".png") ||
		strEndWith(fileName, ".jpg") ||
		strEndWith(fileName, ".jpeg")||
		strEndWith(fileName, ".bmp") ||
		strEndWith(fileName, ".webp"))
		return ICON_FA_FILE_IMAGE_O;
	// video
	if (strEndWith(fileName, ".mov") ||
		strEndWith(fileName, ".mp4") ||
		strEndWith(fileName, ".mpg") ||
		strEndWith(fileName, ".avi") ||
		strEndWith(fileName, ".wmv") ||
		strEndWith(fileName, ".rmvb")||
		strEndWith(fileName, ".mkv"))
		return ICON_FA_FILE_VIDEO_O;
	// audio
	if (strEndWith(fileName, ".mp3") ||
		strEndWith(fileName, ".wav") ||
		strEndWith(fileName, ".aac") ||
		strEndWith(fileName, ".ogg") ||
		strEndWith(fileName, ".flac")||
		strEndWith(fileName, ".ape") ||
		strEndWith(fileName, ".midi"))
		return ICON_FA_FILE_AUDIO_O;
	// code
	if (strEndWith(fileName, ".h")   ||
		strEndWith(fileName, ".hpp") ||
		strEndWith(fileName, ".cpp") ||
		strEndWith(fileName, ".c")   ||
		strEndWith(fileName, ".cc")  ||
		strEndWith(fileName, ".json")||
		strEndWith(fileName, ".py"))
		return ICON_FA_CODE;
	// text
	if (strEndWith(fileName, ".txt") ||
	    strEndWith(fileName, ".log") ||
	    strEndWith(fileName, ".md")  ||
	    strEndWith(fileName, ".rtf"))
		return ICON_FA_FILE_TEXT_O;
	// mesh
	if (strEndWith(fileName, ".obj") ||
		strEndWith(fileName, ".fbx") ||
		strEndWith(fileName, ".gltf"))
		return ICON_FA_CUBE;
	// miscellenous
	if (strEndWith(fileName, ".exe"))
		return ICON_FA_WINDOW_MAXIMIZE;
	if (strEndWith(fileName, ".ini"))
		return ICON_FA_COG;
	if (strEndWith(fileName, ".pdf"))
		return ICON_FA_FILE_PDF_O;
	if (strEndWith(fileName, ".doc"))
		return ICON_FA_FILE_WORD_O;
	if (strEndWith(fileName, ".dcox"))
		return ICON_FA_FILE_WORD_O;
	if (strEndWith(fileName, ".xls"))
		return ICON_FA_FILE_EXCEL_O;
	if (strEndWith(fileName, ".xlsx"))
		return ICON_FA_FILE_EXCEL_O;
	if (strEndWith(fileName, ".ppt"))
		return ICON_FA_FILE_POWERPOINT_O;
	if (strEndWith(fileName, ".pptx"))
		return ICON_FA_FILE_POWERPOINT_O;
	
	return ICON_FA_FILE_O;
}

void FileBrowser::updateList() {
	_fileList.clear();
	fileSys::directory_iterator list{ _currentPath };
	for (auto& entry : list) {
		try {
			const auto& path = fileSys::relative(entry.path(), _currentPath).generic_u8string();
			if (entry.is_directory())
				_fileList.push_back({ entry, path, ICON_FA_FOLDER_O });
			else
				_fileList.push_back({ entry, path, getIcon(path)});
		}
		catch (const std::exception& e) {
			if (_showWarning)
				std::cerr << "File Browser Warining: " << e.what() << std::endl;
		}
	}
}

bool FileBrowser::showBrowser(char* buf, bool* p_open) {
	if (ImGui::Begin("Browser", p_open)) {
		if (ImGui::Button(ICON_FA_ARROW_UP)) {
			_currentPath = fileSys::directory_entry(_currentPath.parent_path());
			updateList();
		}
		SameLine();
		ImGui::Text("%s", fileSys::absolute(_currentPath).generic_u8string().c_str());
		for (auto& file : _fileList) {
			Text(u8"    ©¸"); SameLine();
			if (ImGui::Selectable(file.fname_with_icon.c_str())) {
				_selectedEntry = file.entry;
				if (_selectedEntry.is_directory()) {
					_currentPath = _selectedEntry.path();
					updateList();

					ImGui::End();
					return false;
				}
				else if (_selectedEntry.is_regular_file()) {
					strcpy(buf, _selectedEntry.path().generic_u8string().c_str());

					ImGui::End();
					if (_autoClose && p_open) *p_open = false;
					return true;
				}
			}
		}
	}

	ImGui::End();
	return false;
}

}