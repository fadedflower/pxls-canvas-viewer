//
// Provide classes and methods to render a series of overlay and gui controls using raylib and raygui
//

#ifndef PXLSOVERLAY_H
#define PXLSOVERLAY_H
#include <chrono>
#include "raylib.h"
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"
#include "PxlsCanvas.h"
#include "PxlsLogDB.h"

//===========================PxlsInfoPanel===========================

class PxlsInfoPanel {
public:
    PxlsInfoPanel(unsigned window_w, unsigned window_h);
    // render info panel using raylib and raygui
    void Render(const PxlsCanvas &canvas, const PxlsLogDB &db);
private:
    // is panel expanded
    bool is_expanded = false;
    // window dimension
    unsigned window_width = 0, window_height = 0;
    // the dimension of normal panel
    const static Vector2 NORMAL_DIMENSION;
    // the dimension of expanded panel
    const static Vector2 EXPAND_DIMENSION;
    // the margin and padding of panel
    const static float MARGIN;
    const static float PADDING;
};

const Vector2 PxlsInfoPanel::NORMAL_DIMENSION = { 180.0f, 75.0f };
const Vector2 PxlsInfoPanel::EXPAND_DIMENSION = { 460.0f, 155.0f };
const float PxlsInfoPanel::MARGIN = 20.0f;
const float PxlsInfoPanel::PADDING = 5.0f;

inline PxlsInfoPanel::PxlsInfoPanel(unsigned window_w, unsigned window_h) {
    window_width = window_w; window_height = window_h;
}

void PxlsInfoPanel::Render(const PxlsCanvas &canvas, const PxlsLogDB &db) {
    Rectangle panel_rect;
    unsigned control_line_index = 0;
    // generate bound rect for the next control
    auto NextControlBounds = [&] {
        return Rectangle {
            panel_rect.x + PADDING,
            panel_rect.y + PADDING + 16 * control_line_index++,
            panel_rect.width - 2 * PADDING,
            15 };
    };
    if (is_expanded)
        panel_rect = { MARGIN, static_cast<float>(window_height) - EXPAND_DIMENSION.y - MARGIN,
            EXPAND_DIMENSION.x, EXPAND_DIMENSION.y };
    else
        panel_rect = { MARGIN, static_cast<float>(window_height) - NORMAL_DIMENSION.y - MARGIN,
            NORMAL_DIMENSION.x, NORMAL_DIMENSION.y };
    // render panel gui
    GuiPanel(panel_rect, nullptr);
    unsigned canvas_x, canvas_y;
    if (canvas.GetNearestPixelPos(GetMousePosition(), canvas_x, canvas_y)) {
        auto color = canvas.GetPaletteColor(canvas.Canvas()[canvas_x][canvas_y].color_index);
        GuiLabel(NextControlBounds(), std::format("({}, {})", canvas_x, canvas_y).c_str());
        GuiLabel(NextControlBounds(), std::format("{} (#{:02X}{:02X}{:02X})",
            canvas.GetPaletteColorName(canvas.Canvas()[canvas_x][canvas_y].color_index), color.r, color.g, color.b).c_str());
        GuiLabel(NextControlBounds(), std::format("{} / {}", db.Seek(), db.RecordCount()).c_str());
        if (is_expanded) {
            if (canvas.Canvas()[canvas_x][canvas_y].manipulate_count == 0) {
                GuiLabel(NextControlBounds(), "This is a virgin pixel");
            } else {
                GuiLabel(NextControlBounds(), std::format("Total action count: {}",
                    canvas.Canvas()[canvas_x][canvas_y].manipulate_count).c_str());
                GuiLabel(NextControlBounds(), std::format("Last action type: {}",
                canvas.Canvas()[canvas_x][canvas_y].last_action).c_str());
                GuiLabel(NextControlBounds(), std::format("Last action time: {:%F %T}",
                    canvas.Canvas()[canvas_x][canvas_y].last_time).c_str());
                GuiLabel(NextControlBounds(), "Last record hash:");
                GuiLabel(NextControlBounds(), canvas.Canvas()[canvas_x][canvas_y].last_hash.c_str());
            }
        }
    } else {
        GuiLabel(NextControlBounds(), std::format("{} / {}", db.Seek(), db.RecordCount()).c_str());
    }
    if (is_expanded) {
        control_line_index = 8;
        if (GuiLabelButton(NextControlBounds(), GuiIconText(ICON_ARROW_DOWN_FILL, "Collapse")))
            is_expanded = !is_expanded;
    } else {
        control_line_index = 3;
        if (GuiLabelButton(NextControlBounds(), GuiIconText(ICON_ARROW_UP_FILL, "Expand")))
            is_expanded = !is_expanded;
    }
}

//===========================PxlsCursorOverlay===========================

class PxlsCursorOverlay {
public:
    // render info panel using raylib and raygui
    static void Render(const PxlsCanvas &canvas, const PxlsLogDB &db);
private:
    // overlay dimension and offset relative to mouse cursor
    const static Rectangle OVERLAY_OFFSET;
};

const Rectangle PxlsCursorOverlay::OVERLAY_OFFSET = { 15, 20, 30, 30 };

void PxlsCursorOverlay::Render(const PxlsCanvas &canvas, const PxlsLogDB &db) {
    auto mouse_pos = GetMousePosition();
    unsigned canvas_x, canvas_y;
    if (canvas.GetNearestPixelPos(mouse_pos, canvas_x, canvas_y)) {
        DrawRectangleRounded(Rectangle {
            mouse_pos.x + OVERLAY_OFFSET.x,
            mouse_pos.y + OVERLAY_OFFSET.y,
            OVERLAY_OFFSET.width,
            OVERLAY_OFFSET.height }, 0.2f, 20,
            canvas.GetPaletteColor(canvas.Canvas()[canvas_x][canvas_y].color_index));
        DrawRectangleRoundedLinesEx(Rectangle {
            mouse_pos.x + OVERLAY_OFFSET.x,
            mouse_pos.y + OVERLAY_OFFSET.y,
            OVERLAY_OFFSET.width,
            OVERLAY_OFFSET.height }, 0.3f, 10, 1.5f, BLACK);
    }
}

#endif //PXLSOVERLAY_H
