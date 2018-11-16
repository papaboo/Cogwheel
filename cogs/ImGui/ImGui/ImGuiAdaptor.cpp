// Cogwheel Imgui adaptor.
// ------------------------------------------------------------------------------------------------
// Copyright (C) 2018, Cogwheel. See AUTHORS.txt for authors
//
// This program is open source and distributed under the New BSD License.
// See LICENSE.txt for more detail.
// ------------------------------------------------------------------------------------------------

#include <ImGui/ImGuiAdaptor.h>

#include <Cogwheel/Core/Engine.h>
#include <Cogwheel/Input/Keyboard.h>
#include <Cogwheel/Input/Mouse.h>
#include <Cogwheel/Math/Vector.h>

#define IMGUI_DEFINE_MATH_OPERATORS
#include <ImGui/Src/imgui_internal.h>

#include <sstream>

using namespace Cogwheel::Input;
using namespace Cogwheel::Math;

namespace ImGui {

ImGuiAdaptor::ImGuiAdaptor()
    : m_enabled(true) {

    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();

    // io.ImeWindowHandle = hwnd;

    ImGui::StyleColorsDark();

    io.KeyMap[ImGuiKey_Tab] = int(Keyboard::Key::Tab);
    io.KeyMap[ImGuiKey_LeftArrow] = int(Keyboard::Key::Left);
    io.KeyMap[ImGuiKey_RightArrow] = int(Keyboard::Key::Right);
    io.KeyMap[ImGuiKey_UpArrow] = int(Keyboard::Key::Up);
    io.KeyMap[ImGuiKey_DownArrow] = int(Keyboard::Key::Down);
    io.KeyMap[ImGuiKey_PageUp] = int(Keyboard::Key::PageUp);
    io.KeyMap[ImGuiKey_PageDown] = int(Keyboard::Key::PageDown);
    io.KeyMap[ImGuiKey_Home] = int(Keyboard::Key::Home);
    io.KeyMap[ImGuiKey_End] = int(Keyboard::Key::End);
    io.KeyMap[ImGuiKey_Insert] = int(Keyboard::Key::Insert);
    io.KeyMap[ImGuiKey_Delete] = int(Keyboard::Key::Delete);
    io.KeyMap[ImGuiKey_Backspace] = int(Keyboard::Key::Backspace);
    io.KeyMap[ImGuiKey_Space] = int(Keyboard::Key::Space);
    io.KeyMap[ImGuiKey_Enter] = int(Keyboard::Key::Enter);
    io.KeyMap[ImGuiKey_Escape] = int(Keyboard::Key::Escape);
    io.KeyMap[ImGuiKey_A] = int(Keyboard::Key::A);
    io.KeyMap[ImGuiKey_C] = int(Keyboard::Key::C);
    io.KeyMap[ImGuiKey_V] = int(Keyboard::Key::V);
    io.KeyMap[ImGuiKey_X] = int(Keyboard::Key::X);
    io.KeyMap[ImGuiKey_Y] = int(Keyboard::Key::Y);
    io.KeyMap[ImGuiKey_Z] = int(Keyboard::Key::Z);
}

void ImGuiAdaptor::new_frame(const Cogwheel::Core::Engine& engine) {
    ImGuiIO& io = ImGui::GetIO();

    // Handle window.
    const Cogwheel::Core::Window& window = engine.get_window();
    Vector2i window_size = { window.get_width(), window.get_height() };
    io.DisplaySize = ImVec2((float)window_size.x, (float)window_size.y);

    io.DeltaTime = engine.get_time().get_smooth_delta_time();

    Mouse* mouse = (Mouse*)engine.get_mouse();
    Keyboard* keyboard = (Keyboard*)engine.get_keyboard();

    { // Handle mouse input.
        Vector2i mouse_pos = mouse->get_position();
        io.MousePos = { float(mouse_pos.x), float(mouse_pos.y) };

        for (int k = 0; k < 5; ++k)
            io.MouseDown[k] = mouse->is_pressed((Mouse::Button)k);

        io.MouseWheel += mouse->get_scroll_delta();
    }

    { // Handle keyboard
        // Modifiers
        io.KeyCtrl = keyboard->is_pressed(Keyboard::Key::LeftControl) || keyboard->is_pressed(Keyboard::Key::RightControl);
        io.KeyShift = keyboard->is_pressed(Keyboard::Key::LeftShift) || keyboard->is_pressed(Keyboard::Key::RightShift);
        io.KeyAlt = keyboard->is_pressed(Keyboard::Key::LeftAlt) || keyboard->is_pressed(Keyboard::Key::RightAlt);
        io.KeySuper = keyboard->is_pressed(Keyboard::Key::LeftSuper) || keyboard->is_pressed(Keyboard::Key::RightSuper);

        // Keymapped keys
        for (int k = 0; k < ImGuiKey_COUNT; ++k)
            io.KeysDown[io.KeyMap[k]] = keyboard->is_pressed(Keyboard::Key(io.KeyMap[k]));

        // Text.
        const std::wstring& text = keyboard->get_text();
        for (int i = 0; i < text.length(); ++i)
            io.AddInputCharacter(text[i]);
    }

    ImGui::NewFrame();

    if (m_enabled)
        for (auto& frame : m_frames)
            frame->layout_frame();

    if (io.WantCaptureMouse) {
        mouse->consume_all_button_events();
        mouse->consume_scroll_delta();
    }
    if (io.WantCaptureKeyboard)
        keyboard->consume_all_button_events();
}

// Adopted from ImGui::PlotEx(...)
inline void PlotMultiEx(ImGuiPlotType plot_type, const char* label, PlotData* plots, int plot_count, const char* overlay_text, float scale_min, float scale_max, ImVec2 graph_size) {
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;

    const ImVec2 label_size = CalcTextSize(label, NULL, true);
    if (graph_size.x == 0.0f)
        graph_size.x = CalcItemWidth();
    if (graph_size.y == 0.0f)
        graph_size.y = label_size.y + (style.FramePadding.y * 2);

    const ImRect frame_bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(graph_size.x, graph_size.y));
    const ImRect inner_bb(frame_bb.Min + style.FramePadding, frame_bb.Max - style.FramePadding);
    const ImRect total_bb(frame_bb.Min, frame_bb.Max + ImVec2(label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f, 0));
    ItemSize(total_bb, style.FramePadding.y);
    if (!ItemAdd(total_bb, 0, &frame_bb))
        return;

