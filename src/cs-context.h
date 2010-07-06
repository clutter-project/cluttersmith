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

#ifndef _CS_CONTEXT_H
#define _CS_CONTEXT_H

#include <clutter/clutter.h>
#include <mx/mx.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define CS_TYPE_CONTEXT cs_context_get_type()

#define CS_CONTEXT(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  CS_TYPE_CONTEXT, CSContext))

#define CS_CONTEXT_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  CS_TYPE_CONTEXT, CSContextClass))

#define CS_IS_CONTEXT(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  CS_TYPE_CONTEXT))

#define CS_IS_CONTEXT_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  CS_TYPE_CONTEXT))

#define CS_CONTEXT_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  CS_TYPE_CONTEXT, CSContextClass))

typedef struct _CSContext        CSContext;
typedef struct _CSContextClass   CSContextClass;
typedef struct _CSContextPrivate CSContextPrivate;

struct _CSContext
{
  GObject parent;

  CSContextPrivate *priv;
  gint          ui_mode;
  GList        *animators;
  GList        *state_machines;
  gchar        *project_root;

  ClutterActor  *current_container;

  ClutterState  *current_state_machine;
  const gchar   *current_source_state;  /* interned string */
  const gchar   *current_target_state;  /* interned string */

  ClutterAnimator *current_animator; /* (if any) */
  /* XXX: add currently edited state machine */

  /* The following are cached lookups for ids in cluttersmith.json, since
   * various parts of the ClutterSmith UI might need to know values of
   * or change values in various of these
   */

  ClutterActor *parasite_root;
  ClutterActor *parasite_ui;
  ClutterActor *fake_stage_canvas;
  ClutterActor *fake_stage;
  ClutterActor *project_root_entry;
  ClutterActor *project_title;
  ClutterActor *scene_title;

  ClutterActor *scene_graph;
  ClutterActor *property_editors;
  ClutterActor *property_editors_core;

  ClutterActor *ancestors;
  ClutterActor *type;

  ClutterActor *active_container;
  ClutterActor *callbacks_container;
  ClutterActor *states_container;

  ClutterActor *cs_mode;
  ClutterActor *dialog_project;
  ClutterActor *dialog_selected;
  ClutterActor *dialog_config;
  ClutterActor *dialog_callbacks;
  ClutterActor *dialog_toolbar;
  ClutterActor *dialog_property_inspector;
  ClutterActor *dialog_tree;

  ClutterActor *dialog_annotate;
  ClutterActor *dialog_editor_annotation;

  ClutterActor *dialog_type;
  ClutterActor *dialog_editor;
  ClutterActor *dialog_editor_text;
  ClutterActor *dialog_editor_error;
  ClutterActor *dialog_templates;
  ClutterActor *dialog_scenes;
  ClutterActor *dialog_export;
  ClutterActor *dialog_states;
  ClutterActor *dialog_animator;
  ClutterActor *state_machine_name;
  ClutterActor *source_state;
  ClutterActor *target_state;
  ClutterActor *remove_transition;
  ClutterActor *animation_name;
  ClutterActor *animator_props;
  ClutterActor *animator_editor;
  ClutterActor *state_duration;

  ClutterActor *resize_handle;
  ClutterActor *anchor_handle;
  ClutterActor *move_handle;
  ClutterActor *depth_handle;
  ClutterActor *rotate_x_handle;
  ClutterActor *rotate_y_handle;
  ClutterActor *rotate_z_handle;
};


struct _CSContextClass
{
  GObjectClass parent_class;
};

GType cs_context_get_type (void);

void cs_prop_tweaked (GObject     *object,
                      const gchar *property_name);

gfloat cluttersmith_get_origin_x (void);
gfloat cluttersmith_get_origin_y (void);

CSContext *cs_context_new (void);
void cs_set_current_animator (ClutterAnimator *animator);
ClutterTimeline *cluttersmith_animator_start (const gchar *animator);
CSContext *cluttersmith_context (void);

void cs_context_set_scene (CSContext   *cs_context,
                           const gchar *scene);

G_END_DECLS

#endif /* _CS_CONTEXT_H */
