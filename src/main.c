/* ClutterScript viewer, a viewer for displaying clutter scripts or fragments
 * of clutterscript.
 *
 * Copyright 2007 OpenedHand Ltd
 * Authored by Øyvind Kolås <pippin@o-hand.com>
 * Licensed under the GPL v2 or greater.
 *
 */

#include <clutter/clutter.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <glib/gprintf.h>
#include <glib/gstdio.h>
#include "cluttersmith.h"

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

static ClutterActor *
initialize_stage ()
{
  ClutterActor *stage;
  ClutterColor  color;

  stage = clutter_stage_get_default ();
  clutter_color_from_string (&color, "gray");
  clutter_stage_set_color (CLUTTER_STAGE (stage), &color);
  clutter_actor_show (stage);

  if (args.fullscreen)
    clutter_stage_set_fullscreen (CLUTTER_STAGE (stage), TRUE);
  else
    clutter_actor_set_size (stage, args.width, args.height);

  return stage;
}

void gst_init (gpointer, gpointer);

gboolean idle_add_stage (gpointer stage);

/* hack.. */
static gboolean idle_load_default (gpointer data)
{

  if (args.root_path)
    {
      gchar *fullpath = realpath (args.root_path, NULL);
      cluttersmith_set_project_root (fullpath);
      free (fullpath);
    }
  else
    {
      cluttersmith_set_project_root (PKGDATADIR "docs");
    }

  clutter_actor_queue_redraw (clutter_stage_get_default());
  cluttersmith_set_ui_mode (CLUTTERSMITH_UI_MODE_BROWSE);

  return FALSE;
}

#ifndef COMPILEMODULE
gint
main (gint    argc,
      gchar **argv)
{
  ClutterActor    *stage;
  clutter_init (&argc, &argv);
  gst_init (&argc, &argv);

  if (!parse_args (argv))
    return -1;

  stage = initialize_stage ();

  g_timeout_add (100, idle_add_stage, stage);
  g_timeout_add (800, idle_load_default, NULL);
  g_timeout_add (10000, cluttersmith_save_timeout, NULL); /* auto-save */

  clutter_main ();
  return 0;
}
#endif

