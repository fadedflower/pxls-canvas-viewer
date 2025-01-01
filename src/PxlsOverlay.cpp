//
// PxlsOverlay implementation
//

// define the single raygui implementation here
#define RAYGUI_IMPLEMENTATION
#include "PxlsOverlay.h"

//===========================PxlsDialog===========================
std::optional<unsigned> PxlsDialog::current_token { std::nullopt };
std::array<char, 256> PxlsDialog::input_buf {};

bool PxlsDialog::AcquireToken(const unsigned dialog_token) {
    if (current_token) return false;
    input_buf.fill('\0');
    current_token = dialog_token;
    return true;
}

bool PxlsDialog::ReleaseToken(const unsigned dialog_token) {
    if (!CheckToken(dialog_token)) return false;
    // release token
    current_token = std::nullopt;
    return true;
}

bool PxlsDialog::TextInputBox(const unsigned window_w, const unsigned window_h, const unsigned dialog_token, const std::string &title,
                              const std::string &message, std::string &input_text, int &button_result) {
    if (!CheckToken(dialog_token)) return false;
    DrawDialogBackground(window_w, window_h);
    button_result = GuiTextInputBox({
        (static_cast<float>(window_w) - TEXT_INPUT_BOX_DIMENSION.x) / 2,
        (static_cast<float>(window_h) - TEXT_INPUT_BOX_DIMENSION.y) / 2,
        TEXT_INPUT_BOX_DIMENSION.x,
        TEXT_INPUT_BOX_DIMENSION.y},
        title.c_str(), message.c_str(), "OK;Cancel", input_buf.data(), 255, nullptr);
    if (button_result == 1)
        input_text = input_buf.data();
    return true;
}

bool PxlsDialog::PendingBox(const unsigned window_w, const unsigned window_h, const unsigned dialog_token, const std::string &message) {
    if (!CheckToken(dialog_token)) return false;
    const auto pending_box_width = GetTextWidth(message.c_str()) + 2 * PENDING_BOX_PADDING;
    const Rectangle panel_rect {
        static_cast<float>(window_w) / 2 - pending_box_width / 2,
        static_cast<float>(window_h) / 2 - PENDING_BOX_HEIGHT / 2,
        pending_box_width, PENDING_BOX_HEIGHT};
    DrawDialogBackground(window_w, window_h);
    GuiPanel(panel_rect, nullptr);
    GuiLabel({
        panel_rect.x + PENDING_BOX_PADDING,
        panel_rect.y + (panel_rect.height - 15) / 2,
        static_cast<float>(GetTextWidth(message.c_str())),
        15 }, message.c_str());
    return true;
}

bool PxlsDialog::MessageBox(const unsigned window_w, const unsigned window_h, const unsigned dialog_token, const std::string &title,
                            const std::string &message, int &button_result) {
    if (!CheckToken(dialog_token)) return false;
    const auto message_box_width = GetTextWidth(message.c_str()) + 2 * MESSAGE_BOX_PADDING;
    DrawDialogBackground(window_w, window_h);
    button_result = GuiMessageBox({
        static_cast<float>(window_w) / 2 - message_box_width / 2,
        static_cast<float>(window_h) / 2 - MESSAGE_BOX_HEIGHT / 2,
        message_box_width, MESSAGE_BOX_HEIGHT
    }, title.c_str(), message.c_str(), "OK");
    return true;
}

//===========================PxlsInfoPanel===========================
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
            panel_rect.y + PADDING + 16.0f * static_cast<float>(control_line_index++),
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
    if (PxlsDialog::CurrentToken() == PxlsPlaybackPanel::CANVAS_FUTURE_TOKEN) {
        GuiLabel(NextControlBounds(), "Pending changes");
        return;
    }
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
    // disable expand/collapse button when a dialog is open
    GuiSetState(PxlsDialog::IsDialogOpen() ? STATE_DISABLED : STATE_NORMAL);
    if (is_expanded) {
        control_line_index = 7;
        if (GuiLabelButton(NextControlBounds(), GuiIconText(ICON_ARROW_DOWN, "Less details")))
            is_expanded = !is_expanded;
    } else {
        control_line_index = 2;
        if (GuiLabelButton(NextControlBounds(), GuiIconText(ICON_ARROW_UP, "More details")))
            is_expanded = !is_expanded;
    }
    GuiSetState(STATE_NORMAL);
}

