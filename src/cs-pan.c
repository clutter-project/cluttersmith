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

static gfloat manipulate_x;
static gfloat manipulate_y;

static gfloat manipulate_pan_start_x = 0;
static gfloat manipulate_pan_start_y = 0;


static gboolean
manipulate_pan_capture (ClutterActor *stage,
                        ClutterEvent *event,
                        gpointer      data)
{
  switch (event->any.type)
    {
      case CLUTTER_MOTION:
        {
          gfloat ex = event->motion.x, ey = event->motion.y;
          gfloat originx, originy;

          g_object_get (cs, "origin-x", &originx,
                            "origin-y", &originy,
                            NULL);
          originx += manipulate_x-ex;
          originy += manipulate_y-ey;
          g_object_set (cs, "origin-x", originx,
                            "origin-y", originy,
                            NULL);

          manipulate_x=ex;
          manipulate_y=ey;
        }
        break;
      case CLUTTER_BUTTON_RELEASE:
        clutter_actor_queue_redraw (stage);
        g_signal_handlers_disconnect_by_func (stage, manipulate_pan_capture, data);
        if (manipulate_x == manipulate_pan_start_x &&
            manipulate_y == manipulate_pan_start_y)
          {
            cs_zoom (!(event->button.modifier_state & CLUTTER_SHIFT_MASK), manipulate_x, manipulate_y);
          }
      default:
        break;
    }
  return TRUE;
}

gboolean manipulate_pan_start (ClutterEvent  *event)
{
  manipulate_x = event->button.x;
  manipulate_y = event->button.y;

  manipulate_pan_start_x = manipulate_x;
  manipulate_pan_start_y = manipulate_y;

  g_signal_connect (clutter_actor_get_stage (event->any.source), "captured-event",
                    G_CALLBACK (manipulate_pan_capture), NULL);
  return TRUE;
}



