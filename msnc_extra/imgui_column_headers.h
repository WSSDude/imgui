#pragma once

#include "imgui.h"

namespace ImGui
{
	// ColumnHeader
	struct ImColumnHeader {
		public:
			const char* label;    // Label of the header
			float       minSize;  // Negative value will calculate the size to fit label
			float       currSize; // Current column header size

			ImColumnHeader(const char* label = nullptr, float minSize = -1.0f) :
				label(label), minSize(minSize), currSize(minSize) {}
			~ImColumnHeader() = default;
	};

	// Draw column headers
	IMGUI_API int ColumnHeaders(const char* columnsId, ImColumnHeader* headers, int count, bool border = true);

	// Synchronize with column headers
	IMGUI_API void BeginColumnHeadersSync(const char* columnsId, ImColumnHeader* headers, int count, bool border = true);
	IMGUI_API void EndColumnHeadersSync(ImColumnHeader* headers, int count);
}
