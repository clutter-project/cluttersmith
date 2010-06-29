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

#include <clutter/clutter.h>
#include <mx/mx.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <glib/gprintf.h>
#include <glib/gstdio.h>
#include "cluttersmith.h"
#include "cs-util.h"

/* Global structure containing information parsed from commandline parameters
 */
static struct
{
  gboolean  fullscreen;
  gint      width, height;
  gchar    *root_path;
}

args =
{
  FALSE,
  1024, 600,
  NULL,
};


gboolean
parse_args (gchar **argv)
{
  gchar **arg = argv + 1;

  while (*arg)
    {
      if (g_str_equal (*arg, "-h") ||
          g_str_equal (*arg, "--help"))
        {
          g_print ("\nUsage: %s [options] \n\n",
                   argv[0]);
          g_print ("  -s <widthXheight>       stage size                  (%ix%i)\n",
                   args.width, args.height);
          g_print ("  -fs                     run fullscreen              (%s)\n",
                   args.fullscreen ? "TRUE" : "FALSE");
          g_print ("  -h                      this help\n\n");
          return FALSE;
        }
      else if (g_str_equal (*arg, "-s"))
        {
          arg++; g_assert (*arg);
          args.width = atoi (*arg);
          if (strstr (*arg, "x"))
            args.height = atoi (strstr (*arg, "x") + 1);
        }
      else if (g_str_equal (*arg, "-fs"))
        {
          args.fullscreen = TRUE;
        }
      else
        {
          args.root_path = *arg;
        }
      arg++;
    }
  return TRUE;
}

void gst_init (gpointer, gpointer);

void mode_edit2 (void);

#ifndef COMPILEMODULE
  /* when compiling as an LD_PRELOAD module, we do not use main */

static void load_project (void)
{
  if (args.root_path)
    {
      gchar *fullpath = realpath (args.root_path, NULL);
      gchar *bname = basename (fullpath);

      if (g_file_test (args.root_path, G_FILE_TEST_IS_REGULAR))
        {
          cluttersmith_set_project_root (fullpath);
          if (strrchr (bname, '.'))
            *strrchr (bname, '.') = '\0';
          cluttersmith_load_scene (bname);
          free (fullpath);
        }
      else if (g_file_test (args.root_path, G_FILE_TEST_IS_DIR))
        {
          gchar *fullpath = realpath (args.root_path, NULL);
          cluttersmith_set_project_root (fullpath);
          free (fullpath);
        }
      else
        {
          g_print ("Working dir %s does not exist", args.root_path);
        }
    }
  else
    {
      cluttersmith_set_project_root (PKGDATADIR "docs");
      cluttersmith_load_scene ("index");
    }

  mode_edit2 (); /* make cluttersmith go to editing mode */

  clutter_actor_queue_redraw (clutter_stage_get_default());
}

gint
main (gint    argc,
      gchar **argv)
{
  ClutterActor *stage;
  ClutterColor  color;
  gst_init (&argc, &argv);
  clutter_init (&argc, &argv);

  if (!parse_args (argv))
    return -1;

  stage = clutter_stage_get_default ();
  clutter_color_from_string (&color, "gray");
  clutter_stage_set_color (CLUTTER_STAGE (stage), &color);
  clutter_actor_show (stage);

  if (args.fullscreen)
    clutter_stage_set_fullscreen (CLUTTER_STAGE (stage), TRUE);
  clutter_stage_set_user_resizable (CLUTTER_STAGE (stage), TRUE);
  clutter_actor_set_size (stage, 1024, 600);

  cluttersmith_initialize_for_stage (stage);
  load_project ();
  g_timeout_add (10000, cs_save_timeout, NULL); /* auto-save */

  clutter_main ();
  return 0;
}

void* force_link_of_compilation_units_containting[]={
  session_history_init_hack,
  templates_container_init_hack
};

#endif
