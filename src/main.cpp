#include <filesystem>
#include "raylib.h"
#include "raygui.h"
#include "PxlsLogDB.h"
#include "PxlsCanvas.h"
#include "PxlsOverlay.h"
#include "../../../../../Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX15.2.sdk/System/Library/Frameworks/CoreGraphics.framework/Headers/CGPDFDictionary.h"

constexpr unsigned SCREEN_WIDTH = 1024;
constexpr unsigned SCREEN_HEIGHT = 768;
constexpr unsigned char OVERLAY_ALPHA = 204;

PxlsLogDB db;
PxlsCanvas canvas;
PxlsInfoPanel info_panel(SCREEN_WIDTH, SCREEN_HEIGHT);
bool canvas_move_mode = false;

int main() {
    db.OpenLogDB("../log_test/pixels.sanit.logdb");
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Pxls Canvas Viewer");
    SetTargetFPS(60);
    // load style if it exists
    if (std::filesystem::exists("style.rgs") && !std::filesystem::is_directory("style.rgs"))
        GuiLoadStyle("style.rgs");
    // patch background color to introduce transparency
    auto gui_background_color = GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR));
    gui_background_color.a = OVERLAY_ALPHA;
    GuiSetStyle(DEFAULT, BACKGROUND_COLOR, ColorToInt(gui_background_color));

    canvas.InitCanvas(db.Width(), db.Height(), SCREEN_WIDTH, SCREEN_HEIGHT);
    canvas.LoadPaletteFromJson("../log_test/palette.json");
    db.QueryRecords(20000,
        [](std::string date, std::string hash, unsigned x, unsigned y,
        unsigned color_index, std::string action, QueryDirection direction) {
            if (direction == FORWARD)
                canvas.PerformAction(x, y, date, action, hash, color_index, REDO);
            else
                canvas.PerformAction(x, y, date, action, hash, color_index, UNDO);
    });
    while (!WindowShouldClose())
    {
        BeginDrawing();
        // respond to mouse input and render canvas
        canvas.Scale(canvas.Scale() + GetMouseWheelMove() / 3.0f);
        if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
            SetMouseCursor(MOUSE_CURSOR_RESIZE_ALL);
            auto mouse_move_delta = GetMouseDelta();
            mouse_move_delta = { mouse_move_delta.x / canvas.Scale(), mouse_move_delta.y / canvas.Scale()};
            auto view_center = canvas.ViewCenter();
            canvas.ViewCenter(Vector2 { view_center.x - mouse_move_delta.x, view_center.y - mouse_move_delta.y });
        }
        else {
            SetMouseCursor(MOUSE_CURSOR_DEFAULT);
        }
        // highlight selected pixel
        unsigned highlight_x, highlight_y;
        if (canvas.GetNearestPixelPos(GetMousePosition(), highlight_x, highlight_y))
            canvas.Highlight(highlight_x, highlight_y);
        else
            canvas.DeHighlight();
        canvas.Render();
        // render overlay
        PxlsCursorOverlay::Render(canvas, db);
        info_panel.Render(canvas, db);

        EndDrawing();
        // if (db.Seek() < db.RecordCount()) {
        //     db.QueryRecords(std::min(db.Seek() + 100, db.RecordCount()),
        //         [](std::string date, std::string hash, unsigned x, unsigned y,
        //         unsigned color_index, std::string action, QueryDirection direction) {
        //             if (direction == FORWARD)
        //                 canvas.PerformAction(x, y, date, action, hash, color_index, REDO);
        //             else
        //                 canvas.PerformAction(x, y, date, action, hash, color_index, UNDO);
        //     });
        // }
    }
    CloseWindow();
    return 0;
}
