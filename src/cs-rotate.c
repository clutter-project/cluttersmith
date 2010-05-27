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
selection_to_rotz_commands (GString *string)
{
  GList *s, *selected;

  selected = cs_selected_get_list ();
  for (s = selected; s; s = s->next)
    {
      ClutterActor *actor = s->data;
      gfloat rot = clutter_actor_get_rotation (actor, CLUTTER_Z_AXIS, NULL, NULL, NULL);
      g_string_append_printf (string, "$('%s').rotation_angle_z = %f;\n",
                              cs_get_id (actor), rot);
    }
  g_list_free (selected);
}


static gboolean
manipulate_rotate_z_capture (ClutterActor *stage,
                             ClutterEvent *event,
                             gpointer      data)
{
  switch (event->any.type)
    {
      case CLUTTER_MOTION:
        {
          ClutterActor *actor = cs_selected_get_any();
          gfloat ex, ey;
          gfloat rot;
          rot = clutter_actor_get_rotation (actor, CLUTTER_Z_AXIS, NULL, NULL, NULL);
          ex = event->motion.x;
          ey = event->motion.y;

          rot += (manipulate_y-ey);

          clutter_actor_set_rotation (actor, CLUTTER_Z_AXIS, rot, 0,0,0);
          cs_prop_tweaked (G_OBJECT (actor), "rotation-angle-z");

          manipulate_x=ex;
          manipulate_y=ey;
        }
        break;
      case CLUTTER_BUTTON_RELEASE:
        selection_to_rotz_commands (redo);
        if (start_x != manipulate_x
            || start_y != manipulate_y)
          cs_history_add ("rotate actors around z", redo->str, undo->str);
        g_string_free (undo, TRUE);
        g_string_free (redo, TRUE);
        undo = redo = NULL;

        cs_dirtied ();
        clutter_actor_queue_redraw (stage);
        g_signal_handlers_disconnect_by_func (stage, manipulate_rotate_z_capture, data);
      default:
        break;
    }
  return TRUE;
}

gboolean cs_rotate_z_start (ClutterActor  *actor,
                            ClutterEvent  *event)
{
  manipulate_x = event->button.x;
  manipulate_y = event->button.y;

  undo = g_string_new ("");
  redo = g_string_new ("");
  selection_to_rotz_commands (undo);

  g_signal_connect (clutter_actor_get_stage (actor), "captured-event",
                    G_CALLBACK (manipulate_rotate_z_capture), actor);
  return TRUE;
}

/**********/

static void
selection_to_rotx_commands (GString *string)
{
  GList *s, *selected;

  selected = cs_selected_get_list ();
  for (s = selected; s; s = s->next)
    {
      ClutterActor *actor = s->data;
      gfloat rot = clutter_actor_get_rotation (actor, CLUTTER_X_AXIS, NULL, NULL, NULL);
      g_string_append_printf (string, "$('%s').rotation_angle_x = %f;\n",
                              cs_get_id (actor), rot);
    }
  g_list_free (selected);
}


static gboolean
manipulate_rotate_x_capture (ClutterActor *stage,
                             ClutterEvent *event,
                             gpointer      data)
{
  switch (event->any.type)
    {
      case CLUTTER_MOTION:
        {
          ClutterActor *actor = cs_selected_get_any();
          gfloat ex, ey;
          gfloat rot;
          rot = clutter_actor_get_rotation (actor, CLUTTER_X_AXIS, NULL, NULL, NULL);
          ex = event->motion.x;
          ey = event->motion.y;

          rot += (manipulate_y-ey);

          clutter_actor_set_rotation (actor, CLUTTER_X_AXIS, rot, 0,0,0);
          cs_prop_tweaked (G_OBJECT (actor), "rotation-angle-x");

          manipulate_x=ex;
          manipulate_y=ey;
        }
        break;
      case CLUTTER_BUTTON_RELEASE:
        selection_to_rotx_commands (redo);
        if (start_x != manipulate_x
            || start_y != manipulate_y)
          cs_history_add ("rotate actors around x", redo->str, undo->str);
        g_string_free (undo, TRUE);
        g_string_free (redo, TRUE);
        undo = redo = NULL;

        cs_dirtied ();
        clutter_actor_queue_redraw (stage);
        g_signal_handlers_disconnect_by_func (stage, manipulate_rotate_x_capture, data);
      default:
        break;
    }
  return TRUE;
}

gboolean cs_rotate_x_start (ClutterActor  *actor,
                            ClutterEvent  *event)
{
  manipulate_x = event->button.x;
  manipulate_y = event->button.y;

  undo = g_string_new ("");
  redo = g_string_new ("");
  selection_to_rotx_commands (undo);

  g_signal_connect (clutter_actor_get_stage (actor), "captured-event",
                    G_CALLBACK (manipulate_rotate_x_capture), actor);
  return TRUE;
}



/**********/

static void
selection_to_roty_commands (GString *string)
{
  GList *s, *selected;

  selected = cs_selected_get_list ();
  for (s = selected; s; s = s->next)
    {
      ClutterActor *actor = s->data;
      gfloat rot = clutter_actor_get_rotation (actor, CLUTTER_Y_AXIS, NULL, NULL, NULL);
      g_string_append_printf (string, "$('%s').rotation_angle_y = %f;\n",
                              cs_get_id (actor), rot);
    }
  g_list_free (selected);
}


static gboolean
manipulate_rotate_y_capture (ClutterActor *stage,
                             ClutterEvent *event,
                             gpointer      data)
{
  switch (event->any.type)
    {
      case CLUTTER_MOTION:
        {
          ClutterActor *actor = cs_selected_get_any();
          gfloat ex, ey;
          gfloat rot;
          rot = clutter_actor_get_rotation (actor, CLUTTER_Y_AXIS, NULL, NULL, NULL);
          ex = event->motion.x;
          ey = event->motion.y;

          rot += (manipulate_x-ex);

          clutter_actor_set_rotation (actor, CLUTTER_Y_AXIS, rot, 0,0,0);
          cs_prop_tweaked (G_OBJECT (actor), "rotation-angle-y");

          manipulate_x=ex;
          manipulate_y=ey;
        }
        break;
      case CLUTTER_BUTTON_RELEASE:
        selection_to_roty_commands (redo);
        if (start_x != manipulate_x
            || start_y != manipulate_y)
          cs_history_add ("rotate actors around y", redo->str, undo->str);
        g_string_free (undo, TRUE);
        g_string_free (redo, TRUE);
        undo = redo = NULL;

        cs_dirtied ();
        clutter_actor_queue_redraw (stage);
        g_signal_handlers_disconnect_by_func (stage, manipulate_rotate_y_capture, data);
      default:
        break;
    }
  return TRUE;
}

gboolean cs_rotate_y_start (ClutterActor  *actor,
                            ClutterEvent  *event)
{
  manipulate_x = event->button.x;
  manipulate_y = event->button.y;

  undo = g_string_new ("");
  redo = g_string_new ("");
  selection_to_roty_commands (undo);

  g_signal_connect (clutter_actor_get_stage (actor), "captured-event",
                    G_CALLBACK (manipulate_rotate_y_capture), actor);
  return TRUE;
}

