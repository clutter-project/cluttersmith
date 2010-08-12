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
#include <stdlib.h>
#include <string.h>
#include "cluttersmith.h"

static gdouble manipulate_x, manipulate_y;


static gboolean add_stencil_real (gfloat       x,
                                  gfloat       y,
                                  const gchar *path)
{
  ClutterActor *parent;
  ClutterActor *new_actor;

  parent = cs_get_current_container ();
  new_actor = cs_load_json (path);
  
  if (new_actor)
    {
      gchar *name = g_strdup (strrchr (path, '/')+1);

      * (strrchr (name, '.')) = '\0';

      clutter_container_add_actor (CLUTTER_CONTAINER (parent), new_actor);
      clutter_actor_set_position (new_actor, x, y);

      cs_selected_clear ();
      cs_actor_make_id_unique (new_actor, name);
      cs_selected_add (new_actor);

      g_free (name);
    }

  cs_dirtied ();
  return TRUE;
}

static guint stencil_capture_handler = 0;

static gboolean
add_stencil_capture (ClutterActor *stage,
                     ClutterEvent *event,
                     gpointer      data)
{
    switch (event->any.type)
    {
      case CLUTTER_MOTION:
        {
          gfloat delta[2];
          gfloat x, y;
          delta[0]=manipulate_x-event->motion.x;
          delta[1]=manipulate_y-event->motion.y;
          clutter_actor_get_position (data, &x, &y);
          x-= delta[0];
          y-= delta[1];
          clutter_actor_set_position (data, x, y);
          manipulate_x=event->motion.x;
          manipulate_y=event->motion.y;
        }
        break;
      case CLUTTER_BUTTON_RELEASE:
        g_signal_handler_disconnect (stage,
                                     stencil_capture_handler);
        stencil_capture_handler = 0;

        manipulate_x -= clutter_actor_get_x (cs->fake_stage);
        manipulate_y -= clutter_actor_get_y (cs->fake_stage);
        add_stencil_real (manipulate_x, manipulate_y, g_object_get_data (data, "path"));
        clutter_actor_destroy (data);


        clutter_actor_queue_redraw (stage);
      default:
        break;
    }
  return TRUE;
}


static gboolean add_stencil (ClutterActor *actor,
                             ClutterEvent *event,
                             ClutterActor *stencil)
{
  ClutterActor *clone;
  manipulate_x = event->button.x;
  manipulate_y = event->button.y;

  clone = clutter_clone_new (stencil);
  clutter_container_add_actor (CLUTTER_CONTAINER (clutter_actor_get_stage (actor)),
                               clone);

  g_object_set_data (G_OBJECT (clone), "path", g_object_get_data (G_OBJECT (stencil), "path"));
  clutter_actor_set_position (clone, manipulate_x, manipulate_y);

  stencil_capture_handler = 
     g_signal_connect (clutter_actor_get_stage (actor), "captured-event",
                       G_CALLBACK (add_stencil_capture), clone);
  clutter_actor_queue_redraw (clone);
  return TRUE;
}

static void load_path (ClutterActor *actor,
                       const gchar  *path)
{
   GDir *dir = g_dir_open (path, 0, NULL);
   const gchar *name;


   while ((name = g_dir_read_name (dir)))
      {
        ClutterColor  none = {0xff,0xff,0xff,0x00};
        ClutterActor *group;
        ClutterActor *rectangle;

        if (!g_str_has_suffix (name, ".json"))
          continue;

        rectangle = clutter_rectangle_new ();
        group = clutter_group_new ();


        clutter_rectangle_set_color (CLUTTER_RECTANGLE (rectangle), &none);
        clutter_actor_set_reactive (rectangle, TRUE);
          {
            gchar *path2;
            ClutterActor *oi;
            path2 = g_strdup_printf ("%s/%s", path, name);
            oi = cs_load_json (path2);
            if (oi)
              {
                ClutterActor *label;
                ClutterColor  white = {0xff,0xff,0xff,0xff};
#define DIM 100.0

                gfloat width, height;
                gfloat scale;
                gchar *name2 = g_strdup (name);
                clutter_actor_get_size (oi, &width, &height);
                scale = DIM/width;
                if (DIM/height < scale)
                  scale = DIM/height;
                clutter_actor_set_scale (oi, scale, scale);
                clutter_actor_set_size (rectangle, DIM, height*scale);
                if (strrchr (name2, '.'))
                   *strrchr (name2, '.') = '\0';

                label = clutter_text_new_with_text ("Sans 10px", name2);
                g_free (name2);
                clutter_text_set_color (CLUTTER_TEXT (label), &white);

                clutter_container_add_actor (CLUTTER_CONTAINER (group), oi);
                clutter_container_add_actor (CLUTTER_CONTAINER (group), label);
                clutter_container_add_actor (CLUTTER_CONTAINER (group), rectangle);
                clutter_actor_set_y (label, clutter_actor_get_height (rectangle));
                clutter_actor_set_size (group, DIM, height*scale +
                                                            clutter_actor_get_height (label));
                g_object_set_data_full (G_OBJECT (oi), "path", path2, g_free);
                g_signal_connect (rectangle, "button-press-event", G_CALLBACK (add_stencil), oi);
              }
            else
              {
                g_free (path2);
              }
          }
        clutter_container_add_actor (CLUTTER_CONTAINER (actor), group);
      }
    g_dir_close (dir);
}


void templates_container_init_hack (ClutterActor  *actor)
{
  /* we hook this up to the first paint, since no other signal seems to
   * be available to hook up for some additional initialization
   */
  static gboolean done = FALSE; 
  gchar *path;
  if (done)
    return;
  done = TRUE;
  path = g_strdup_printf ("%s/cluttersmith/%s", g_get_user_data_dir (), "templates");
  load_path (actor, path);
  g_free (path);
}

void annotation_templates_container_init_hack (ClutterActor  *actor)
{
  /* we hook this up to the first paint, since no other signal seems to
   * be available to hook up for some additional initialization
   */
  static gboolean done = FALSE; 
  gchar *path;
  if (done)
    return;
  done = TRUE;
  path = g_strdup_printf ("%s/cluttersmith/%s", g_get_user_data_dir (), "annotation-templates");
  load_path (actor, path);
  g_free (path);
}
