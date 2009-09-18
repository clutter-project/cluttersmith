#include <clutter/clutter.h>
#include <stdlib.h>
#include <string.h>
#include "cluttersmith.h"
#include "util.h"

void cluttersmith_link_text_changed (ClutterActor *actor)
{
  const gchar *new_text = clutter_text_get_text (CLUTTER_TEXT (actor));
  gchar *new_link = g_strdup_printf ("link=%s", new_text);
  clutter_actor_set_name (clutter_actor_get_parent (clutter_actor_get_parent (actor)), new_link);
}

void cluttersmith_link_follow (ClutterActor *actor)
{
  const gchar *link = clutter_actor_get_name (clutter_actor_get_parent (clutter_actor_get_parent (actor)));
  cluttersmith_open_layout (link + 5);
}
