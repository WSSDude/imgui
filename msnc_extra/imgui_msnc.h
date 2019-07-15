#pragma once

namespace ImGui {
	IMGUI_API void              StyleMSNC(ImGuiStyle* dst = NULL);          // default MSNC style
  IMGUI_API void              TextWrappedUnformatted(const char* text, const char* text_end = NULL);         // shortcut for PushTextWrapPos(0.0f); TextUnformatted(text, text_end); PopTextWrapPos();. Note that this won't work on an auto-resizing window if there's no other widgets to extend the window width, yoy may need to set a size using SetNextWindowSize().
  IMGUI_API void              TextUnformattedDisabled(const char* text, const char* text_end = NULL);
  IMGUI_API void              HelpMarker(const char* desc);             // creates help marker with given description
  IMGUI_API bool              InputScalarFixed(const char* label, ImGuiDataType data_type, void* data_ptr, int length = 64, const void* step = NULL, const void* step_fast = NULL, const char* format = NULL, ImGuiInputTextFlags flags = 0); // get scalar of fixed length
  IMGUI_API ImFont*           AddFontMSNC(ImFontAtlas* atlas, const ImFontConfig* font_cfg = NULL);
  IMGUI_API const ImWchar*    GetGlyphRangesMSNC();                   // Characters for all languages supported by MSNC
}

void UnpackAccumulativeOffsetsIntoRanges(int base_codepoint, const short* accumulative_offsets, int accumulative_offsets_count, ImWchar* out_ranges);