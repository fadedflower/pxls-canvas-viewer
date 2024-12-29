//
// PxlsOverlay implementation
//

// define the single raygui implementation here
#define RAYGUI_IMPLEMENTATION
#include "PxlsOverlay.h"

//===========================PxlsDialog===========================
std::optional<unsigned> PxlsDialog::current_token { std::nullopt };
std::array<char, 256> PxlsDialog::input_buf {};
const Vector2 PxlsDialog::TEXT_INPUT_BOX_DIMENSION { 350.0f, 150.0f };
const Color PxlsDialog::DIALOG_BACKGROUND_COLOR = Fade(BLACK, 0.6f);

bool PxlsDialog::TextInputBox(const unsigned window_w, const unsigned window_h, unsigned dialog_token, const std::string &title,
        const std::string &message, std::string &input_text) {
    if (current_token && current_token != dialog_token) return false;
    if (!current_token) {
        // acquire token and init input_buf
        current_token = dialog_token;
        input_buf.fill('\0');
    }
    DrawRectangleRec({ 0.0f, 0.0f, static_cast<float>(window_w), static_cast<float>(window_h)}, DIALOG_BACKGROUND_COLOR);
    const auto button_result = GuiTextInputBox({
        window_w / 2 - TEXT_INPUT_BOX_DIMENSION.x / 2,
        window_h / 2 - TEXT_INPUT_BOX_DIMENSION.y / 2,
        TEXT_INPUT_BOX_DIMENSION.x,
        TEXT_INPUT_BOX_DIMENSION.y},
        title.c_str(), message.c_str(), "OK;Cancel", input_buf.data(), 255, nullptr);
    if (button_result == 1)
        input_text = input_buf.data();
    if (button_result != -1) {
        // release token
        current_token = std::nullopt;
    }
    return true;
}

//===========================PxlsInfoPanel===========================
const Vector2 PxlsInfoPanel::NORMAL_DIMENSION { 190.0f, 60.0f };
const Vector2 PxlsInfoPanel::EXPAND_DIMENSION { 460.0f, 140.0f };
const float PxlsInfoPanel::LEFT_MARGIN = 10.0f;
const float PxlsInfoPanel::BOTTOM_MARGIN = 40.0f;
const float PxlsInfoPanel::PADDING = 5.0f;

PxlsInfoPanel::PxlsInfoPanel(const unsigned window_w, const unsigned window_h) {
    window_width = window_w; window_height = window_h;
}

void PxlsInfoPanel::Render(const PxlsCanvas &canvas) {
    Rectangle panel_rect;
    unsigned control_line_index = 0;
    // generate bound rect for the next control
    auto NextControlBounds = [&] {
        return Rectangle {
            panel_rect.x + PADDING,
            panel_rect.y + PADDING + 16 * control_line_index++,
            panel_rect.width - 2 * PADDING,
            15 };
    };
    if (is_expanded)
        panel_rect = { LEFT_MARGIN, static_cast<float>(window_height) - EXPAND_DIMENSION.y - BOTTOM_MARGIN,
            EXPAND_DIMENSION.x, EXPAND_DIMENSION.y };
    else
        panel_rect = { LEFT_MARGIN, static_cast<float>(window_height) - NORMAL_DIMENSION.y - BOTTOM_MARGIN,
            NORMAL_DIMENSION.x, NORMAL_DIMENSION.y };
    // render panel gui
    GuiPanel(panel_rect, nullptr);
    unsigned canvas_x, canvas_y;
    if (canvas.GetNearestPixelPos(GetMousePosition(), canvas_x, canvas_y)) {
        auto color = canvas.GetPaletteColor(canvas.Canvas()[canvas_x][canvas_y].color_index);
        // pixel position
        GuiLabel(NextControlBounds(), std::format("({}, {})", canvas_x, canvas_y).c_str());
        // pixel color
        GuiLabel(NextControlBounds(), std::format("{} ({}, #{:02X}{:02X}{:02X})",
            canvas.GetPaletteColorName(canvas.Canvas()[canvas_x][canvas_y].color_index),
            canvas.Canvas()[canvas_x][canvas_y].color_index, color.r, color.g, color.b).c_str());
        if (is_expanded) {
            if (canvas.Canvas()[canvas_x][canvas_y].manipulate_count == 0) {
                GuiLabel(NextControlBounds(), "Virgin pixel");
            } else {
                // pixel detail
                GuiLabel(NextControlBounds(), std::format("Total action count: {}",
                    canvas.Canvas()[canvas_x][canvas_y].manipulate_count).c_str());
                GuiLabel(NextControlBounds(), std::format("Last action type: {}",
                canvas.Canvas()[canvas_x][canvas_y].last_action).c_str());
                GuiLabel(NextControlBounds(), std::format("Last action time: {:%F %T}",
                    canvas.Canvas()[canvas_x][canvas_y].last_time).c_str());
                GuiLabel(NextControlBounds(), "Last record hash:");
                GuiLabel(NextControlBounds(), canvas.Canvas()[canvas_x][canvas_y].last_hash.c_str());
            }
        }
    }
    if (is_expanded) {
        control_line_index = 7;
        if (GuiLabelButton(NextControlBounds(), GuiIconText(ICON_ARROW_DOWN, "Less details")) && !PxlsDialog::IsDialogOpen())
            is_expanded = !is_expanded;
    } else {
        control_line_index = 2;
        if (GuiLabelButton(NextControlBounds(), GuiIconText(ICON_ARROW_UP, "More details")) && !PxlsDialog::IsDialogOpen())
            is_expanded = !is_expanded;
    }
}

