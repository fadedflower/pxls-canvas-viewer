//
// PxlsCanvas implementation
//

#include "PxlsCanvas.h"

const Color PxlsCanvas::BACKGROUND_COLOR { 0xC5, 0xC5, 0xC5 };
const Color PxlsCanvas::FALLBACK_PIXEL_COLOR { WHITE };
const float PxlsCanvas::MAX_SCALE = 50.0f;
const float PxlsCanvas::MIN_SCALE = 1.0f;

bool PxlsCanvas::LoadPaletteFromJson(const std::string &filename) {
    if (!std::filesystem::exists(filename) || std::filesystem::is_directory(filename)) return false;
    std::ifstream palette_file(filename);
    json palette_json = json::parse(palette_file, nullptr, false, true);
    // check invalid json
    if (palette_json.is_discarded() || !palette_json.is_object()) return false;
    /*
     * palette format
     * {
     *     "White": "#FFFFFF",
     *     ...
     * }
     */
    std::vector<PxlsCanvasColor> new_palette;
    for (const auto &[name, color] : palette_json.items()) {
        if (!color.is_string()) return false;
        auto color_hex = color.get<std::string>();
        if (color_hex.size() != 7 || color_hex[0] != '#') return false;
        Color palette_color;
        try {
            palette_color.r = std::stoi(color_hex.substr(1, 2), nullptr, 16);
            palette_color.g = std::stoi(color_hex.substr(3, 2), nullptr, 16);
            palette_color.b = std::stoi(color_hex.substr(5, 2), nullptr, 16);
        }
        catch (std::invalid_argument&) { return false; }
        palette_color.a = 255;
        new_palette.push_back({ name, palette_color });
    }
    palette = new_palette;
    return true;
}

bool PxlsCanvas::InitCanvas(const unsigned canvas_w, const unsigned canvas_h, const unsigned window_w, const unsigned window_h) {
    if (canvas_w == 0 || canvas_h == 0 || window_w == 0 || window_h == 0) return false;
    canvas_width = canvas_w; canvas_height = canvas_h; window_width = window_w; window_height = window_h;
    view_center = { canvas_width / 2.0f, canvas_height / 2.0f };
    ClearCanvas();
    return true;
}

void PxlsCanvas::ClearCanvas() {
    canvas = std::vector(canvas_width, std::vector(canvas_height, PxlsCanvasPixel()));
}

Color PxlsCanvas::GetPaletteColor(const unsigned color_index) const {
    if (color_index >= palette.size()) return FALLBACK_PIXEL_COLOR;
    return palette[color_index].color;
}

std::string PxlsCanvas::GetPaletteColorName(const unsigned color_index) const {
    if (color_index >= palette.size()) return "<fallback>";
    return palette[color_index].name;
}

bool PxlsCanvas::PerformAction(const unsigned x, const unsigned y, const ActionDirection direction, std::optional<std::string> time_str,
                const std::optional<std::string> &action, const std::optional<std::string> &hash, const std::optional<unsigned> &color_index) {
    if (x >= canvas_width || y >= canvas_height) return false;
    sys_time_ms last_time;
    std::istringstream { time_str ? *time_str : "1972-01-01 00:00:00.000" } >> date::parse("%F %T", last_time);
    unsigned new_manipulate_count = canvas[x][y].manipulate_count;
    if (direction == REDO) {
        new_manipulate_count++;
    }
    else {
        if (new_manipulate_count != 0)
            new_manipulate_count--;
        // revert to virgin pixel
        if (new_manipulate_count == 0) {
            canvas[x][y] = PxlsCanvasPixel();
            return true;
        }
    }
    canvas[x][y] = { new_manipulate_count, last_time, *action, *hash, *color_index };
    return true;
}

void PxlsCanvas::ViewCenter(Vector2 center) {
    center.x = std::clamp(center.x, 0.0f, static_cast<float>(canvas_width));
    center.y = std::clamp(center.y, 0.0f, static_cast<float>(canvas_height));
    view_center = center;
}

void PxlsCanvas::Scale(const float s) {
    scale = std::clamp(s, MIN_SCALE, MAX_SCALE);
}

bool PxlsCanvas::Highlight(const unsigned x, const unsigned y) {
    if (x >= canvas_width || y >= canvas_height) return false;
    do_highlight = true;
    highlight_x = x; highlight_y = y;
    return true;
}

void PxlsCanvas::DeHighlight() {
    do_highlight = false;
    highlight_x = highlight_y = 0;
}

