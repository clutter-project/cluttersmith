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

/* snap size, as affected by resizing lower right corner,
 * will need extension if other corners are to be supported,
 * it seems possible to do all needed alignments through
 * simple workarounds when only snapping for lower right).
 */
static void snap_size (ClutterActor *actor,
                       gfloat        in_width,
                       gfloat        in_height,
                       gfloat       *out_width,
                       gfloat       *out_height)
{
  *out_width = in_width;
  *out_height = in_height;

  ClutterActor *parent;

  parent = clutter_actor_get_parent (actor);

  if (CLUTTER_IS_CONTAINER (parent))
    {
      gfloat in_x = clutter_actor_get_x (actor);
      gfloat in_y = clutter_actor_get_y (actor);
      gfloat in_end_x = in_x + in_width;
      gfloat in_end_y = in_y + in_height;


      gfloat best_x = 0;
      gfloat best_x_diff = 4096;
      gfloat best_y = 0;
      gfloat best_y_diff = 4096;
      GList *children, *c;
      children = clutter_container_get_children (CLUTTER_CONTAINER (parent));

      hor_pos = 0;
      ver_pos = 0;

      /* We only search our siblings for snapping...
       * perhaps we should search more.
       */

      for (c=children; c; c = c->next)
        {
          gfloat this_x = clutter_actor_get_x (c->data);
          gfloat this_width = clutter_actor_get_width (c->data);
          gfloat this_height = clutter_actor_get_height (c->data);
          gfloat this_end_x = this_x + this_width;
          gfloat this_y = clutter_actor_get_y (c->data);
          gfloat this_end_y = this_y + this_height;

          /* skip self */
          if (c->data == actor)
            continue;

/*
 end aligned with start
                    this_x     this_mid_x     this_end_x
in_x    in_mid_x    in_end_x
*/
          if (abs (this_x - in_end_x) < best_x_diff)
            {
              best_x_diff = abs (this_x - in_end_x);
              best_x = this_x;
              hor_pos=3;
            }
          if (abs (this_y - in_end_y) < best_y_diff)
            {
              best_y_diff = abs (this_y - in_end_y);
              best_y = this_y;
              ver_pos=3;
            }
/*
 ends aligned
                    this_x     this_mid_x     this_end_x
                          in_x    in_mid_x    in_end_x
*/
          if (abs (this_end_x - in_end_x) < best_x_diff)
            {
              best_x_diff = abs (this_end_x - in_end_x);
              best_x = this_end_x;
              hor_pos=3;
            }
          if (abs (this_end_y - in_end_y) < best_y_diff)
            {
              best_y_diff = abs (this_end_y - in_end_y);
              best_y = this_end_y;
              ver_pos=3;
            }
        }

        {
          if (best_x_diff < SNAP_THRESHOLD)
            {
              *out_width = best_x-in_x;
            }
          else
            {
              hor_pos = 0;
            }
          if (best_y_diff < SNAP_THRESHOLD)
            {
              *out_height = best_y-in_y;
            }
          else
            {
              ver_pos = 0;
            }
        }
    }
}

static void
selection_to_size_commands (GString *string)
{
  GList *s, *selected;

  selected = cs_selected_get_list ();
  for (s = selected; s; s = s->next)
    {
      ClutterActor *actor = s->data;
      gfloat width, height;
      clutter_actor_get_size (actor, &width, &height);
      g_string_append_printf (string, "$('%s').width = %f; $('%s').height = %f;\n",
                              cs_get_id (actor), width,
                              cs_get_id (actor), height);
    }
  g_list_free (selected);
}


static gboolean
manipulate_resize_capture (ClutterActor *stage,
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
          clutter_actor_get_size (actor, &w, &h);

          w-= manipulate_x-ex;
          h-= manipulate_y-ey;

          snap_size (actor, w, h, &w, &h);

          clutter_actor_set_size (actor, w, h);
          cs_prop_tweaked (G_OBJECT (actor), "width");
          cs_prop_tweaked (G_OBJECT (actor), "height");

          manipulate_x=ex;
          manipulate_y=ey;
        }
        break;
      case CLUTTER_BUTTON_RELEASE:
        hor_pos = 0;
        ver_pos = 0;

        selection_to_size_commands (redo);
        if (start_x != manipulate_x
            || start_y != manipulate_y)
          cs_history_add ("resize actors", redo->str, undo->str);
        g_string_free (undo, TRUE);
        g_string_free (redo, TRUE);
        undo = redo = NULL;

        cs_dirtied ();
        clutter_actor_queue_redraw (stage);
        g_signal_handlers_disconnect_by_func (stage, manipulate_resize_capture, data);
      default:
        break;
    }
  return TRUE;
}

gboolean cs_resize_start (ClutterActor  *actor,
                          ClutterEvent  *event)
{
  ClutterActor *first_actor = cs_selected_get_any();
  start_x = manipulate_x = event->button.x;
  start_y = manipulate_y = event->button.y;

  clutter_actor_transform_stage_point (first_actor, event->button.x, event->button.y, &manipulate_x, &manipulate_y);

  undo = g_string_new ("");
  redo = g_string_new ("");
  selection_to_size_commands (undo);

  g_signal_connect (clutter_actor_get_stage (actor), "captured-event",
                    G_CALLBACK (manipulate_resize_capture), actor);
  return TRUE;
}
