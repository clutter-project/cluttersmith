/*
 * ClutterSmith - a visual authoring environment for clutter.
 * Copyright (c) 2009, Intel Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU Lesser General Public License,
 * version 2.1, as published by the Free Software Foundation.
 *
 * Alternatively, you can redistribute it and/or modify it
 * under the terms and conditions of the GNU Lesser General Public License,
 * version 3, with the following additional permissions:
 *
 * 1. Intel grants you an additional permission under Section 7 of the
 * GNU General Public License, version 3, exempting you from the
 * requirement in Section 6 of the GNU General Public License, version 3,
 * to accompany Corresponding Source with Installation Information for
 * the Program or any work based on the Program.  You are still required
 * to comply with all other Section 6 requirements to provide
 * Corresponding Source.
 *
 * 2. Intel grants you an additional permission under Section 7 of the
 * GNU General Public License, version 3, allowing you to convey the
 * Program or a work based on the Program in combination with or linked
 * to any works licensed under the GNU General Public License version 2,
 * with the terms and conditions of the GNU General Public License
 * version 2 applying to the combined or linked work as a whole.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Written by: Øyvind Kolås <oyvind.kolas@intel.com>
 */
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

extern CSContext *cs;

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

gboolean cs_selected_lasso_start (ClutterActor  *actor,
                                 ClutterEvent  *event);

void cs_selected_paint (void);
void cs_move_snap_paint (void);
extern ClutterActor      *lasso; /* XXX: global */
/*****/
void cs_actor_editors_add (GType type,
                           void (*editing_start) (ClutterActor *edited_actor),
                           void (*editing_end) (ClutterActor *edited_actor));
void cs_edit_text_init (void);

void previews_reload (ClutterActor *actor);
void cluttersmith_load_scene (const gchar *new_title);
void cs_animation_edit_init (void);

gboolean manipulator_key_pressed (ClutterActor *stage, ClutterModifierType modifier, guint key);
gboolean manipulator_key_pressed_global (ClutterActor *stage, ClutterModifierType modifier, guint key);

void cs_zoom (gboolean in,
              gfloat   x,
              gfloat   y);
void cs_sync_chrome (void);

gboolean cs_stage_capture (ClutterActor *actor,
                           ClutterEvent *event,
                           gpointer      data /* unused */);

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

void session_history_init_hack (ClutterActor  *actor);
void templates_container_init_hack (ClutterActor  *actor);

gboolean cs_edit_actor_start (ClutterActor *actor);
gboolean cs_edit_actor_end   (void);
gchar *json_serialize_animator (ClutterAnimator *animator);
gchar *json_serialize_state (ClutterState *state);

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

void cs_update_animator_editor (ClutterState *state,
                                const gchar  *source_state,
                                const gchar  *target_state);
void cs_session_history_add    (const gchar *dir);

gboolean cluttersmith_initialize_for_stage (gpointer stage);

gboolean cs_move_start (ClutterActor  *actor,
                        ClutterEvent  *event);
gboolean cs_resize_start (ClutterActor  *actor,
                          ClutterEvent  *event);

#define SNAP_THRESHOLD  2

extern gint cs_set_keys_freeze; /* XXX: global! */
gchar *cs_json_escape_string (const gchar *in);

void cs_callbacks_populate (ClutterActor *actor);

/* these are defined here to allow sharing the boilerplate 
 * among different .c files
 */
#define SELECT_ACTION_PRE2() \
  g_string_append_printf (undo, "var list=[");\
  cs_selected_foreach (G_CALLBACK (each_add_to_list), undo);\
  g_string_append_printf (undo, "];\n"\
                          "CS.selected_clear();\n"\
                          "for (x in list) CS.selected_add (list[x]);\n");
#define SELECT_ACTION_PRE() \
  GString *undo = g_string_new ("");\
  GString *redo = g_string_new ("");\
  SELECT_ACTION_PRE2();
#define SELECT_ACTION_POST(action_name) \
  g_string_append_printf (redo, "var list=[");\
  cs_selected_foreach (G_CALLBACK (each_add_to_list), redo);\
  g_string_append_printf (redo, "];\n"\
                          "CS.selected_clear();\n"\
                          "for (x in list) CS.selected_add (list[x]);\n");\
  if (!g_str_equal (redo->str, undo->str))\
    cs_history_add (action_name, redo->str, undo->str);\
  g_string_free (undo, TRUE);\
  g_string_free (redo, TRUE);\
  undo = redo = NULL;

#endif
