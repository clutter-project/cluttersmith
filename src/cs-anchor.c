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


#include <math.h>
#include <stdlib.h>
#include "cluttersmith.h"

/* snap positions, in relation to actor */
static gint ver_pos = 0; /* 1 = start 2 = mid 3 = end */
static gint hor_pos = 0; /* 1 = start 2 = mid 3 = end */
/*
 * Shared state used by all the separate states the event handling can
 * the substates are exclusive (can be extended to have a couple of stages
 * that allow falling through to each other, but a fixed event pipeline
 * is an simpler initial base).
 */
static gfloat manipulate_x;
static gfloat manipulate_y;
static gfloat start_x;
static gfloat start_y;

static GString *undo = NULL;
static GString *redo = NULL;

static void
selection_to_anchor_commands (GString *string)
{
  GList *s, *selected;

  selected = cs_selected_get_list ();
  for (s = selected; s; s = s->next)
    {
      ClutterActor *actor = s->data;
      gfloat x, y;
      clutter_actor_get_anchor_point (actor, &x, &y);
      g_string_append_printf (string, "$('%s').anchor_x = %f;"
                                      "$('%s').anchor_y = %f;\n",
                              cs_get_id (actor), x,
                              cs_get_id (actor), y);
      g_string_append_printf (string, "$('%s').x = %f;"
                                      "$('%s').y = %f;\n",
                              cs_get_id (actor), clutter_actor_get_x (actor),
                              cs_get_id (actor), clutter_actor_get_y (actor));

    }
  g_list_free (selected);
}


static gboolean
manipulate_anchor_capture (ClutterActor *stage,
                           ClutterEvent *event,
                           gpointer      data)
{
  switch (event->any.type)
    {
      case CLUTTER_MOTION:
        {
          ClutterActor *actor = cs_selected_get_any();
          /* resize is semi bust for more than one actor */
          gfloat ex, ey;
          clutter_actor_transform_stage_point (actor, event->motion.x, event->motion.y,
                                               &ex, &ey);
          gfloat w, h;
          clutter_actor_get_anchor_point (actor, &w, &h);

          w-= manipulate_x-ex;
          h-= manipulate_y-ey;

          clutter_actor_move_anchor_point (actor, w, h);
          cs_prop_tweaked (G_OBJECT (actor), "anchor-x");
          cs_prop_tweaked (G_OBJECT (actor), "anchor-y");

          manipulate_x=ex;
          manipulate_y=ey;
        }
        break;
      case CLUTTER_BUTTON_RELEASE:
        hor_pos = 0;
        ver_pos = 0;

        selection_to_anchor_commands (redo);
        if (start_x != manipulate_x
            || start_y != manipulate_y)
          cs_history_add ("resize actors", redo->str, undo->str);
        g_string_free (undo, TRUE);
        g_string_free (redo, TRUE);
        undo = redo = NULL;

        cs_dirtied ();
        clutter_actor_queue_redraw (stage);
        g_signal_handlers_disconnect_by_func (stage, manipulate_anchor_capture, data);
      default:
        break;
    }
  return TRUE;
}

gboolean cs_anchor_start (ClutterActor  *actor,
                          ClutterEvent  *event)
{
  ClutterActor *first_actor = cs_selected_get_any();
  start_x = manipulate_x = event->button.x;
  start_y = manipulate_y = event->button.y;

  clutter_actor_transform_stage_point (first_actor, event->button.x, event->button.y, &manipulate_x, &manipulate_y);

  undo = g_string_new ("");
  redo = g_string_new ("");
  selection_to_anchor_commands (undo);

  g_signal_connect (clutter_actor_get_stage (actor), "captured-event",
                    G_CALLBACK (manipulate_anchor_capture), actor);
  return TRUE;
}
