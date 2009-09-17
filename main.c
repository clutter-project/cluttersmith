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


/* Global structure containing information parsed from commandline parameters */
static struct
{
  gboolean  fullscreen;
  gchar    *bg_color;
  gint      width, height;
  gchar    *path;
  gchar    *id;
  gchar    *timeline;
}
args =
{
  FALSE,
  "gray",
  1024, 600,
  "about",
  "actor",
  NULL
};

/* using global variables, this is needed at least for the ClutterScript to avoid
 * possible behaviours to be destroyed when the script is destroyed.
 */
static ClutterActor    *stage;
static ClutterScript   *script   = NULL;

gboolean
parse_args (gchar **argv)
{
  gchar **arg = argv + 1;

  while (*arg)
    {
      if (g_str_equal (*arg, "-h") ||
          g_str_equal (*arg, "--help"))
        {
usage:
          g_print ("\nUsage: %s [options] %s\n\n",
                   argv[0], args.path ? args.path : "<clutterscript>");
          g_print ("  -s <widthXheight>       stage size                  (%ix%i)\n",
                   args.width, args.height);
          g_print ("  -fs                     run fullscreen              (%s)\n",
                   args.fullscreen ? "TRUE" : "FALSE");
          g_print ("  -bg <color>             stage color                 (%s)\n",
                   args.bg_color);
          g_print ("  -id <actor id>          which actor id to show      (%s)\n",
                   args.id ? args.id : "NULL");
          g_print ("  -timeline <timeline id> a timeline to play          (%s)\n",
                   args.timeline ? args.timeline : "NULL");
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
      else if (g_str_equal (*arg, "-bg"))
        {
          arg++; g_assert (*arg);
          args.bg_color = *arg;
        }
      else if (g_str_equal (*arg, "-id"))
        {
          arg++; g_assert (*arg);
          args.id = *arg;
        }
      else if (g_str_equal (*arg, "-timeline"))
        {
          arg++; g_assert (*arg);
          args.timeline = *arg;
        }
      else if (g_str_equal (*arg, "-fs"))
        {
          args.fullscreen = TRUE;
        }
      else
        {
          args.path = *arg;
        }
      arg++;
    }
  if (args.path == NULL)
    {
      g_print ("Error parsing commandline: no clutterscript provided\n");
      goto usage;
    }
  return TRUE;
}

static ClutterActor *
initialize_stage ()
{
  ClutterActor *stage;
  ClutterColor  color;

  stage = clutter_stage_get_default ();
  clutter_color_from_string (&color, args.bg_color);
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
  cluttersmith_open_layout (args.path);
  clutter_actor_queue_redraw (clutter_stage_get_default());
  return FALSE;
}

#ifndef COMPILEMODULE
gint
main (gint    argc,
      gchar **argv)
{
  clutter_init (&argc, &argv);
  gst_init (&argc, &argv);

  if (!parse_args (argv))
    return -1;

  stage = initialize_stage ();

  script = clutter_script_new ();
  /*load_script (args.path);*/

  g_timeout_add (100, idle_add_stage, stage);
  g_timeout_add (500, idle_load_default, NULL);

  clutter_main ();
  return 0;
}
#endif
