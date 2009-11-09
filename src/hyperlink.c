#include <clutter/clutter.h>
#include <mx/mx.h>
#include <stdlib.h>
#include <string.h>
#include "cluttersmith.h"
#include "util.h"

void cs_link_text_changed (ClutterActor *actor)
{
  static gboolean in = FALSE;
  const gchar *new_text;
  new_text = clutter_text_get_text (CLUTTER_TEXT (actor));

  if (!in)
    {
      in = TRUE;
      gchar *new_link = g_strdup_printf ("link=%s", new_text);
      clutter_actor_set_name (clutter_actor_get_parent (actor), new_link);
      mx_button_set_label (MX_BUTTON (clutter_actor_get_parent (actor)), new_text);
      in = FALSE;
    }
}

void cs_link_follow (ClutterActor *actor)
{
  const gchar *link = clutter_actor_get_name (clutter_actor_get_parent (clutter_actor_get_parent (actor)));
  cluttersmith_load_scene (link + 5);
}
