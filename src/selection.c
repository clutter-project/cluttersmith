#include <clutter/clutter.h>
#include <stdlib.h>
#include <string.h>
#include "cluttersmith.h"
#include "util.h"

static GList *selected = NULL;

void cluttersmith_selected_init (void)
{
  /*selected = g_hash_table_new_full (g_direct_hash, g_direct_equal, NULL, NULL);*/
}

GList *cluttersmith_selected_get_list (void)
{
  return g_list_copy (selected);
}

static void update_active_actor (void)
{
  if (g_list_length (selected)==1)
    {
      cluttersmith_set_active (cluttersmith_selected_get_any ());
    }
  else
    {
      cluttersmith_set_active (NULL);
    }
}

void cluttersmith_selected_add (ClutterActor *actor)
{
  if (!g_list_find (selected, actor))
    selected = g_list_append (selected, actor);
  update_active_actor ();
}

void cluttersmith_selected_remove (ClutterActor *actor)
{
  if (g_list_find (selected, actor))
    selected = g_list_remove (selected, actor);
  update_active_actor ();
}

void cluttersmith_selected_foreach (GCallback cb, gpointer data)
{
  void (*each)(ClutterActor *actor, gpointer data)=(void*)cb;
  GList *s;
  s=selected;
  for (s=selected; s; s=s->next)
    {
      ClutterActor *actor = s->data;
      if (actor == clutter_actor_get_stage (actor))
        continue;
      each(actor, data);
    }
}

gpointer cluttersmith_selected_match (GCallback match_fun, gpointer data)
{
  gpointer ret = NULL;
  ret = util_list_match (selected, match_fun, data);
  return ret;
}

void cluttersmith_selected_clear (void)
{
  if (selected)
    g_list_free (selected);
  selected = NULL;
  update_active_actor ();
}


gint cluttersmith_selected_count (void)
{
  return g_list_length (selected);
}


gboolean cluttersmith_selected_has_actor (ClutterActor *actor)
{
  if (g_list_find (selected, actor))
    return TRUE;
  return FALSE;
}

ClutterActor *cluttersmith_selected_get_any (void)
{
  if (selected)
    return selected->data;
  return NULL;
}

