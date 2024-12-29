#include <filesystem>
#include "raylib.h"
#include "raygui.h"
#include "PxlsLogDB.h"
#include "PxlsCanvas.h"
#include "PxlsOverlay.h"

constexpr unsigned SCREEN_WIDTH = 1280;
constexpr unsigned SCREEN_HEIGHT = 960;
constexpr unsigned char OVERLAY_ALPHA = 204;

int main() {
    PxlsLogDB db;
    PxlsCanvas canvas;
    PxlsInfoPanel info_panel(SCREEN_WIDTH, SCREEN_HEIGHT);
    PxlsPlaybackPanel playback_panel(SCREEN_WIDTH, SCREEN_HEIGHT);

    db.OpenLogDB("../log_test/pixels.sanit.logdb");
    canvas.InitCanvas(db.Width(), db.Height(), SCREEN_WIDTH, SCREEN_HEIGHT);
    canvas.LoadPaletteFromJson("../log_test/palette.json");

    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Pxls Canvas Viewer");
    SetTargetFPS(60);
    // load style if it exists
    if (std::filesystem::exists("style.rgs") && !std::filesystem::is_directory("style.rgs"))
        GuiLoadStyle("style.rgs");
    // patch background color to introduce transparency
    const auto gui_background_color = Fade(GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)), 0.8);
    GuiSetStyle(DEFAULT, BACKGROUND_COLOR, ColorToInt(gui_background_color));

    while (!WindowShouldClose())
    {
        BeginDrawing();
        // render canvas only if the canvas is not updating
        if (playback_panel.IsCanvasUpdating())
            ClearBackground(PxlsCanvas::BACKGROUND_COLOR);
        else
            canvas.Render();
        // render overlay
        PxlsCursorOverlay::Render(canvas);
        info_panel.Render(canvas);
        playback_panel.Render(db, canvas);
        EndDrawing();
    }
    CloseWindow();
    return 0;
}
