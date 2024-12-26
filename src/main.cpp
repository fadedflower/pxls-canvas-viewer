#include "raylib.h"
#include "PxlsLogDB.h"
#include "PxlsCanvas.h"

constexpr int SCREEN_WIDTH = 1024;
constexpr int SCREEN_HEIGHT = 768;

int main() {
    PxlsLogDB db;
    db.OpenLogDB("../log_test/pixels.sanit.logdb");
    db.QueryDimension([](const unsigned w, const unsigned h) {std::cout << w << 'x' << h << std::endl;});

    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "raylib [core] example - basic window");
    SetTargetFPS(60);

    PxlsCanvas canvas;
    canvas.LoadPaletteFromJson("../log_test/palette.json");
    while (!WindowShouldClose())
    {
        BeginDrawing();
        canvas.Render();
        EndDrawing();
    }
    CloseWindow();
    return 0;
}
