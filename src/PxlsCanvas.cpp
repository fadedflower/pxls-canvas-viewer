//
// PxlsCanvas implementation
//

#include "PxlsCanvas.h"

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

bool PxlsCanvas::InitCanvas(unsigned width, unsigned height) {
    if (width == 0 || height == 0) return false;
    canvas = std::vector(width, std::vector(height, PxlsCanvasPixel()));
    canvas_width = width; canvas_height = height;
    return true;
}

Color PxlsCanvas::GetPaletteColor(unsigned color_index) const {
    if (color_index >= palette.size()) return FALLBACK_PIXEL_COLOR;
    return palette[color_index].color;
}

bool PxlsCanvas::PerformAction(unsigned x, unsigned y, std::string time_str, std::string action, std::string hash,
    unsigned color_index, ActionDirection direction) {
    if (x >= canvas_width || y >= canvas_height) return false;
    auto &px = canvas[x][y];
    sys_time_ms last_time;
    std::istringstream { time_str } >> date::parse("%F &T", last_time);
    unsigned new_manipulation_count = canvas[x][y].manipulate_count;
    if (direction == FORWARD)
        new_manipulation_count++;
    else
        new_manipulation_count = std::min(new_manipulation_count - 1, new_manipulation_count);
    canvas[x][y] = PxlsCanvasPixel { new_manipulation_count, last_time, action, hash, color_index };
    return true;
}

void PxlsCanvas::Render() {
    ClearBackground(BACKGROUND_COLOR);
}
