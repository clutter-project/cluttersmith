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

#include <clutter/clutter.h>
#include <mx/mx.h>
#include <stdlib.h>
#include <string.h>
#include "cluttersmith.h"

static gboolean text_was_editable = FALSE;
static gboolean text_was_reactive = FALSE;

static void
edit_text_start (ClutterActor *actor)
{
  if (MX_IS_LABEL (actor))
    actor = mx_label_get_clutter_text (MX_LABEL (actor));

  g_object_get (actor, "editable", &text_was_editable,
                       "reactive", &text_was_reactive, NULL);
  g_object_set (actor, "editable", TRUE,
                       "reactive", TRUE, NULL);
  clutter_stage_set_key_focus (CLUTTER_STAGE (clutter_actor_get_stage (actor)), actor);
}

static void
edit_text_end (ClutterActor *actor)
{
  g_object_set (actor, "editable", text_was_editable,
                       "reactive", text_was_reactive, NULL);
  cs_dirtied ();
}

void
cs_edit_text_init (void)
{
  cs_actor_editors_add (CLUTTER_TYPE_TEXT, edit_text_start, edit_text_end);
  cs_actor_editors_add (MX_TYPE_LABEL, edit_text_start, edit_text_end);
}

