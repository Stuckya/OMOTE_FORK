#pragma once
#include <string>
#include <lvgl.h>

enum GUIlists {
// MAIN_GUI_LIST: we are in the main_gui_list (with the scene selector as first gui), either if a scene is active or not
// SCENE_GUI_LIST: a scene is active and we are not in the main_gui_list. In that case, we try to use the scene specific gui list, if the scene defined one.
  MAIN_GUI_LIST,
  SCENE_GUI_LIST
};

void gui_memoryOptimizer_onStartup(lv_obj_t** tabview, lv_obj_t** panel, lv_obj_t** img1, lv_obj_t** img2);
void gui_memoryOptimizer_afterSliding(lv_obj_t** tabview, lv_obj_t** panel, lv_obj_t** img1, lv_obj_t** img2, int newTabID);
void gui_memoryOptimizer_afterGUIlistChanged(lv_obj_t** tabview, lv_obj_t** panel, lv_obj_t** img1, lv_obj_t** img2, GUIlists newGUIlist);
void gui_memoryOptimizer_navigateToGUI(lv_obj_t** tabview, lv_obj_t** panel, lv_obj_t** img1, lv_obj_t** img2, GUIlists GUIlist, int gui_list_index);
void gui_memoryOptimizer_navigateToLastActiveGUIofPreviousGUIlist(lv_obj_t** tabview, lv_obj_t** panel, lv_obj_t** img1, lv_obj_t** img2);

// Free the whole tab tree while a full-screen overlay (e.g. pairing) owns the
// screen, then rebuild the same GUI on exit. The overlay lives on lv_layer_top,
// so it survives the teardown. Both are idempotent and reuse the tested
// notifyAndClear/navigateToGUI paths so no parallel teardown logic is needed.
void gui_memoryOptimizer_suspendActiveTabs(lv_obj_t** tabview, lv_obj_t** panel, lv_obj_t** img1, lv_obj_t** img2);
void gui_memoryOptimizer_resumeActiveTabs(lv_obj_t** tabview, lv_obj_t** panel, lv_obj_t** img1, lv_obj_t** img2);
// true while a full-screen overlay has torn the tab tree down; tab (re)creation
// must be suppressed so a stray nav does not rebuild the tabview under the overlay
bool gui_memoryOptimizer_tabsSuspended();

int gui_memoryOptimizer_getActiveTabID();
bool gui_memoryOptimizer_isTabIDInMemory(int tabID);
bool gui_memoryOptimizer_isGUInameInMemory(std::string GUIname);

std::string gui_memoryOptimizer_getActiveSceneName();
void gui_memoryOptimizer_setActiveSceneName(std::string aSceneName);
std::string gui_memoryOptimizer_getActiveGUIname();
void gui_memoryOptimizer_setActiveGUIname(std::string aGUIname);
GUIlists gui_memoryOptimizer_getActiveGUIlist();
void gui_memoryOptimizer_setActiveGUIlist(GUIlists aGUIlist);