    // Determine scale from values if not specified
    if (scale_min == FLT_MAX || scale_max == FLT_MAX) {
        float v_min = FLT_MAX;
        float v_max = -FLT_MAX;
        for (int plot_index = 0; plot_index < plot_count; ++plot_index) {
            auto& plot = plots[plot_index];
            for (int i = 0; i < plot.value_count; ++i) {
                float v = plot.function(i);
                v_min = ImMin(v_min, v);
                v_max = ImMax(v_max, v);
            }
        }
        if (scale_min == FLT_MAX)
            scale_min = v_min;
        if (scale_max == FLT_MAX)
            scale_max = v_max;
    }

    RenderFrame(frame_bb.Min, frame_bb.Max, GetColorU32(ImGuiCol_FrameBg), true, style.FrameRounding);

    if (plots->value_count > 0) {
        float t_hovered = -1.0f;

        // Tooltip on hover
        const bool hovered = ItemHoverable(inner_bb, 0);
        if (hovered) {
            t_hovered = (g.IO.MousePos.x - inner_bb.Min.x) / (inner_bb.Max.x - inner_bb.Min.x);

            std::ostringstream tooltip;
            for (int plot_index = 0; plot_index < plot_count; ++plot_index) {
                auto& plot = plots[plot_index];

                tooltip << plot.label << ":\n";

                int item_count = plot.value_count + (plot_type == ImGuiPlotType_Lines ? -1 : 0);
                const int v_idx = (int)(t_hovered * item_count);
                IM_ASSERT(v_idx >= 0 && v_idx < item_count);

                float v0 = plot.function(v_idx);
                if (plot_type == ImGuiPlotType_Lines) {
                    float v1 = plot.function(v_idx + 1);
                    tooltip << "  (" << v_idx << ", " << v0 << ") -> (" << (v_idx + 1) << ", " << v1 << ")\n";
                }
                else if (plot_type == ImGuiPlotType_Histogram)
                    tooltip << v_idx << ": " << v0;
            }

            SetTooltip(tooltip.str().c_str());
        }

        const float inv_scale = (scale_min == scale_max) ? 0.0f : (1.0f / (scale_max - scale_min));

        for (int plot_index = 0; plot_index < plot_count; ++plot_index) {
            auto& plot = plots[plot_index];

            int res_w = ImMin((int)graph_size.x, plot.value_count) + (plot_type == ImGuiPlotType_Lines ? -1 : 0);
            int item_count = plot.value_count + (plot_type == ImGuiPlotType_Lines ? -1 : 0);
            const float t_step = 1.0f / (float)res_w;

            const int hovered_v_idx = (int)(t_hovered * item_count);

            float v0 = plot.function(0);
            float t0 = 0.0f;
            ImVec2 tp0 = ImVec2(t0, 1.0f - ImSaturate((v0 - scale_min) * inv_scale));                       // Point in the normalized space of our target rectangle
            float histogram_zero_line_t = (scale_min * scale_max < 0.0f) ? (-scale_min * inv_scale) : (scale_min < 0.0f ? 0.0f : 1.0f);   // Where does the zero line stands

            const ImU32 col_base = plot.color;
            const ImU32 col_hovered = GetColorU32(plot_type == ImGuiPlotType_Lines ? ImGuiCol_PlotLinesHovered : ImGuiCol_PlotHistogramHovered);

            for (int n = 0; n < res_w; ++n) {
                const float t1 = t0 + t_step;
                const int v1_idx = (int)(t0 * item_count + 0.5f);
                IM_ASSERT(v1_idx >= 0 && v1_idx < item_count);
                const float v1 = plot.function(v1_idx + 1);
                const ImVec2 tp1 = ImVec2(t1, 1.0f - ImSaturate((v1 - scale_min) * inv_scale));

                // NB: Draw calls are merged together by the DrawList system. Still, we should render our batch are lower level to save a bit of CPU.
                ImVec2 pos0 = ImLerp(inner_bb.Min, inner_bb.Max, tp0);
                ImVec2 pos1 = ImLerp(inner_bb.Min, inner_bb.Max, plot_type == ImGuiPlotType_Lines ? tp1 : ImVec2(tp1.x, histogram_zero_line_t));
                if (plot_type == ImGuiPlotType_Lines)
                {
                    window->DrawList->AddLine(pos0, pos1, hovered_v_idx == v1_idx ? col_hovered : col_base);
                }
                else if (plot_type == ImGuiPlotType_Histogram)
                {
                    if (pos1.x >= pos0.x + 2.0f)
                        pos1.x -= 1.0f;
                    window->DrawList->AddRectFilled(pos0, pos1, hovered_v_idx == v1_idx ? col_hovered : col_base);
                }

                t0 = t1;
                tp0 = tp1;
            }
        }
    }

    // Text overlay
    if (overlay_text)
        RenderTextClipped(ImVec2(frame_bb.Min.x, frame_bb.Min.y + style.FramePadding.y), frame_bb.Max, overlay_text, NULL, NULL, ImVec2(0.5f, 0.0f));

    if (label_size.x > 0.0f)
        RenderText(ImVec2(frame_bb.Max.x + style.ItemInnerSpacing.x, inner_bb.Min.y), label);
}

void PlotLines(const char* label, PlotData* plots, int plot_count, const char* overlay_text, float scale_min, float scale_max, ImVec2 graph_size) {
    PlotMultiEx(ImGuiPlotType_Lines, label, plots, plot_count, overlay_text, scale_min, scale_max, graph_size);
}

void PlotHistograms(const char* label, PlotData* plots, int plot_count, const char* overlay_text, float scale_min, float scale_max, ImVec2 graph_size) {
    PlotMultiEx(ImGuiPlotType_Histogram, label, plots, plot_count, overlay_text, scale_min, scale_max, graph_size);
}

} // NS ImGui
