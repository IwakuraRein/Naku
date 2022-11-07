#ifndef SELECT_TABLE_HPP
#define SELECT_TABLE_HPP

#include "resources/resource.hpp"

#include <imgui.h>

#include <vector>
#include <unordered_map>
#include <string>
#include <filesystem>

namespace naku {

template<typename T>
class SelectTable {
public:
	SelectTable(
		ResourceCollection<T>& items,
		uint32_t columns = 4,
		bool autoClose = true)
		: items{ items }, _autoClose{ autoClose }, _columns{ columns }{}
	~SelectTable() {}

	bool showSelectTable(bool* p_open, bool isWindow = true, const std::string& title = "Select", ImGuiWindowFlags_ flags = ImGuiWindowFlags_None) {
		if (isWindow) ImGui::Begin(title.c_str(), p_open, flags);
		int col{ 0 };
		if (ImGui::BeginTable("select_table", _columns)) {
			ImGui::TableNextRow();
			for (ResId id = 0; id < items.size(); id++) {
				if (col == _columns) {
					ImGui::TableNextRow();
					col = 0;
				}
				ImGui::TableSetColumnIndex(col);
				std::string itemName = items.name(id);
				if (ImGui::Selectable(itemName.c_str(), selected == id)) {
					selected = id;
					if (_autoClose && p_open) *p_open = false;
					ImGui::EndTable();
					if (isWindow) ImGui::End();
					return true;
				}
				col++;
			}
			ImGui::EndTable();
		}
		if (isWindow) ImGui::End();


		return false;
	}
	ResourceCollection<T>& items;
	ResId selected{ERROR_RES_ID};

private:
	bool _autoClose;
	uint32_t _columns;
};

}

#endif