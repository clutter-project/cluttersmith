#include <clutter/clutter.h>
#include <stdlib.h>
#include <string.h>
#include "cluttersmith.h"
#include "util.h"

static GHashTable *selected = NULL;

void cluttersmith_selected_init (void)
{
  selected = g_hash_table_new_full (g_direct_hash, g_direct_equal, NULL, NULL);
}

GList *cluttersmith_selected_get_list (void)
{
  GList *ret = g_hash_table_get_keys (selected);
  return ret;
}


void cluttersmith_selected_add (ClutterActor *actor)
{
  g_hash_table_insert (selected, actor, actor);
}

void cluttersmith_selected_remove (ClutterActor *actor)
{
  g_hash_table_remove (selected, actor);
}



void cluttersmith_selected_foreach (GCallback cb, gpointer data)
{
  void (*each)(ClutterActor *actor, gpointer data)=(void*)cb;
  GList *s, *selected;
  selected = cluttersmith_selected_get_list ();
  s=selected;
  for (s=selected; s; s=s->next)
    {
      ClutterActor *actor = s->data;
      if (actor == clutter_actor_get_stage (actor))
        continue;
      each(actor, data);
    }
  g_list_free (selected);
}

gpointer cluttersmith_selected_match (GCallback match_fun, gpointer data)
{
  gpointer ret = NULL;
  GList *selected;
  selected = cluttersmith_selected_get_list ();
  ret = util_list_match (selected, match_fun, data);
  g_list_free (selected);
  return ret;
}

void cluttersmith_selected_clear (void)
{
  g_hash_table_remove_all (selected);
}


gint cluttersmith_selected_count (void)
{
  return g_hash_table_size (selected);
}


gboolean cluttersmith_selected_has_actor (ClutterActor *actor)
{
  return g_hash_table_lookup (selected, actor)!=NULL;
}

ClutterActor *cluttersmith_selected_get_any (void)
{
  GHashTableIter      iter;
  gpointer            key = NULL, value;

  g_hash_table_iter_init (&iter, selected);
  g_hash_table_iter_next (&iter, &key, &value);
  return key;
}

