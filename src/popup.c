/*
 * Hornsey - Moblin Media Player.
 * Copyright © 2009 Intel Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU Lesser General Public License,
 * version 2.1, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <nbtk/nbtk.h>
#include "popup.h"

static ClutterActor *popup        = NULL;
static ClutterActor *popup_parent = NULL;
static guint         popuphandler = 0;

gboolean
hrn_actor_has_ancestor (ClutterActor *actor, ClutterActor *ancestor)
{
  ClutterActor *iter;

  if (!actor || !ancestor)
    return FALSE;
  for (iter = actor;
       iter;
       iter = clutter_actor_get_parent (iter))
    {
      if (iter == ancestor)
        return TRUE;
    }
  return FALSE;
}

static gboolean delayed_destroy (gpointer actor)
{
  clutter_actor_destroy (actor);
  return FALSE;
}

static void
destroy_when_done (ClutterAnimation *anim, ClutterActor     *actor)
{
  g_idle_add (delayed_destroy, actor);
}

void
popup_close (void)
{
  if (!popup)
    return;
  if (popuphandler)
    {
      g_signal_handler_disconnect (popup_parent, popuphandler);
      popuphandler = 0;
    }
  clutter_actor_animate (popup, CLUTTER_LINEAR, 150, "scale-x", 0.001,
                         "scale-y", 0.001,
                         "signal::completed", destroy_when_done, popup,
                         NULL);
  popup = NULL;
}

gboolean
popup_capture (ClutterActor *stage, ClutterEvent *event, gpointer popup)
{
  switch (event->type)
    {
      case CLUTTER_LEAVE: /* to allow dehovering what we right-clicked */
        return FALSE;

      case CLUTTER_BUTTON_RELEASE: /* clicks outside the menu makes it go away
                                    */
        if (!hrn_actor_has_ancestor (clutter_event_get_source (event), popup))
          {
            popup_close ();
            return TRUE;
          }

      case CLUTTER_BUTTON_PRESS:
      case CLUTTER_MOTION:
      case CLUTTER_ENTER:
        if (hrn_actor_has_ancestor (clutter_event_get_source (event), popup))
          return FALSE;
        break;

      default:
        break;
    }
  return TRUE;
}

void
popup_actor (ClutterActor *stage, gint x, gint y, ClutterActor *actor)
{
  gfloat        w2, h2;
  gint          w, h;

  clutter_actor_raise_top (stage);
  /* install a capture that only allows events destined to
   * children of the popped up actor to come through
   */
  clutter_group_add (stage, actor);
  clutter_actor_get_size (actor, &w2, &h2);
  w = w2;
  h = h2;
  clutter_actor_set_position (actor, x, y);

  clutter_actor_set_scale (actor, 0.01, 0.01);


  /* Avoid placing the popup off stage
   * if gravity could be specified in floating point values, this could be
   * handled more elegantly
   */
  if (x + w / 2 > clutter_actor_get_width (stage))
    {
      if (y + h / 2 > clutter_actor_get_height (stage))
        {
          clutter_actor_set_anchor_point_from_gravity (
            actor, CLUTTER_GRAVITY_SOUTH_EAST);
        }
      else if (y - h / 2 < 0)
        {
          clutter_actor_set_anchor_point_from_gravity (
            actor, CLUTTER_GRAVITY_NORTH_EAST);
        }
      else
        {
          clutter_actor_set_anchor_point_from_gravity (actor,
                                                       CLUTTER_GRAVITY_EAST);
        }
    }
  else if (x - w / 2 < 0)
    {
      if (y + h / 2 > clutter_actor_get_height (stage))
        {
          clutter_actor_set_anchor_point_from_gravity (
            actor, CLUTTER_GRAVITY_SOUTH_WEST);
        }
      else if (y - h / 2 < 0)
        {
          clutter_actor_set_anchor_point_from_gravity (
            actor, CLUTTER_GRAVITY_NORTH_WEST);
        }
      else
        {
          clutter_actor_set_anchor_point_from_gravity (actor,
                                                       CLUTTER_GRAVITY_WEST);
        }
    }
  else
    {
      if (y + h / 2 > clutter_actor_get_height (stage))
        {
          clutter_actor_set_anchor_point_from_gravity (actor,
                                                       CLUTTER_GRAVITY_SOUTH);
        }
      else if (y - h / 2 < 0)
        {
          clutter_actor_set_anchor_point_from_gravity (actor,
                                                       CLUTTER_GRAVITY_NORTH);
        }
      else
        {
          clutter_actor_set_anchor_point_from_gravity (actor,
                                                       CLUTTER_GRAVITY_CENTER);
        }
    }

  clutter_actor_animate (actor, CLUTTER_LINEAR, 150, "scale-x", 1.0, "scale-y",
                         1.0,
                         NULL);
  popup = actor;
  popup_parent = stage;
  popuphandler =
    g_signal_connect_after (popup_parent, "captured-event", G_CALLBACK (
                              popup_capture), actor);
}


void
popup_actor_fixed (ClutterActor *stage, gint x, gint y, ClutterActor *actor)
{
  gfloat        w2, h2;
  gint          w, h;

  clutter_actor_raise_top (stage);
  clutter_actor_raise_top (actor);
  /* install a capture that only allows events destined to
   * children of the popped up actor to come through
   */
  clutter_group_add (stage, actor);
  clutter_actor_get_size (actor, &w2, &h2);
  w = w2;
  h = h2;
  clutter_actor_set_position (actor, x, y);
  popup = actor;

  popup_parent = stage;
  popuphandler =
    g_signal_connect_after (stage, "captured-event", G_CALLBACK (
                              popup_capture), actor);
}


ClutterActor *
popup_actions (gpointer *actions, gpointer userdata)
{
  gint          i;
  gint          max_width = 0;
  ClutterActor *group     = CLUTTER_ACTOR (g_object_new (NBTK_TYPE_GRID,
                                                         "width", 0.0,
                                                         NULL));

  nbtk_widget_set_style_class_name (NBTK_WIDGET (group), "HrnPopup");

  for (i = 0; actions[i]; i += 2)
    {
      gfloat        w, h;
      ClutterActor *label;

      label = CLUTTER_ACTOR (nbtk_button_new_with_label (actions[i]));
      clutter_container_add_actor (CLUTTER_CONTAINER (group), label);

      if (actions[i + 1])
        {
          g_signal_connect_swapped (label, "clicked",
                                    G_CALLBACK (actions[i + 1]), userdata);
        }
      else
        {
          g_signal_connect (label, "clicked", G_CALLBACK (
                              popup_close), userdata);
        }
      clutter_actor_get_size (label, &w, &h);
      if (w + 20 > max_width)
        max_width = w + 20;
    }

  clutter_actor_set_width (group, max_width + 44);
  {
    GList *i, *children =
      clutter_container_get_children (CLUTTER_CONTAINER (group));
    for (i = children; i; i = i->next)
      clutter_actor_set_width (i->data, max_width);
    g_list_free (children);
  }
  clutter_actor_set_reactive (group, TRUE);
  return group;
}
