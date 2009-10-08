
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

void
props_populate (ClutterActor *container,
                GObject      *object);

extern CSContext *cluttersmith;

/* Things that need to go in headers / get proper API: */

void cs_actor_editing_init (gpointer stage);


void   cs_set_project_root (const gchar *new_root);
gchar *cs_get_project_root (void);

void cs_open_layout (const gchar *new_title);
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
void cs_load_dialog_state (void);
void cs_save_dialog_state (void);
ClutterActor *cs_selected_get_any (void);

/*****/

void previews_reload (ClutterActor *actor);
char * cs_make_config_file (const char *filename);


gboolean manipulator_key_pressed (ClutterActor *stage, ClutterModifierType modifier, guint key);
gboolean manipulator_key_pressed_global (ClutterActor *stage, ClutterModifierType modifier, guint key);

void cs_sync_chrome (void);

extern gint cs_last_x;
extern gint cs_last_y;

void playback_popup  (gint          x,
                      gint          y);
void root_popup      (gint          x,
                      gint          y);
void object_popup    (ClutterActor *actor,
                      gint          x,
                      gint          y);
void selection_popup (gint          x,
                      gint          y);

gboolean edit_text_start (ClutterActor *actor);

#define CS_PROPEDITOR_LABEL_WIDTH  120
#define CS_PROPEDITOR_EDITOR_WIDTH  80
#define EDITOR_LINE_HEIGHT         20
#define CS_EDITOR_LABEL_FONT "Liberation 11px"

#endif