//===========================PxlsCursorOverlay===========================
const Rectangle PxlsCursorOverlay::OVERLAY_OFFSET { 15, 20, 30, 30 };

void PxlsCursorOverlay::Render(const PxlsCanvas &canvas) {
    const auto mouse_pos = GetMousePosition();
    unsigned canvas_x, canvas_y;
    if (canvas.GetNearestPixelPos(mouse_pos, canvas_x, canvas_y)) {
        DrawRectangleRounded(Rectangle {
            mouse_pos.x + OVERLAY_OFFSET.x,
            mouse_pos.y + OVERLAY_OFFSET.y,
            OVERLAY_OFFSET.width,
            OVERLAY_OFFSET.height }, 0.2f, 20,
            canvas.GetPaletteColor(canvas.Canvas()[canvas_x][canvas_y].color_index));
        DrawRectangleRoundedLinesEx(Rectangle {
            mouse_pos.x + OVERLAY_OFFSET.x,
            mouse_pos.y + OVERLAY_OFFSET.y,
            OVERLAY_OFFSET.width,
            OVERLAY_OFFSET.height }, 0.3f, 10, 1.5f, BLACK);
    }
}

//===========================PxlsPlaybackPanel===========================
const float PxlsPlaybackPanel::PANEL_HEIGHT = 23.0f;
const float PxlsPlaybackPanel::HEAD_LABEL_MIN_WIDTH = 120.0f;
const float PxlsPlaybackPanel::SPEED_LABEL_MIN_WIDTH = 70.0f;
const float PxlsPlaybackPanel::PLAYBACK_BTN_WIDTH = 15.0f;
const float PxlsPlaybackPanel::MARGIN = 10.0f;
const float PxlsPlaybackPanel::CONTROL_GAP = 5.0f;

PxlsPlaybackPanel::PxlsPlaybackPanel(const unsigned window_w, const unsigned window_h) {
    window_width = window_w; window_height = window_h;
}

