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

#ifndef __CS_ANIMATOR_EDITOR_H__
#define __CS_ANIMATOR_EDITOR_H__

#include <clutter/clutter.h>

G_BEGIN_DECLS

#define CS_TYPE_ANIMATOR_EDITOR    \
  (cs_animator_editor_get_type ())
#define CS_ANIMATOR_EDITOR(obj)    \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj),  \
                               CS_TYPE_ANIMATOR_EDITOR, \
                               CSAnimatorEditor))
#define CS_ANIMATOR_EDITOR_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass),    \
                            CS_TYPE_ANIMATOR_EDITOR, \
                            CSAnimatorEditorClass))
#define IS_CS_ANIMATOR_EDITOR(obj)    \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
                               CS_TYPE_ANIMATOR_EDITOR))
#define IS_CS_ANIMATOR_EDITOR_CLASS(klass)  \
  (G_TYPE_CHECK_CLASS_TYPE ((klass),    \
                            CS_TYPE_ANIMATOR_EDITOR))
#define CS_ANIMATOR_EDITOR_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj),     \
                              CS_TYPE_ANIMATOR_EDITOR, \
                              CSAnimatorEditorClass))

typedef struct _CSAnimatorEditorPrivate CSAnimatorEditorPrivate;
typedef struct _CSAnimatorEditor CSAnimatorEditor;
typedef struct _CSAnimatorEditorClass CSAnimatorEditorClass;

struct _CSAnimatorEditor
{
  ClutterActor parent;

  CSAnimatorEditorPrivate *priv;
};

struct _CSAnimatorEditorClass
{
  ClutterActorClass parent_class;
};

GType cs_animator_editor_get_type (void) G_GNUC_CONST;

void cs_animator_editor_set_animator (CSAnimatorEditor *animator_editor,
                                      ClutterAnimator  *animator);

void cs_animator_editor_set_progress (CSAnimatorEditor *animator_editor,
                                      gdouble           progress); 
gdouble cs_animator_editor_get_progress (CSAnimatorEditor *animator_editor);
void cs_animator_editor_stage_paint (void);

G_END_DECLS

#endif /* __CS_ANIMATOR_EDITOR_H__ */