bool PxlsCanvas::GetNearestPixelPos(const Vector2 window_pos, unsigned &canvas_x, unsigned &canvas_y) const {
    if (window_pos.x > window_width || window_pos.y > window_height || window_pos.x < 0 || window_pos.y < 0)
        return false;
    // calc the position of the center pixel in the canvas
    const unsigned canvas_view_center_x = std::min(std::floorf(view_center.x), canvas_width - 1.0f);
    const unsigned canvas_view_center_y = std::min(std::floorf(view_center.y), canvas_height - 1.0f);
    // calc the position of the upper left of the center pixel in the window
    const Vector2 window_view_center = {
        window_width / 2 - (view_center.x - canvas_view_center_x) * scale,
        window_height / 2 - (view_center.y - canvas_view_center_y) * scale
    };
    // pos offset relative to the upper left of the center pixel in the window
    const Vector2 window_pos_offset = {
        window_pos.x - window_view_center.x,
        window_pos.y - window_view_center.y
    };
    // check if out of range
    if (window_pos_offset.x < 0 && -std::floorf(window_pos_offset.x / scale) > canvas_view_center_x) return false;
    if (window_pos_offset.y < 0 && -std::floorf(window_pos_offset.y / scale) > canvas_view_center_y) return false;
    if (window_pos_offset.x > 0 && canvas_view_center_x + std::floorf(window_pos_offset.x / scale) >= canvas_width)
        return false;
    if (window_pos_offset.y > 0 && canvas_view_center_y + std::floorf(window_pos_offset.y / scale) >= canvas_height)
        return false;
    canvas_x = canvas_view_center_x + std::floorf(window_pos_offset.x / scale);
    canvas_y = canvas_view_center_y + std::floorf(window_pos_offset.y / scale);
    return true;
}

void PxlsCanvas::Render(){
    // respond to mouse input
    Scale(Scale() + GetMouseWheelMove() / 3.0f);
    // right click or middle click to move view
    if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT) || IsMouseButtonDown(MOUSE_BUTTON_MIDDLE)) {
        SetMouseCursor(MOUSE_CURSOR_RESIZE_ALL);
        auto mouse_move_delta = GetMouseDelta();
        mouse_move_delta = { mouse_move_delta.x / Scale(), mouse_move_delta.y / Scale()};
        const auto [view_center_x, view_center_y] = ViewCenter();
        ViewCenter(Vector2 { view_center_x - mouse_move_delta.x, view_center_y - mouse_move_delta.y });
    }
    else {
        SetMouseCursor(MOUSE_CURSOR_DEFAULT);
    }
    // highlight selected pixel
    unsigned highlight_x, highlight_y;
    if (GetNearestPixelPos(GetMousePosition(), highlight_x, highlight_y))
        Highlight(highlight_x, highlight_y);
    else
        DeHighlight();
    // do the actual rendering
    ClearBackground(BACKGROUND_COLOR);
    // calc the position of the center pixel in the canvas
    const unsigned canvas_view_center_x = std::min(std::floorf(view_center.x), canvas_width - 1.0f);
    const unsigned canvas_view_center_y = std::min(std::floorf(view_center.y), canvas_height - 1.0f);
    // calc the position of the upper left corner of the center pixel in the window
    const Vector2 window_view_center = {
        window_width / 2 - (view_center.x - canvas_view_center_x) * scale,
        window_height / 2 - (view_center.y - canvas_view_center_y) * scale
    };
    // view rect offset relative to center in all directions
    const unsigned canvas_offset_left = std::ceilf(window_view_center.x / scale);
    const unsigned canvas_offset_top = std::ceilf(window_view_center.y / scale);
    const unsigned canvas_offset_right = std::floorf((window_width - window_view_center.x) / scale);
    const unsigned canvas_offset_bottom = std::floorf((window_height - window_view_center.y) / scale);
    // the position of the upper left corner pixel in the canvas view
    unsigned canvas_view_origin_x, canvas_view_origin_y;
    // the dimension of the canvas view
    if (canvas_offset_left > canvas_view_center_x)
        canvas_view_origin_x = 0;
    else
        canvas_view_origin_x = canvas_view_center_x - canvas_offset_left;
    if (canvas_offset_top > canvas_view_center_y)
        canvas_view_origin_y = 0;
    else
        canvas_view_origin_y = canvas_view_center_y - canvas_offset_top;
    const unsigned canvas_view_width = canvas_view_center_x - canvas_view_origin_x +
                                 std::min(canvas_width - 1 - canvas_view_center_x, canvas_offset_right) + 1;
    const unsigned canvas_view_height = canvas_view_center_y - canvas_view_origin_y +
                                  std::min(canvas_height - 1 - canvas_view_center_y, canvas_offset_bottom) + 1;
    const Vector2 window_view_origin = {
        window_view_center.x - (canvas_view_center_x - canvas_view_origin_x) * scale,
        window_view_center.y - (canvas_view_center_y - canvas_view_origin_y) * scale
    };
    for (unsigned x = 0; x < canvas_view_width; x++) {
        for (unsigned y = 0; y < canvas_view_height; y++) {
            // optimization for 1.0f scale
            if (scale == 1.0f) {
                DrawPixelV({ window_view_origin.x + x, window_view_origin.y + y },
                    GetPaletteColor(canvas[canvas_view_origin_x + x][canvas_view_origin_y + y].color_index));
            } else {
                DrawRectangleRec({
                        window_view_origin.x + x * scale, window_view_origin.y + y * scale,
                        scale, scale
                    }, GetPaletteColor(canvas[canvas_view_origin_x + x][canvas_view_origin_y + y].color_index));
                if (do_highlight && highlight_x == canvas_view_origin_x + x && highlight_y == canvas_view_origin_y + y) {
                    DrawRectangleLinesEx({
                        window_view_origin.x + x * scale, window_view_origin.y + y * scale,
                        scale, scale
                    }, 1.5f, BLACK);
                }
            }
        }
    }
}