void PxlsPlaybackPanel::Render(PxlsLogDB &db, PxlsCanvas &canvas) {
    const Rectangle progress_panel_rect = { MARGIN,
        static_cast<float>(window_height) - PANEL_HEIGHT - MARGIN,
        window_width - 2 * MARGIN, PANEL_HEIGHT };
    auto control_x = progress_panel_rect.x + CONTROL_GAP;
    // generate bound rect for the next control
    auto NextControlBounds = [&](const float width) {
        const Rectangle next_rect = { control_x, progress_panel_rect.y + 4, width, 15 };
        control_x += width + CONTROL_GAP;
        return next_rect;
    };
    // render panel gui
    GuiPanel(progress_panel_rect, nullptr);
    // jump to beginning button
    if (GuiLabelButton(NextControlBounds(PLAYBACK_BTN_WIDTH), GuiIconText(ICON_PLAYER_PREVIOUS, nullptr)) && !PxlsDialog::IsDialogOpen())
        playback_head = 0;
    // playback control button
    if (playback_state == PLAY &&
        (GuiLabelButton(NextControlBounds(PLAYBACK_BTN_WIDTH), GuiIconText(ICON_PLAYER_PAUSE, nullptr)) || IsKeyPressed(KEY_SPACE))
        && !PxlsDialog::IsDialogOpen())
        playback_state = PAUSE;
    else if (playback_state == PAUSE &&
        (GuiLabelButton(NextControlBounds(PLAYBACK_BTN_WIDTH), GuiIconText(ICON_PLAYER_PLAY, nullptr)) || IsKeyPressed(KEY_SPACE))
        && !PxlsDialog::IsDialogOpen())
        playback_state = PLAY;
    // jump to end button
    if (GuiLabelButton(NextControlBounds(PLAYBACK_BTN_WIDTH), GuiIconText(ICON_PLAYER_NEXT, nullptr)) && !PxlsDialog::IsDialogOpen())
        playback_head = db.RecordCount();
    // playback speed label
    const auto speed_label_str = std::format("{} rec/f", playback_speed);
    std::string speed_value_str;
    bool show_playback_speed_dialog = false;
    if (GuiLabelButton(NextControlBounds(std::max(SPEED_LABEL_MIN_WIDTH, static_cast<float>(GetTextWidth(speed_label_str.c_str())))),
        speed_label_str.c_str())) {
        // try to open the dialog
        PlaybackSpeedInputDialog(speed_value_str);
    } else if (PxlsDialog::CurrentToken() == 0)
        show_playback_speed_dialog = true;
    // head label
    const auto head_label_str = std::format("{} / {}", db.Seek(), db.RecordCount());
    std::string head_value_str;
    bool show_playback_head_dialog = false;
    if (GuiLabelButton(NextControlBounds(std::max(HEAD_LABEL_MIN_WIDTH, static_cast<float>(GetTextWidth(head_label_str.c_str())))),
        head_label_str.c_str())) {
        // try to open the dialog
        PlaybackHeadInputDialog(head_value_str, db);
    } else if (PxlsDialog::CurrentToken() == 1)
        show_playback_head_dialog = true;
    // progress bar
    float playback_head_raw = playback_head;
    GuiSliderBar(NextControlBounds(progress_panel_rect.width - control_x),
        nullptr, nullptr, &playback_head_raw, 0.0f, db.RecordCount());
    if (!PxlsDialog::IsDialogOpen())
        playback_head = std::floorf(playback_head_raw);
    if (show_playback_speed_dialog) {
        // render the dialog as long as the dialog is open
        PlaybackSpeedInputDialog(speed_value_str);
        if (!speed_value_str.empty()) {
            try {
                if (const auto pb_speed = std::stoi(speed_value_str); pb_speed != 0)
                    playback_speed = std::clamp(pb_speed, -10000, 10000);
            } catch (std::invalid_argument&) {}
        }
    }
    if (show_playback_head_dialog) {
        // render the dialog as long as the dialog is open
        PlaybackHeadInputDialog(head_value_str, db);
        if (!head_value_str.empty()) {
            try {
                playback_head = std::clamp(std::stoul(head_value_str), 0ul, db.RecordCount());
            } catch (std::invalid_argument&) {}
        }
    }

    // do playback and update canvas
    if (playback_head != db.Seek())
        UpdateCanvas(playback_head, db, canvas);
    if (playback_state == PLAY) {
        // loop playback
        if (db.Seek() == db.RecordCount() && playback_speed > 0)
            UpdateCanvas(0, db, canvas);
        else if (db.Seek() == 0 && playback_speed < 0)
            UpdateCanvas(db.Seek() == db.RecordCount(), db, canvas);
        else
            UpdateCanvas(std::clamp(static_cast<long long>(db.Seek()) + playback_speed, 0ll, static_cast<long long>(db.RecordCount())), db, canvas);
        // pause when the playback head reaches the end
        if ((db.Seek() == 0 && playback_speed < 0) || (db.Seek() == db.RecordCount() && playback_speed > 0))
            playback_state = PAUSE;
    }
    playback_head = db.Seek();
}

void PxlsPlaybackPanel::UpdateCanvas(const unsigned pb_head, PxlsLogDB &db, PxlsCanvas &canvas) {
    // optimization for jumping back to beginning
    if (pb_head == 0) {
        canvas.ClearCanvas();
        db.Seek(0);
    } else {
        db.QueryRecords(pb_head, [&](const std::optional<std::string> &date, const std::optional<std::string> &hash,
            const unsigned x, const unsigned y, const std::optional<unsigned> color_index, const std::optional<std::string> &action, const QueryDirection direction) {
            if (direction == FORWARD) {
                canvas.PerformAction(x, y, REDO, date, action, hash, color_index);
            } else {
                canvas.PerformAction(x, y, UNDO, date, action, hash, color_index);
            }
        });
    }
}
