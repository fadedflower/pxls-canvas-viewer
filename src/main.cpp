#include "raylib.h"
#include "PxlsLogDB.h"
#include "PxlsCanvas.h"

constexpr unsigned SCREEN_WIDTH = 1024;
constexpr unsigned SCREEN_HEIGHT = 768;

PxlsLogDB db;
PxlsCanvas canvas;
bool canvas_move_mode = false;

int main() {
    db.OpenLogDB("../log_test/pixels.sanit.logdb");
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Pxls Canvas Viewer");
    SetTargetFPS(60);

    canvas.InitCanvas(db.Width(), db.Height(), SCREEN_WIDTH, SCREEN_HEIGHT);
    canvas.LoadPaletteFromJson("../log_test/palette.json");
    while (!WindowShouldClose())
    {
        BeginDrawing();
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
        canvas.Render();
        if (db.Seek() < db.RecordCount()) {
            db.QueryRecords(std::min(db.Seek() + 100, db.RecordCount()),
                [](std::string date, std::string hash, unsigned x, unsigned y,
                unsigned color_index, std::string action, int direction) {
                    if (direction == 1)
                        canvas.PerformAction(x, y, date, action, hash, color_index, FORWARD);
                    else
                        canvas.PerformAction(x, y, date, action, hash, color_index, BACKWARD);
            });
        }
        EndDrawing();
    }
    CloseWindow();
    return 0;
}
