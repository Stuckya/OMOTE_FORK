#include "guiShownListeners.h"

#include <algorithm>
#include <vector>

namespace Gui {

static std::vector<ShownGuiListener>& listeners() {
  static std::vector<ShownGuiListener> registered;
  return registered;
}

void registerShownGuiListener(ShownGuiListener listener) {
  if (std::find(listeners().begin(), listeners().end(), listener) != listeners().end()) {
    return;
  }
  listeners().push_back(listener);
}

void notifyShownGui(const std::string& shownGuiName) {
  for (ShownGuiListener listener : listeners()) {
    listener(shownGuiName);
  }
}

}  // namespace Gui
