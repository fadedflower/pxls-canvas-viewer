//
// Provide classes and methods to render a pxls canvas on the screen using raylib
//

#ifndef PXLSCANVAS_H
#define PXLSCANVAS_H
#include <vector>
#include <filesystem>
#include <fstream>
#include <chrono>
#include <sstream>
#include "raylib.h"
#include "nlohmann/json.hpp"
#include "date/date.h"
using json = nlohmann::ordered_json;
using sys_time_ms = std::chrono::sys_time<std::chrono::milliseconds>;
using hh_mm_ss = std::chrono::hh_mm_ss<std::chrono::milliseconds>;

struct PxlsCanvasColor {
    std::string name;
    Color color { WHITE };
};

struct PxlsCanvasPixel {
    // number of action performed on the pixel, also used to judge if it is a virgin pixel
    unsigned manipulate_count { 0 };
    // last action time
    sys_time_ms last_time {};
    // last action name
    std::string last_action = "none";
    // last action hash, used to distinguish one user's pixels from others
    std::string last_hash = "<empty>";
    // color index in the palette
    unsigned color_index { 0 };
};

enum ActionDirection { FORWARD, BACKWARD };

class PxlsCanvas {
public:
    // load palette from a palette json
    bool LoadPaletteFromJson(const std::string &filename);
    // init canvas with specified dimension
    bool InitCanvas(unsigned canvas_w, unsigned canvas_h, unsigned window_w, unsigned window_h);
    // get readonly access to palette
    const auto& Palette() { return palette; }
    // get readonly access to canvas
    const auto& Canvas() { return canvas; }
    // get palette color by color index
    Color GetPaletteColor(unsigned color_index) const;
    // perform action on the specified pixel, either forward or backward
    bool PerformAction(unsigned x, unsigned y, std::string time_str, std::string action, std::string hash,
        unsigned color_index, ActionDirection direction);
    // get/set canvas view
    void ViewCenter(Vector2 center);
    const Vector2& ViewCenter() const { return view_center; }
    void Scale(float s);
    float Scale() const { return scale; }
    // render canvas using raylib
    void Render();
    // background color of the canvas
    const Color BACKGROUND_COLOR { 0xC5, 0xC5, 0xC5 };
    // pixel color used when the palette is empty or the color index is out of range
    const Color FALLBACK_PIXEL_COLOR { WHITE };
    // scale limit
    const float MAX_SCALE = 50.0f;
    const float MIN_SCALE = 1.0f;
private:
    // palette
    std::vector<PxlsCanvasColor> palette;
    // canvas
    std::vector<std::vector<PxlsCanvasPixel>> canvas;
    // canvas dimension
    unsigned canvas_width { 0 }, canvas_height { 0 };
    // window dimension
    unsigned window_width { 0 }, window_height { 0 };
    // center position of the canvas view in the canvas
    Vector2 view_center { 0.0, 0.0 };
    // scale of the view, it is actually the width of pixel
    float scale { 1.0f };
};

#endif //PXLSCANVAS_H
