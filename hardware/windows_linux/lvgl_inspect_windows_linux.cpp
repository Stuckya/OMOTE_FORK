#include "lvgl_inspect_windows_linux.h"

#include <lvgl.h>
#include <src/extra/others/snapshot/lv_snapshot.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>

namespace {

const char *const kInspectDir  = "/tmp/omote_inspect";
const char *const kRequestPath = "/tmp/omote_inspect/request";
const char *const kDonePath    = "/tmp/omote_inspect/done";
const char *const kScreenPath  = "/tmp/omote_inspect/screen.png";
const char *const kTreePath    = "/tmp/omote_inspect/tree.json";

const uint32_t kPollPeriodMs = 150;

struct StateName {
  lv_state_t bit;
  const char *name;
};

const StateName kStateNames[] = {
  {LV_STATE_CHECKED, "checked"},   {LV_STATE_FOCUSED, "focused"},
  {LV_STATE_EDITED, "edited"},     {LV_STATE_HOVERED, "hovered"},
  {LV_STATE_PRESSED, "pressed"},   {LV_STATE_SCROLLED, "scrolled"},
  {LV_STATE_DISABLED, "disabled"},
};

// LVGL 8.3 stores no class name, so map the class singleton to a label. Only
// reference classes whose widget is compiled in, to avoid undefined symbols.
const char *widget_type_name(lv_obj_t *obj) {
  const lv_obj_class_t *cls = lv_obj_get_class(obj);
#if LV_USE_LABEL
  if (cls == &lv_label_class) return "label";
#endif
#if LV_USE_BTN
  if (cls == &lv_btn_class) return "btn";
#endif
#if LV_USE_BTNMATRIX
  if (cls == &lv_btnmatrix_class) return "btnmatrix";
#endif
#if LV_USE_IMG
  if (cls == &lv_img_class) return "img";
#endif
#if LV_USE_BAR
  if (cls == &lv_bar_class) return "bar";
#endif
#if LV_USE_SLIDER
  if (cls == &lv_slider_class) return "slider";
#endif
#if LV_USE_ARC
  if (cls == &lv_arc_class) return "arc";
#endif
#if LV_USE_SWITCH
  if (cls == &lv_switch_class) return "switch";
#endif
#if LV_USE_CHECKBOX
  if (cls == &lv_checkbox_class) return "checkbox";
#endif
#if LV_USE_DROPDOWN
  if (cls == &lv_dropdown_class) return "dropdown";
#endif
#if LV_USE_ROLLER
  if (cls == &lv_roller_class) return "roller";
#endif
#if LV_USE_TEXTAREA
  if (cls == &lv_textarea_class) return "textarea";
#endif
#if LV_USE_LINE
  if (cls == &lv_line_class) return "line";
#endif
  return "obj";
}

const char *widget_text(lv_obj_t *obj) {
  const lv_obj_class_t *cls = lv_obj_get_class(obj);
#if LV_USE_LABEL
  if (cls == &lv_label_class) return lv_label_get_text(obj);
#endif
#if LV_USE_CHECKBOX
  if (cls == &lv_checkbox_class) return lv_checkbox_get_text(obj);
#endif
#if LV_USE_TEXTAREA
  if (cls == &lv_textarea_class) return lv_textarea_get_text(obj);
#endif
  return NULL;
}

void json_write_escaped(FILE *f, const char *s) {
  for (; *s != '\0'; ++s) {
    unsigned char c = (unsigned char)*s;
    switch (c) {
      case '"':  fputs("\\\"", f); break;
      case '\\': fputs("\\\\", f); break;
      case '\n': fputs("\\n", f); break;
      case '\r': fputs("\\r", f); break;
      case '\t': fputs("\\t", f); break;
      default:
        if (c < 0x20) fprintf(f, "\\u%04x", c);
        else fputc(c, f);
    }
  }
}

void write_indent(FILE *f, int depth) {
  for (int i = 0; i < depth; ++i) fputs("  ", f);
}

void write_node(FILE *f, lv_obj_t *obj, int depth) {
  lv_area_t area;
  lv_obj_get_coords(obj, &area);
  lv_state_t state = lv_obj_get_state(obj);

  write_indent(f, depth);
  fprintf(f, "{\"type\":\"%s\",\"x\":%d,\"y\":%d,\"w\":%d,\"h\":%d",
          widget_type_name(obj), (int)area.x1, (int)area.y1,
          (int)lv_area_get_width(&area), (int)lv_area_get_height(&area));

  fputs(",\"state\":[", f);
  bool first = true;
  for (const StateName &sn : kStateNames) {
    if ((state & sn.bit) == 0) continue;
    if (!first) fputc(',', f);
    fprintf(f, "\"%s\"", sn.name);
    first = false;
  }
  fputc(']', f);

  const char *text = widget_text(obj);
  if (text != NULL) {
    fputs(",\"text\":\"", f);
    json_write_escaped(f, text);
    fputc('"', f);
  }

  uint32_t child_cnt = lv_obj_get_child_cnt(obj);
  if (child_cnt > 0) {
    fputs(",\"children\":[\n", f);
    for (uint32_t i = 0; i < child_cnt; ++i) {
      write_node(f, lv_obj_get_child(obj, i), depth + 1);
      fputs(i + 1 < child_cnt ? ",\n" : "\n", f);
    }
    write_indent(f, depth);
    fputc(']', f);
  }
  fputc('}', f);
}

bool write_screen_png(lv_obj_t *scr) {
#if LV_COLOR_DEPTH == 16
  uint32_t buf_size = lv_snapshot_buf_size_needed(scr, LV_IMG_CF_TRUE_COLOR);
  if (buf_size == 0) {
    printf("[inspect] snapshot size unavailable\n");
    return false;
  }
  // A full-screen snapshot (~150KB) dwarfs the LVGL heap (LV_MEM_SIZE), so take
  // it into a system-heap buffer; lv_snapshot_take() would assert-hang on the
  // failed lv_mem_alloc. take_to_buf only draws through a small LVGL draw_ctx.
  void *buf = malloc(buf_size);
  if (buf == NULL) {
    printf("[inspect] snapshot buffer alloc failed (%u bytes)\n", buf_size);
    return false;
  }

  lv_img_dsc_t dsc;
  bool ok = false;
  if (lv_snapshot_take_to_buf(scr, LV_IMG_CF_TRUE_COLOR, &dsc, buf, buf_size) ==
      LV_RES_OK) {
    SDL_Surface *surface = SDL_CreateRGBSurfaceWithFormatFrom(
        buf, dsc.header.w, dsc.header.h, LV_COLOR_DEPTH,
        dsc.header.w * (LV_COLOR_DEPTH / 8), SDL_PIXELFORMAT_RGB565);
    if (surface != NULL) {
      ok = IMG_SavePNG(surface, kScreenPath) == 0;
      SDL_FreeSurface(surface);
    }
    if (!ok) printf("[inspect] PNG save failed: %s\n", IMG_GetError());
  } else {
    printf("[inspect] lv_snapshot_take_to_buf failed\n");
  }

  free(buf);
  return ok;
#else
  (void)scr;
  printf("[inspect] PNG dump only implemented for LV_COLOR_DEPTH 16\n");
  return false;
#endif
}

void do_dump() {
  remove(kDonePath);
  lv_obj_t *scr = lv_scr_act();
  bool png_ok = write_screen_png(scr);

  bool tree_ok = false;
  FILE *tree = fopen(kTreePath, "w");
  if (tree != NULL) {
    write_node(tree, scr, 0);
    fputc('\n', tree);
    fclose(tree);
    tree_ok = true;
  }

  remove(kRequestPath);

  FILE *done = fopen(kDonePath, "w");
  if (done != NULL) {
    fprintf(done, "{\"png\":%s,\"tree\":%s}\n", png_ok ? "true" : "false",
            tree_ok ? "true" : "false");
    fclose(done);
  }
  printf("[inspect] dump complete (png=%d tree=%d)\n", png_ok, tree_ok);
}

void inspect_timer_cb(lv_timer_t *timer) {
  LV_UNUSED(timer);
  if (access(kRequestPath, F_OK) == 0) do_dump();
}

}  // namespace

void lvgl_inspect_init() {
  mkdir(kInspectDir, 0755);
  IMG_Init(IMG_INIT_PNG);
  lv_timer_create(inspect_timer_cb, kPollPeriodMs, NULL);
  printf("[inspect] LVGL inspector armed: touch %s to dump\n", kRequestPath);
}
