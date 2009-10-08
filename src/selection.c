#include <clutter/clutter.h>
#include <stdlib.h>
#include <string.h>
#include "cluttersmith.h"
#include "util.h"

static GList *selected = NULL;

void cs_selected_init (void)
{
  /*selected = g_hash_table_new_full (g_direct_hash, g_direct_equal, NULL, NULL);*/
}

GList *cs_selected_get_list (void)
{
  return g_list_copy (selected);
}

static void update_active_actor (void)
{
  if (g_list_length (selected)==1)
    {
      cs_set_active (cs_selected_get_any ());
    }
  else
    {
      cs_set_active (NULL);
    }
  if (g_list_length (selected)>0)
    clutter_stage_set_key_focus (CLUTTER_STAGE (clutter_actor_get_stage (cluttersmith->parasite_root)), NULL);
}

void cs_selected_add (ClutterActor *actor)
{
  if (!g_list_find (selected, actor))
    selected = g_list_append (selected, actor);
  update_active_actor ();
}

void cs_selected_remove (ClutterActor *actor)
{
  if (g_list_find (selected, actor))
    selected = g_list_remove (selected, actor);
  update_active_actor ();
}

void cs_selected_foreach (GCallback cb, gpointer data)
{
  void (*each)(ClutterActor *actor, gpointer data)=(void*)cb;
  GList *s;

  for (s = selected; s; s = s->next)
    {
      ClutterActor *actor = s->data;
      if (actor != clutter_actor_get_stage (actor))
        each(actor, data);
    }
}

gpointer cs_selected_match (GCallback match_fun, gpointer data)
{
  gpointer ret = NULL;
  ret = cs_list_match (selected, match_fun, data);
  return ret;
}

void cs_selected_clear (void)
{
  if (selected)
    g_list_free (selected);
  selected = NULL;
  update_active_actor ();
}


gint cs_selected_count (void)
{
  return g_list_length (selected);
}


gboolean cs_selected_has_actor (ClutterActor *actor)
{
  if (g_list_find (selected, actor))
    return TRUE;
  return FALSE;
}

ClutterActor *cs_selected_get_any (void)
{
  if (selected)
    return selected->data;
  return NULL;
}