//===========================PxlsCursorOverlay===========================
void PxlsCursorOverlay::Render(const PxlsCanvas &canvas) {
    if (PxlsDialog::CurrentToken() == PxlsPlaybackPanel::CANVAS_FUTURE_TOKEN) return;
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
PxlsPlaybackPanel::PxlsPlaybackPanel(const unsigned window_w, const unsigned window_h) {
    window_width = window_w; window_height = window_h;
}

bool PxlsPlaybackPanel::InitPlayback(const PxlsLogDB &db) {
    if (IsCanvasUpdating()) return false;
    playback_state = PAUSE; playback_head = 0; playback_speed = 100;
    db.QuerySnapshotIdList(snapshot_ids);
    return true;
}


void PxlsPlaybackPanel::Render(PxlsLogDB &db, PxlsCanvas &canvas) {
    const Rectangle progress_panel_rect = { MARGIN,
        static_cast<float>(window_height) - PANEL_HEIGHT - MARGIN,
        static_cast<float>(window_width) - 2 * MARGIN, PANEL_HEIGHT };
    auto control_x = progress_panel_rect.x + CONTROL_GAP;
    // generate bound rect for the next control
    auto NextControlBounds = [&](const float width) {
        const Rectangle next_rect = { control_x, progress_panel_rect.y + 4, width, 15 };
        control_x += width + CONTROL_GAP;
        return next_rect;
    };

    // render panel gui
    GuiPanel(progress_panel_rect, nullptr);
    // disable playback control when a dialog is open
    GuiSetState(PxlsDialog::IsDialogOpen() ? STATE_DISABLED : STATE_NORMAL);
    // jump to beginning button
    if (GuiLabelButton(NextControlBounds(PLAYBACK_BTN_WIDTH), GuiIconText(ICON_PLAYER_PREVIOUS, nullptr)))
        playback_head = 0;
    // playback control button
    if (playback_state == PLAY && !PxlsDialog::IsDialogOpen() &&
        (GuiLabelButton(NextControlBounds(PLAYBACK_BTN_WIDTH), GuiIconText(ICON_PLAYER_PAUSE, nullptr)) || IsKeyPressed(KEY_SPACE)))
        playback_state = PAUSE;
    else if (playback_state == PAUSE && !PxlsDialog::IsDialogOpen() &&
        (GuiLabelButton(NextControlBounds(PLAYBACK_BTN_WIDTH), GuiIconText(ICON_PLAYER_PLAY, nullptr)) || IsKeyPressed(KEY_SPACE)))
        playback_state = PLAY;
    // jump to end button
    if (GuiLabelButton(NextControlBounds(PLAYBACK_BTN_WIDTH), GuiIconText(ICON_PLAYER_NEXT, nullptr)))
        playback_head = db.RecordCount();

    int button_result;
    // playback speed label
    const auto speed_label_str = std::format("{} rec/f", playback_speed);
    if (GuiLabelButton(NextControlBounds(std::max(SPEED_LABEL_MIN_WIDTH, static_cast<float>(GetTextWidth(speed_label_str.c_str())))),
        speed_label_str.c_str()) && PxlsDialog::CurrentToken() != PLAYBACK_SPEED_TOKEN) {
        // try to acquire the dialog token
        PxlsDialog::AcquireToken(PLAYBACK_SPEED_TOKEN);
    }

    // head label
    const auto head_label_str = std::format("{} / {}", db.Seek(), db.RecordCount());
    if (GuiLabelButton(NextControlBounds(std::max(HEAD_LABEL_MIN_WIDTH, static_cast<float>(GetTextWidth(head_label_str.c_str())))),
        head_label_str.c_str()) && PxlsDialog::CurrentToken() != PLAYBACK_HEAD_TOKEN) {
        // try to acquire the dialog token
        PxlsDialog::AcquireToken(PLAYBACK_HEAD_TOKEN);
    }

    // progress bar
    auto playback_head_raw = static_cast<float>(playback_head);
    GuiSliderBar(NextControlBounds(progress_panel_rect.width - control_x),
        nullptr, nullptr, &playback_head_raw, 0.0f, static_cast<float>(db.RecordCount()));
    if (!PxlsDialog::IsDialogOpen())
        playback_head = static_cast<unsigned long>(std::floorf(playback_head_raw));
    // revert back to normal
    GuiSetState(STATE_NORMAL);
    // render all kinds of dialog
    if (PxlsDialog::CurrentToken() == PLAYBACK_SPEED_TOKEN) {
        std::string speed_value_str;
        // render the dialog as long as the dialog is open
        PxlsDialog::TextInputBox(window_width, window_height, 0, "Set playback speed",
                                        "Input playback speed(-10000 - 10000 rec/f):", speed_value_str, button_result);
        if (button_result == 1) {
            try {
                if (const auto pb_speed = std::stoi(speed_value_str); pb_speed != 0)
                    playback_speed = std::clamp(pb_speed, -10000, 10000);
            } catch (std::invalid_argument&) {}
        }
        if (button_result != -1)
            PxlsDialog::ReleaseToken(0);
    }

    if (PxlsDialog::CurrentToken() == PLAYBACK_HEAD_TOKEN) {
        std::string head_value_str;
        // render the dialog as long as the dialog is open
        PxlsDialog::TextInputBox(window_width, window_height, 1, "Set playback head",
                                        std::format("Input playback head(0 - {}):", db.RecordCount()), head_value_str, button_result);
        if (button_result == 1) {
            try {
                playback_head = std::clamp(std::stoul(head_value_str), 0ul, db.RecordCount());
            } catch (std::invalid_argument&) {}
        }
        if (button_result != -1)
            PxlsDialog::ReleaseToken(1);
    }

    if (PxlsDialog::CurrentToken() == CANVAS_FUTURE_TOKEN) {
        progress_mutex.lock();
        PxlsDialog::PendingBox(window_width, window_height, CANVAS_FUTURE_TOKEN,
            std::format("Updating canvas, please wait... ({} / {})", update_progress, update_progress_total));
        progress_mutex.unlock();
    }
    // wait for the update process to finish before doing the next canvas update
    if (!IsCanvasUpdating()) {
        // do playback and update canvas
        if (playback_head != db.Seek())
            UpdateCanvas(playback_head, db, canvas);
        else if (playback_state == PLAY) {
            // loop playback
            if (db.Seek() == db.RecordCount() && playback_speed > 0)
                UpdateCanvas(0, db, canvas);
            else if (db.Seek() == 0 && playback_speed < 0)
                UpdateCanvas(db.RecordCount(), db, canvas);
            else
                UpdateCanvas(std::clamp(static_cast<long long>(db.Seek()) + playback_speed, 0ll, static_cast<long long>(db.RecordCount())), db, canvas);
            // pause when the playback head reaches the end
            if ((db.Seek() == 0 && playback_speed < 0) || (db.Seek() == db.RecordCount() && playback_speed > 0))
                playback_state = PAUSE;
        }
        playback_head = db.Seek();
    }
}

void PxlsPlaybackPanel::UpdateCanvas(const unsigned pb_head, PxlsLogDB &db, PxlsCanvas &canvas) {
    // optimization for jumping back to beginning
    if (pb_head == 0) {
        canvas.ClearCanvas();
        db.Seek(0);
    } else {
        // jump to nearest snapshot to improve performance
        JumpToNearestSnapshot(pb_head, db, canvas);
        // return if the nearest snapshot id is pb_head itself
        if (db.Seek() == pb_head) return;
        if (std::abs(static_cast<long long>(db.Seek()) - pb_head) > ASYNC_PROCESS_THRESHOLD) {
            // enable async processing to prevent gui from freezing for a long time
            update_progress = 0;
            update_progress_total = std::abs(static_cast<long long>(db.Seek()) - pb_head);
            PxlsDialog::AcquireToken(CANVAS_FUTURE_TOKEN);
            canvas_future = std::async([&, pb_head] {
                db.QueryRecords(pb_head, [&](const std::optional<std::string> &date, const std::optional<std::string> &hash,
                    const unsigned x, const unsigned y, const std::optional<unsigned> color_index, const std::optional<std::string> &action, const QueryDirection direction) {
                    if (direction == FORWARD) {
                        canvas.PerformAction(x, y, REDO, date, action, hash, color_index);
                    } else {
                        canvas.PerformAction(x, y, UNDO, date, action, hash, color_index);
                    }
                    progress_mutex.lock();
                    update_progress++;
                    progress_mutex.unlock();
                });
                PxlsDialog::ReleaseToken(CANVAS_FUTURE_TOKEN);
                playback_head = db.Seek();
            });
        } else {
            // use usual sync processing to prevent pending box from showing frequently
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
}

void PxlsPlaybackPanel::JumpToNearestSnapshot(const unsigned pb_head, PxlsLogDB &db, PxlsCanvas &canvas) const {
    unsigned long min_dist = std::abs(static_cast<long long>(db.Seek()) - pb_head);
    std::optional<unsigned long> snapshot_id { std::nullopt };
    // regard 0 as a special snapshot id
    auto snapshot_ids_with_zero = snapshot_ids;
    snapshot_ids_with_zero.push_back(0);
    // find nearest snapshot id
    for (const auto &id: snapshot_ids_with_zero) {
        if (std::abs(static_cast<long long>(id) - pb_head) < min_dist) {
            snapshot_id = id;
            min_dist = std::abs(static_cast<long long>(id) - pb_head);
        }
    }
    if (snapshot_id) {
        if (*snapshot_id == 0)
            canvas.ClearCanvas();
        else {
            // load snapshot
            db.QuerySnapshot(*snapshot_id, [&](const void* snapshot_blob) {
                canvas.LoadSnapshot(static_cast<const PxlsCanvasSnapshotPixel*>(snapshot_blob));
            });
        }
        db.Seek(*snapshot_id);
    }
}

//===========================PxlsToolbar===========================
void PxlsToolbar::Render(const std::vector<ToolbarItem> &items, const ToolbarCallback &callback) {
    unsigned btn_x = MARGIN;
    std::optional<std::string> execute_command { std::nullopt };
    GuiEnableTooltip();
    for (const auto &[button_text, button_tooltip, command, disabled, pressed] : items) {
        // set tooltip if it exists
        if (button_tooltip)
            GuiSetTooltip(button_tooltip->c_str());
        else
            GuiSetTooltip(nullptr);
        // disable toolbar button when a dialog is open or disabled == true
        // set pressed state if pressed == true
        GuiSetState(PxlsDialog::IsDialogOpen() || disabled ? STATE_DISABLED : pressed ? STATE_PRESSED : STATE_NORMAL);
        if (GuiButton({ static_cast<float>(btn_x), MARGIN, BTN_WIDTH, BTN_WIDTH }, button_text.c_str()))
            execute_command = command;
        btn_x += BTN_WIDTH + BTN_GAP;
    }
    GuiDisableTooltip();
    // revert back to normal
    GuiSetState(STATE_NORMAL);
    if (execute_command)
        callback(*execute_command);
}