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
        new_palette.push_back(PxlsCanvasColor { name, palette_color });
    }
    palette = new_palette;
    return true;
}

bool PxlsCanvas::InitCanvas(unsigned canvas_w, unsigned canvas_h, unsigned window_w, unsigned window_h) {
    if (canvas_w == 0 || canvas_h == 0 || window_w == 0 || window_h == 0) return false;
    canvas = std::vector(canvas_w, std::vector(canvas_h, PxlsCanvasPixel()));
    canvas_width = canvas_w; canvas_height = canvas_h; window_width = window_w; window_height = window_h;
    view_center = Vector2 { canvas_width / 2.0f, canvas_height / 2.0f };
    return true;
}

Color PxlsCanvas::GetPaletteColor(unsigned color_index) const {
    if (color_index >= palette.size()) return FALLBACK_PIXEL_COLOR;
    return palette[color_index].color;
}

std::string PxlsCanvas::GetPaletteColorName(unsigned color_index) const {
    if (color_index >= palette.size()) return "<fallback>";
    return palette[color_index].name;
}

bool PxlsCanvas::PerformAction(unsigned x, unsigned y, std::string time_str, std::string action, std::string hash,
    unsigned color_index, ActionDirection direction) {
    if (x >= canvas_width || y >= canvas_height) return false;
    auto &px = canvas[x][y];
    sys_time_ms last_time;
    std::istringstream { time_str } >> date::parse("%F %T", last_time);
    unsigned new_manipulation_count = canvas[x][y].manipulate_count;
    if (direction == REDO)
        new_manipulation_count++;
    else
        new_manipulation_count = std::min(new_manipulation_count - 1, new_manipulation_count);
    canvas[x][y] = PxlsCanvasPixel { new_manipulation_count, last_time, action, hash, color_index };
    return true;
}

void PxlsCanvas::ViewCenter(Vector2 center) {
    center.x = std::clamp(center.x, 0.0f, static_cast<float>(canvas_width));
    center.y = std::clamp(center.y, 0.0f, static_cast<float>(canvas_height));
    view_center = center;
}

void PxlsCanvas::Scale(float s) {
    scale = std::clamp(s, MIN_SCALE, MAX_SCALE);
}

bool PxlsCanvas::Highlight(unsigned x, unsigned y) {
    if (x >= canvas_width || y >= canvas_height) return false;
    do_highlight = true;
    highlight_x = x; highlight_y = y;
}

void PxlsCanvas::DeHighlight() {
    do_highlight = false;
    highlight_x = highlight_y = 0;
}

bool PxlsCanvas::GetNearestPixelPos(Vector2 window_pos, unsigned &canvas_x, unsigned &canvas_y) const {
    if (window_pos.x > window_width || window_pos.y > window_height || window_pos.x < 0 || window_pos.y < 0)
        return false;
    // calc the position of the center pixel in the canvas
    unsigned canvas_view_center_x = std::min(std::floorf(view_center.x), canvas_width - 1.0f);
    unsigned canvas_view_center_y = std::min(std::floorf(view_center.y), canvas_height - 1.0f);
    // calc the position of the upper left of the center pixel in the window
    Vector2 window_view_center = {
        window_width / 2 - (view_center.x - canvas_view_center_x) * scale,
        window_height / 2 - (view_center.y - canvas_view_center_y) * scale
    };
    // pos offset relative to the upper left of the center pixel in the window
    Vector2 window_pos_offset = {
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

void PxlsCanvas::Render() const {
    ClearBackground(BACKGROUND_COLOR);
    // calc the position of the center pixel in the canvas
    unsigned canvas_view_center_x = std::min(std::floorf(view_center.x), canvas_width - 1.0f);
    unsigned canvas_view_center_y = std::min(std::floorf(view_center.y), canvas_height - 1.0f);
    // calc the position of the upper left corner of the center pixel in the window
    Vector2 window_view_center = {
        window_width / 2 - (view_center.x - canvas_view_center_x) * scale,
        window_height / 2 - (view_center.y - canvas_view_center_y) * scale
    };
    // view rect offset relative to center in all directions
    unsigned canvas_offset_left = std::ceilf(window_view_center.x / scale);
    unsigned canvas_offset_top = std::ceilf(window_view_center.y / scale);
    unsigned canvas_offset_right = std::floorf((window_width - window_view_center.x) / scale);
    unsigned canvas_offset_bottom = std::floorf((window_height - window_view_center.y) / scale);
    // the position of the upper left corner pixel in the canvas view
    unsigned canvas_view_origin_x, canvas_view_origin_y;
    // the dimension of the canvas view
    unsigned canvas_view_width, canvas_view_height;
    if (canvas_offset_left > canvas_view_center_x)
        canvas_view_origin_x = 0;
    else
        canvas_view_origin_x = canvas_view_center_x - canvas_offset_left;
    if (canvas_offset_top > canvas_view_center_y)
        canvas_view_origin_y = 0;
    else
        canvas_view_origin_y = canvas_view_center_y - canvas_offset_top;
    canvas_view_width = canvas_view_center_x - canvas_view_origin_x +
        std::min(canvas_width - 1 - canvas_view_center_x, canvas_offset_right) + 1;
    canvas_view_height = canvas_view_center_y - canvas_view_origin_y +
        std::min(canvas_height - 1 - canvas_view_center_y, canvas_offset_bottom) + 1;
    Vector2 window_view_origin = {
        window_view_center.x - (canvas_view_center_x - canvas_view_origin_x) * scale,
        window_view_center.y - (canvas_view_center_y - canvas_view_origin_y) * scale
    };
    for (unsigned x = 0; x < canvas_view_width; x++) {
        for (unsigned y = 0; y < canvas_view_height; y++) {
            // optimization for 1.0f scale
            if (scale == 1.0f) {
                DrawPixelV(Vector2 { window_view_origin.x + x, window_view_origin.y + y },
                    GetPaletteColor(canvas[canvas_view_origin_x + x][canvas_view_origin_y + y].color_index));
            } else {
                DrawRectangleRec(Rectangle {
                        window_view_origin.x + x * scale, window_view_origin.y + y * scale,
                        scale, scale
                    }, GetPaletteColor(canvas[canvas_view_origin_x + x][canvas_view_origin_y + y].color_index));
                if (do_highlight && highlight_x == canvas_view_origin_x + x && highlight_y == canvas_view_origin_y + y) {
                    DrawRectangleLinesEx(Rectangle {
                        window_view_origin.x + x * scale, window_view_origin.y + y * scale,
                        scale, scale
                    }, 1.5f, BLACK);
                }
            }
        }
    }
}
