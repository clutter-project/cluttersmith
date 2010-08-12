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

#include <stdlib.h>
#include "cluttersmith.h"

static void cs_copy_directory (GFile *src_dir,
                               GFile *dst_dir)
{
  GFileEnumerator * file_enumerator;
  GFileInfo       * info;
  g_file_make_directory_with_parents (dst_dir, NULL, NULL);
  file_enumerator = g_file_enumerate_children (src_dir,
                                               "*",G_FILE_QUERY_INFO_NONE,
                                               NULL, NULL);

  while ((info = g_file_enumerator_next_file (file_enumerator, NULL, NULL)))
    {
      GFile *src, *dst;
      src = g_file_get_child (src_dir, g_file_info_get_name (info));
      dst = g_file_get_child (dst_dir, g_file_info_get_name (info));

      g_file_copy (src, dst, G_FILE_COPY_OVERWRITE, NULL, NULL, NULL, NULL);
      g_object_unref (src);
      g_object_unref (dst);
    }
  g_object_unref (file_enumerator);
}

static void cs_copy_dir (const gchar *name)
{
  gchar *dst_path;
  gchar *src_path;
  GFile *dst_dir;
  GFile *src_dir;

  src_path = g_strdup_printf ("%s%s", PKGDATADIR, name);
  dst_path = g_strdup_printf ("%s/cluttersmith/%s", g_get_user_data_dir (), name);
  src_dir = g_file_new_for_path (src_path);
  dst_dir = g_file_new_for_path (dst_path);
  cs_copy_directory (src_dir, dst_dir);

  g_object_unref (dst_dir);
  g_object_unref (src_dir);
  g_free (src_path);
  g_free (dst_path);
}

void cs_user_install (void)
{
  gchar *user_path;
  gchar *sys_path;
  gchar *sys_version = NULL;
  gchar *user_version = NULL;
  sys_path = g_strdup_printf ("%s%s", PKGDATADIR, "stamp");
  user_path = g_strdup_printf ("%s/cluttersmith/stamp", g_get_user_data_dir ());

  g_file_get_contents (sys_path, &sys_version, NULL, NULL);
  g_file_get_contents (user_path, &user_version, NULL, NULL);

  if (!user_version || !sys_version ||
      !g_str_equal (user_version, sys_version))
    {
      if (sys_version)
        {
          GFile *foo = g_file_new_for_path (user_path);
          GFile *parent = g_file_get_parent (foo);
          g_file_make_directory_with_parents (parent, NULL, NULL);
          g_object_unref (parent);
          g_object_unref (foo);
          g_file_set_contents (user_path, sys_version, -1, NULL);
        }
      g_print ("Installing files in %s/cluttersmith\n", g_get_user_data_dir ());
      cs_copy_dir ("templates");
      cs_copy_dir ("annotation-templates");
      cs_copy_dir ("docs");
    }

  if (user_version)
    g_free (user_version);
  if (sys_version)
    g_free (sys_version);

  g_free (sys_path);
  g_free (user_path);
}
