#pragma once

#include <lvgl.h>
#include <string>

enum class ResultKind { kSuccess, kFailed, kTimeout };

// A centered result block (badge + title + wrapped subtitle) for terminal/wait
// states. Added into a flex-column parent (e.g. a modalShell); the caller adds
// the action buttons below it, since those vary by state.
void resultCard_create(lv_obj_t *parent, ResultKind kind,
                       const std::string &title, const std::string &subtitle);
