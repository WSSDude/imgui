#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_column_headers.h"

// Draw column headers
int ImGui::ColumnHeaders(const char* columnsId, ImColumnHeader* headers, int count, bool border) {
	if (count <= 0) {
		return -1;
	}

	int ret = -1;
	ImGuiStyle& style = GetStyle();
	const ImVec2 firstTextSize = CalcTextSize(headers[0].label, nullptr, true);

	BeginChild(columnsId, ImVec2(0, firstTextSize.y + 2 * style.ItemSpacing.y), true);

	char str_id[256];
	ImFormatString(str_id, 256, "Col_%s", columnsId);

	Columns(count, str_id, border);

	float offset = 0.0f;
	float workSize;
	
	static bool setSize = true;
	//static bool sizeChanged;

	//sizeChanged = false;
	for (int i = 0; i < count; ++i) {
		ImColumnHeader& header = headers[i];
		if (setSize) {
			workSize = header.currSize;
			SetColumnOffset(i, offset);
			if (header.currSize == -2.0f) {
				workSize = GetWindowWidth() - offset;
			}
			else if (header.currSize < 0.0f) {
				const ImVec2 textsize = CalcTextSize(header.label, nullptr, true);
				const float colSizeX = (textsize.x + 2 * style.ItemSpacing.x);
				if (header.currSize < 0.0f) {
					workSize = colSizeX;
				}
			}
			offset += workSize;
		}
		if (Selectable(header.label)) {
			ret = i;
		}
		if (!setSize) {
			header.currSize = GetColumnWidth();
			// TODO - this is broken ATM...
			//if (header.currSize < header.minSize) {
			//	header.currSize = header.minSize;
			//	setSize = true;
			//	sizeChanged = true;
			//}
		}
		NextColumn();
	}

	//if (!sizeChanged) {
		setSize = false;
	//}

	Columns(1);
	EndChild();

	return ret;
}

// Synchronize with column headers
void ImGui::BeginColumnHeadersSync(const char* columnsId, ImColumnHeader* headers, int count, bool border) {
	if (count <= 0) {
		return;
	}

	BeginChild(columnsId, ImVec2(0, -GetStyle().ItemSpacing.y), true);
	Columns(count, columnsId, border);

	float offset = 0.0f;
	for (int i = 0; i < count; ++i) {
		ImColumnHeader& header = headers[i];
		SetColumnOffset(i, offset);
		offset += header.currSize;
	}
}

void ImGui::EndColumnHeadersSync(ImColumnHeader* headers, int count) {
	if (count <= 0) {
		return;
	}

	Columns(1);
	EndChild();
}
