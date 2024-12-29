//
// Provide classes and methods to render a series of overlay and gui controls using raylib and raygui
//

#ifndef PXLSOVERLAY_H
#define PXLSOVERLAY_H
#include <chrono>
#include <array>
#include <optional>
#include <format>
#include "raylib.h"
#include "raygui.h"
#include "PxlsCanvas.h"
#include "PxlsLogDB.h"

class PxlsDialog {
public:
    // show TextInputBox, return false if it is occupied by others
    static bool TextInputBox(unsigned window_w, unsigned window_h, unsigned dialog_token, const std::string &title,
        const std::string &message, std::string &input_text);
    // is dialog open
    static bool IsDialogOpen() { return current_token.has_value(); }
    static const std::optional<unsigned>& CurrentToken() { return current_token; }
private:
    // current dialog token, used for preventing opening multiple dialogs
    static std::optional<unsigned> current_token;
    static std::array<char, 256> input_buf;
    // the dimension of text input box
    const static Vector2 TEXT_INPUT_BOX_DIMENSION;
    // the background color when the dialog shows
    const static Color DIALOG_BACKGROUND_COLOR;
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
    unsigned window_width = 0, window_height = 0;
    // the dimension of normal panel
    const static Vector2 NORMAL_DIMENSION;
    // the dimension of expanded panel
    const static Vector2 EXPAND_DIMENSION;
    // the margin and padding of panel
    const static float LEFT_MARGIN;
    const static float BOTTOM_MARGIN;
    const static float PADDING;
};

class PxlsCursorOverlay {
public:
    // render info panel using raylib and raygui
    static void Render(const PxlsCanvas &canvas);
private:
    // overlay dimension and offset relative to mouse cursor
    const static Rectangle OVERLAY_OFFSET;
};

enum PlaybackState { PLAY, PAUSE };
using playback_callback = std::function<void (unsigned pb_head)>;

class PxlsPlaybackPanel {
public:
    PxlsPlaybackPanel(unsigned window_w, unsigned window_h);
    // render playback panel using raylib and raygui and perform playback operation
    void Render(PxlsLogDB &db, PxlsCanvas &canvas);
private:
    // input dialogs
    bool PlaybackSpeedInputDialog(std::string &playback_speed_str) const {
        return PxlsDialog::TextInputBox(window_width, window_height, 0, "Set playback speed",
            "Input playback speed(-10000 - 10000 rec/f):", playback_speed_str);
    }
    bool PlaybackHeadInputDialog(std::string &playback_head_str, const PxlsLogDB &db) const {
        return PxlsDialog::TextInputBox(window_width, window_height, 1, "Set playback head",
            std::format("Input playback head(0 - {}):", db.RecordCount()), playback_head_str);
    }
    // update canvas according to playback head
    static void UpdateCanvas(unsigned pb_head, PxlsLogDB &db, PxlsCanvas &canvas);
    // window dimension
    unsigned window_width = 0, window_height = 0;
    // playback state
    PlaybackState playback_state = PAUSE;
    // playback head
    unsigned long playback_head = 0;
    // playback speed
    int playback_speed = 100;
    // playback panel height
    const static float PANEL_HEIGHT;
    // progress label minimum width
    const static float HEAD_LABEL_MIN_WIDTH;
    // playback speed label minimum width
    const static float SPEED_LABEL_MIN_WIDTH;
    // playback button width
    const static float PLAYBACK_BTN_WIDTH;
    // the margin of panel
    const static float MARGIN;
    // the horizontal gap of controls
    const static float CONTROL_GAP;
};

#endif //PXLSOVERLAY_H
