
#ifndef CS_H
#define CS_H

/* #define EDIT_SELF  */

typedef enum RunMode {
  CS_UI_MODE_BROWSE  = 0,
  CS_UI_MODE_UI      = 1,
  CS_UI_MODE_EDIT    = 2,
  CS_UI_MODE_CHROME  = CS_UI_MODE_UI|CS_UI_MODE_EDIT,
} RunMode;

#include "cs-context.h"
#include "cs-util.h"
#include "cs-animator-editor.h"
#include "cs-history.h"

void
props_populate (ClutterActor *container,
                GObject      *object,
                gboolean      easing_buttons);

extern CSContext *cluttersmith;

/* Things that need to go in headers / get proper API: */

void cs_actor_editing_init (gpointer stage);


void   cluttersmith_set_project_root (const gchar *new_root);
gchar *cs_get_project_root (void);

void cs_set_active (ClutterActor *item);
ClutterActor *cs_get_active (void);

void cs_tree_populate (ClutterActor *scene_graph,
                    ClutterActor *active_actor);

ClutterActor *cs_get_current_container (void);
void          cs_set_current_container (ClutterActor *actor);

void cs_set_ui_mode (guint ui_mode);
guint cs_get_ui_mode (void);

void cs_dirtied (void);

/* actor-editing: */

/* selection */

GList   *cs_selected_get_list  (void);
gint     cs_selected_count     (void);
gboolean cs_selected_has_actor (ClutterActor *actor);
void     cs_selected_clear     (void);
void     cs_selected_init      (void);
void     cs_selected_add       (ClutterActor *actor);
void     cs_selected_remove    (ClutterActor *actor);
void     cs_selected_foreach   (GCallback     cb,
                                          gpointer      data);
gboolean cs_save_timeout       (gpointer      data);
gpointer cs_selected_match     (GCallback     match_fun,
                                          gpointer      data);
ClutterActor *cs_selected_get_any (void);

/*****/

void previews_reload (ClutterActor *actor);
char * cs_make_config_file (const char *filename);
void cluttersmith_load_scene (const gchar *new_title);

gboolean manipulator_key_pressed (ClutterActor *stage, ClutterModifierType modifier, guint key);
gboolean manipulator_key_pressed_global (ClutterActor *stage, ClutterModifierType modifier, guint key);

void cs_sync_chrome (void);

extern gint cs_last_x;
extern gint cs_last_y;

void playback_menu  (gint          x,
                     gint          y);
void root_menu      (gint          x,
                     gint          y);
void object_menu    (ClutterActor *actor,
                     gint          x,
                     gint          y);
void selection_menu (gint          x,
                     gint          y);

gboolean edit_text_start (ClutterActor *actor);
void session_history_init_hack (ClutterActor  *actor);
void templates_container_init_hack (ClutterActor  *actor);

gboolean cs_edit_actor_start (ClutterActor *actor);
gboolean cs_edit_actor_end   (void);
gchar *json_serialize_animator (ClutterAnimator *animator);

void cs_save (gboolean force);
#define CS_PROPEDITOR_LABEL_WIDTH  100
#define CS_PROPEDITOR_EDITOR_WIDTH  80
#define EDITOR_LINE_HEIGHT          23
#define CS_EDITOR_LABEL_FONT "Liberation 11px"


#define JS_PREAMBLE \
     "const CS = imports.gi.ClutterSmith;\n"\
                "const Clutter = imports.gi.Clutter;\n"\
                "const GLib = imports.gi.GLib;\n"\
                "const Lang = imports.lang;\n"\
                "const Mainloop = imports.mainloop;\n"\
                "function $(id) {return CS.get_actor(id);}"

MxMenu *cs_menu_new (void);

void export_png (const gchar *scene,
                 const gchar *png);
void export_pdf (const gchar *pdf);

void cs_update_animator_editor (ClutterStates *states,
                                const gchar   *source_state,
                                const gchar   *target_state);
void cs_session_history_add    (const gchar *dir);


gboolean cs_move_start (ClutterActor  *actor,
                        ClutterEvent  *event);
gboolean cs_resize_start (ClutterActor  *actor,
                          ClutterEvent  *event);

#define SNAP_THRESHOLD  2

extern gint cs_set_keys_freeze; /* XXX: global! */
gchar *cs_json_escape_string (const gchar *in);
#endif
