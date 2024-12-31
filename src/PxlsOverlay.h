//
// Provide classes and methods to render a series of overlay and gui controls using raylib and raygui
//

#ifndef PXLSOVERLAY_H
#define PXLSOVERLAY_H
#include <string>
#include <chrono>
#include <array>
#include <vector>
#include <optional>
#include <format>
#include <future>
#include <mutex>
#include "raylib.h"
#include "raygui.h"
#include "PxlsCanvas.h"
#include "PxlsLogDB.h"

class PxlsDialog {
public:
    // acquire/release dialog token
    static bool AcquireToken(unsigned dialog_token);
    static bool ReleaseToken(unsigned dialog_token);
    // show TextInputBox, acquire a dialog token before calling it
    static bool TextInputBox(unsigned window_w, unsigned window_h, unsigned dialog_token, const std::string &title,
                             const std::string &message, std::string &input_text, int &button_result);
    // show PendingBox, acquire a dialog token before calling it
    static bool PendingBox(unsigned window_w, unsigned window_h, unsigned dialog_token, const std::string &message);
    // show MessageBox, acquire a dialog token before calling it
    static bool MessageBox(unsigned window_w, unsigned window_h, unsigned dialog_token, const std::string &title,
                            const std::string &message, int &button_result);
    // is a dialog open
    static bool IsDialogOpen() { return current_token.has_value(); }
    static const std::optional<unsigned>& CurrentToken() { return current_token; }
private:
    // check if a dialog token is in use
    static bool CheckToken(const unsigned dialog_token) { return current_token && current_token == dialog_token; }
    // draw dialog background
    static void DrawDialogBackground(const unsigned window_w, const unsigned window_h) {
        DrawRectangleRec({ 0.0f, 0.0f, static_cast<float>(window_w), static_cast<float>(window_h)}, DIALOG_BACKGROUND_COLOR);
    }
    // current dialog token, used for preventing opening multiple dialogs
    static std::optional<unsigned> current_token;
    static std::array<char, 256> input_buf;
    // the dimension of text input box
    static constexpr Vector2 TEXT_INPUT_BOX_DIMENSION { 350.0f, 150.0f };
    // the height of pending box
    static constexpr float PENDING_BOX_HEIGHT { 50.0f };
    // the left and right padding of pending box
    static constexpr float PENDING_BOX_PADDING { 15.0f };
    // the height of message box
    static constexpr float MESSAGE_BOX_HEIGHT { 100.0f };
    // the left and right padding of message box
    static constexpr float MESSAGE_BOX_PADDING { 30.0f };
    // the background color when the dialog shows
    static constexpr Color DIALOG_BACKGROUND_COLOR { 0, 0, 0, 127 };
};

class PxlsInfoPanel {
public:
    PxlsInfoPanel(unsigned window_w, unsigned window_h);
    // render info panel using raylib and raygui
    void Render(const PxlsCanvas &canvas);
private:
    // is panel expanded
    bool is_expanded = false;
    // window dimension
    unsigned window_width { 0 }, window_height { 0 };
    // the dimension of normal panel
    static constexpr Vector2 NORMAL_DIMENSION { 190.0f, 60.0f };
    // the dimension of expanded panel
    static constexpr Vector2 EXPAND_DIMENSION { 460.0f, 140.0f };
    // the margin and padding of panel
    static constexpr float LEFT_MARGIN { 10.0f };
    static constexpr float BOTTOM_MARGIN { 40.0f };
    static constexpr float PADDING { 5.0f };
};

class PxlsCursorOverlay {
public:
    // render info panel using raylib and raygui
    static void Render(const PxlsCanvas &canvas);
private:
    // overlay dimension and offset relative to mouse cursor
    static constexpr Rectangle OVERLAY_OFFSET { 15, 20, 30, 30 };
};

enum PlaybackState { PLAY, PAUSE };
using PlaybackCallback = std::function<void (unsigned pb_head)>;

class PxlsPlaybackPanel {
public:
    PxlsPlaybackPanel(unsigned window_w, unsigned window_h);
    // render playback panel using raylib and raygui and perform playback operation
    void Render(PxlsLogDB &db, PxlsCanvas &canvas);
    // check if the canvas is updating by checking canvas_future
    [[nodiscard]] bool IsCanvasUpdating() const {
        return canvas_future.valid() &&
            canvas_future.wait_for(std::chrono::seconds(0)) == std::future_status::timeout;
    }
    void ResetPlayback() {
        playback_state = PAUSE; playback_head = 0; playback_speed = 100;
    }
    // dialog tokens
    static constexpr unsigned PLAYBACK_SPEED_TOKEN { 0 };
    static constexpr unsigned PLAYBACK_HEAD_TOKEN { 1 };
    static constexpr unsigned CANVAS_FUTURE_TOKEN { 2 };
private:
    // update canvas according to playback head
    void UpdateCanvas(unsigned pb_head, PxlsLogDB &db, PxlsCanvas &canvas);
    // window dimension
    unsigned window_width { 0 }, window_height { 0 };
    // playback state
    PlaybackState playback_state { PAUSE };
    // playback head
    unsigned long playback_head { 0 };
    // playback speed
    int playback_speed { 100 };
    // the future of updating canvas
    std::future<void> canvas_future;
    // progress shown while updating canvas
    unsigned long update_progress { 0 };
    unsigned long update_progress_total { 0 };
    std::mutex progress_mutex;
    // playback panel height
    static constexpr float PANEL_HEIGHT { 23.0f };
    // progress label minimum width
    static constexpr float HEAD_LABEL_MIN_WIDTH { 120.0f };
    // playback speed label minimum width
    static constexpr float SPEED_LABEL_MIN_WIDTH { 70.0f };
    // playback button width
    static constexpr float PLAYBACK_BTN_WIDTH { 15.0f };
    // the margin of panel
    static constexpr float MARGIN { 10.0f };
    // the horizontal gap of controls
    static constexpr float CONTROL_GAP { 5.0f };
    // the threshold of absolute id difference that should be reached before using async method
    static constexpr unsigned ASYNC_PROCESS_THRESHOLD { 70000 };
};

struct ToolbarItem {
    std::string button_text;
    std::optional<std::string> button_tooltip { std::nullopt };
    std::string command;
    bool disabled { false };
    bool pressed { false };
};
using ToolbarCallback = std::function<void (const std::string &command)>;

class PxlsToolbar {
public:
    // render toolbar using raylib and execute toolbar commands via callback
    static void Render(const std::vector<ToolbarItem> &items, const ToolbarCallback &callback);
private:
    // toolbar button width
    static constexpr float BTN_WIDTH { 30.0f };
    static constexpr float BTN_GAP { 5.0f };
    // toolbar margin
    static constexpr float MARGIN { 10.0f };
};

#endif //PXLSOVERLAY_H
