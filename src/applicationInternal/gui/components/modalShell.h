#pragma once

#include <lvgl.h>
#include <string>

// A focused/modal overlay on lv_layer_top: opaque, positioned below the 20px
// status bar (device context stays), with a standard header (device icon +
// name + optional subtitle + optional cancel X). Returns the content root, a
// flex column to add body widgets into; delete it to dismiss the modal.
//
// on_cancel == nullptr hides the header X (terminal states own cancel via their
// own buttons, so a header X would be a redundant second cancel).
lv_obj_t *modalShell_create(const std::string &name, const std::string &sub,
                            lv_event_cb_t on_cancel);
