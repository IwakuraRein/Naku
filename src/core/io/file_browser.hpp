#ifndef FILE_BROWSER_HPP
#define FILE_BROWSER_HPP

#include "naku.hpp"

#include <imgui.h>

namespace naku {

class FileBrowser {
public:
	struct FileInfo {
		std::filesystem::directory_entry entry;
		std::string fname;
		std::string fname_with_icon;

		FileInfo (const std::filesystem::directory_entry& Entry, const std::string& Fname, const char* Icon)
			: entry{ Entry }, fname{ Fname } {
			fname_with_icon = Icon;
			fname_with_icon += " " + Fname;
		}
	};

	FileBrowser(bool autoClose = true, bool showWarning = false);
	~FileBrowser() {}

	static const char* getIcon(const std::string& fileName);

	bool showBrowser(char* buf, bool* p_open);

private:
	bool _showWarning, _autoClose;
	std::filesystem::path _currentPath;
	std::filesystem::directory_entry _selectedEntry;
	std::vector<FileInfo> _fileList;

	void updateList();
};

}

#endif