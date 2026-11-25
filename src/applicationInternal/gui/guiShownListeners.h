#pragma once

#include <string>

namespace Gui {

using ShownGuiListener = void (*)(const std::string& shownGuiName);

// Tabs are rebuilt before the shown one is known, so a tab that works only while on screen, such
// as a ticking clock, waits for this to learn whether it is the one shown.
void registerShownGuiListener(ShownGuiListener listener);

void notifyShownGui(const std::string& shownGuiName);

}  // namespace Gui
