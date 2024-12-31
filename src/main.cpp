#include <vector>
#include <array>
#include <string>
#include <filesystem>
#include <future>
#include "raylib.h"
#include "raygui.h"
#include "PxlsLogDB.h"
#include "PxlsCanvas.h"
#include "PxlsOverlay.h"
#include "tinyfiledialogs.h"

constexpr unsigned SCREEN_WIDTH = 1280;
constexpr unsigned SCREEN_HEIGHT = 960;
constexpr unsigned char OVERLAY_ALPHA = 204;
constexpr unsigned RAW_LOG_FUTURE_TOKEN = { 3 };
constexpr unsigned LOGDB_FUTURE_TOKEN = { 4 };
constexpr unsigned LOAD_FAILURE_TOKEN = { 5 };
constexpr std::array<std::string, 2> required_files { "style.rgs", "palette.json" };
constexpr std::array log_filter_pattern { "*.log", "*.logdb" };
std::vector<ToolbarItem> toolbar_items {
    { GuiIconText(ICON_FILE_OPEN, nullptr), "Load a Pxls log or LogDB", "OPEN"},
    {GuiIconText(ICON_FILE_DELETE,nullptr), "Close LogDB", "CLOSE"},
    { GuiIconText(ICON_FILETYPE_PLAY, nullptr), "Toggle playback panel", "TOGGLE_PLAYBACK", false, true},
    { GuiIconText(ICON_INFO, nullptr), "Toggle info panel", "TOGGLE_INFO", false, true},
    { GuiIconText(ICON_CURSOR_POINTER, nullptr), "Toggle cursor overlay", "TOGGLE_CURSOR_OVERLAY", false, true},
    { GuiIconText(ICON_EXIT, nullptr), "Exit program", "EXIT"}
};
std::future<void> raw_log_future, logdb_future;

inline bool is_future_pending(const std::future<void> &future) {
    return future.valid() && future.wait_for(std::chrono::seconds(0)) == std::future_status::timeout;
}

inline bool is_log_loading() {
    return is_future_pending(raw_log_future) || is_future_pending(logdb_future);
}

int main() {
    // check if required files exist before running
    for (const auto& file: required_files) {
        if (!std::filesystem::exists(file) || std::filesystem::is_directory(file)) {
            tinyfd_messageBox(
                "Missing file",
                std::format("{} is missing. Please reinstall the program.", file).c_str(),
                "ok",
                "error",
                1);
            return 1;
        }
    }
    PxlsLogDB db;
    PxlsCanvas canvas;
    PxlsInfoPanel info_panel(SCREEN_WIDTH, SCREEN_HEIGHT);
    PxlsPlaybackPanel playback_panel(SCREEN_WIDTH, SCREEN_HEIGHT);
    bool exit_flag = false;

    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Pxls Canvas Viewer");
    SetTargetFPS(60);
    // load style file and palette
    GuiLoadStyle("style.rgs");
    canvas.LoadPaletteFromJson("palette.json");
    // patch background color to introduce transparency
    const auto gui_background_color = Fade(GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)), 0.8);
    GuiSetStyle(DEFAULT, BACKGROUND_COLOR, ColorToInt(gui_background_color));

    while (!WindowShouldClose())
    {
        BeginDrawing();
        // render canvas only if LogDB is open and the canvas is not updating
        if (db.IsOpen() && !is_log_loading() && !playback_panel.IsCanvasUpdating())
            canvas.Render();
        else
            ClearBackground(PxlsCanvas::BACKGROUND_COLOR);
        // render overlay and gui
        if (db.IsOpen() && !is_log_loading() && toolbar_items[4].pressed)
            PxlsCursorOverlay::Render(canvas);
        // update toolbar state
        toolbar_items[1].disabled = toolbar_items[2].disabled = toolbar_items[3].disabled = toolbar_items[4].disabled = !db.IsOpen();
        PxlsToolbar::Render(toolbar_items, [&](const std::string &command) {
            if (command == "OPEN") {
                // show open file dialog
                const auto file_path_raw = tinyfd_openFileDialog(
                    "Choose a Pxls log or LogDB",
                    nullptr,
                    2,
                    log_filter_pattern.data(),
                    "Pxls log / LogDB files",
                    0);
                if (file_path_raw && std::filesystem::exists(file_path_raw) && !std::filesystem::is_directory(file_path_raw)) {
                    const std::string file_path { file_path_raw };
                    const auto ext = std::filesystem::path { file_path }.extension().string();
                    if (ext == ".log") {
                        PxlsDialog::AcquireToken(RAW_LOG_FUTURE_TOKEN);
                        raw_log_future = std::async([&, file_path] {
                            if (db.OpenLogRaw(file_path)) {
                                canvas.InitCanvas(db.Width(), db.Height(), SCREEN_WIDTH, SCREEN_HEIGHT);
                                playback_panel.ResetPlayback();
                                PxlsDialog::ReleaseToken(RAW_LOG_FUTURE_TOKEN);
                            } else {
                                PxlsDialog::ReleaseToken(RAW_LOG_FUTURE_TOKEN);
                                PxlsDialog::AcquireToken(LOAD_FAILURE_TOKEN);
                            }

                        });
                    } else {
                        PxlsDialog::AcquireToken(LOGDB_FUTURE_TOKEN);
                        logdb_future = std::async([&, file_path] {
                            if (db.OpenLogDB(file_path)) {
                                canvas.InitCanvas(db.Width(), db.Height(), SCREEN_WIDTH, SCREEN_HEIGHT);
                                playback_panel.ResetPlayback();
                                PxlsDialog::ReleaseToken(LOGDB_FUTURE_TOKEN);
                            } else {
                                PxlsDialog::ReleaseToken(LOGDB_FUTURE_TOKEN);
                                PxlsDialog::AcquireToken(LOAD_FAILURE_TOKEN);
                            }
                        });
                    }
                }
            }
            else if (command == "CLOSE")
                db.CloseLogDB();
            else if (command == "TOGGLE_PLAYBACK")
                toolbar_items[2].pressed = !toolbar_items[2].pressed;
            else if (command == "TOGGLE_INFO")
                toolbar_items[3].pressed = !toolbar_items[3].pressed;
            else if (command == "TOGGLE_CURSOR_OVERLAY")
                toolbar_items[4].pressed = !toolbar_items[4].pressed;
            else if (command == "EXIT")
                exit_flag = true;
        });
        if (db.IsOpen() && !is_log_loading()) {
            if (toolbar_items[3].pressed)
                info_panel.Render(canvas);
            if (toolbar_items[2].pressed)
                playback_panel.Render(db, canvas);
        }
        // render pending box
        PxlsDialog::PendingBox(SCREEN_WIDTH, SCREEN_HEIGHT, RAW_LOG_FUTURE_TOKEN,
            "Building LogDB... It will take some time, depending on the log size.");
        PxlsDialog::PendingBox(SCREEN_WIDTH, SCREEN_HEIGHT, LOGDB_FUTURE_TOKEN,
            "Loading LogDB...");
        // render message box
        if (int button_result;
            PxlsDialog::MessageBox(SCREEN_WIDTH, SCREEN_HEIGHT, LOAD_FAILURE_TOKEN,
            "Load failed", "Failed to load Pxls log / LogDB. Please ensure the file is not corrupted.", button_result) &&
            button_result != -1) {
            PxlsDialog::ReleaseToken(LOAD_FAILURE_TOKEN);
        }
        EndDrawing();
        if (exit_flag)
            break;
    }
    CloseWindow();
    return 0;
}
