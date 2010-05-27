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


#include <gdk-pixbuf/gdk-pixbuf.h>

#include <clutter/clutter.h>
#include "cluttersmith.h"

static void
free_data (guchar  *pixels,
           gpointer  data)
{
  g_free (pixels);
}

void export_png (const gchar *scene,
                 const gchar *png)
{
  GdkPixbuf *pixbuf;
  GError    *error = NULL;
  guchar    *pixels;
  gfloat     x, y;
  gint       width, height;
  guint      old_mode = cs_get_ui_mode ();

  /* disable chrome */

  cs_set_ui_mode (CS_UI_MODE_BROWSE);

  cluttersmith_load_scene (scene);
  clutter_actor_get_position (cs->fake_stage, &x, &y);

  g_object_get (cs, "canvas-width", &width,
                    "canvas-height", &height,
                    NULL);

  g_print ("exporting scene %s to %s  %f,%f %dx%d\n", scene, png,
           x, y, width, height);

  pixels = clutter_stage_read_pixels (
             (ClutterStage *)clutter_actor_get_stage (cs->fake_stage),
             x, y, width, height);
  pixbuf = gdk_pixbuf_new_from_data (pixels,
                                     GDK_COLORSPACE_RGB,
                                     TRUE,
                                     8,
                                     width,
                                     height,
                                     ((gint)(width)) * 4,
                                     free_data,
                                     NULL);

  gdk_pixbuf_save (pixbuf, png, "png", &error, NULL);
  if (error)
    {
      g_warning ("Failed to save scene to file: %s: %s\n",
                 png, error->message);
      g_error_free (error);
    }

  g_object_unref (pixbuf);
  cs_set_ui_mode (old_mode);
  /* re-enable chrome */
}

void export_pdf (const gchar *pdf)
{
  g_print ("exporting scenes to %s\n", pdf);
}

void cs_export_png (void)
{
  export_png (mx_entry_get_text (MX_ENTRY (cs->scene_title)),
              mx_entry_get_text (MX_ENTRY (cs_find_by_id_int (
                                 clutter_actor_get_stage (cs->fake_stage), 
                                 "cs-png-path"))));

}

void cs_export_pdf (void)

{
  export_pdf (mx_entry_get_text (MX_ENTRY (cs_find_by_id_int (
                                 clutter_actor_get_stage (cs->fake_stage), 
        "cs-pdf-path"))));
}
