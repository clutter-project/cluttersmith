#include <clutter/clutter.h>

static GHashTable *layouts = NULL;

static void util_init (void)
{
  if (layouts)
    return;
  layouts = g_hash_table_new_full (g_str_hash, g_str_equal, NULL, NULL);
}

static ClutterActor *util_get_root (ClutterActor *actor)
{
  ClutterActor *iter = actor;
  while (iter)
    {
      if (g_object_get_data (G_OBJECT (iter), "clutter-script"))
        {
          return iter;
        }
      iter = clutter_actor_get_parent (iter);
    }
  return NULL;
}

gboolean util_dismiss_cb (ClutterActor *actor, ClutterEvent *event, gpointer data)
{
  actor = util_get_root (actor);
  if (actor)
    clutter_actor_hide (actor);
  return TRUE;
}

ClutterActor *util_load_json (const gchar *name)
{
  ClutterActor *actor;
  ClutterScript *script = clutter_script_new ();

  if (g_file_test (name, G_FILE_TEST_IS_REGULAR))
    {
      clutter_script_load_from_file (script, name, NULL);
    }
  else
    {
      gchar *path;
      path = g_strdup_printf ("json/%s", name);
      if (g_file_test (path, G_FILE_TEST_IS_REGULAR))
        {
          clutter_script_load_from_file (script, path, NULL);
        }
      else
        {
          g_warning ("Didn't find requested clutter script %s", name);
          return NULL;
        }
    }

  actor = CLUTTER_ACTOR (clutter_script_get_object (script, "actor"));
  if (!actor)
    {
      g_warning ("failed loading from json %s\n", name);
    }
  clutter_script_connect_signals (script, script);
  g_object_set_data_full (G_OBJECT (actor), "clutter-script", script, g_object_unref);
  return actor;
}

/* show a singleton actor with name name */
ClutterActor *util_show (const gchar *name)
{
  ClutterActor *actor;
  util_init ();
  actor = g_hash_table_lookup (layouts, name);
  
  if (!actor)
    {
      actor = util_load_json (name);
      if (actor)
        {
          clutter_group_add (CLUTTER_GROUP (clutter_stage_get_default ()), actor);
          g_hash_table_insert (layouts, g_strdup (name), actor);
        }
      else
        {
          g_print ("failed to load layout %s\n", name);
          return NULL;
        }
    }

  clutter_actor_set_opacity (actor, 0);
  clutter_actor_show (actor);
  clutter_actor_animate (actor, CLUTTER_LINEAR, 200, "opacity", 0xff, NULL);
  clutter_actor_raise_top (actor);
  return actor;
}

gboolean util_show_cb (ClutterActor  *actor,
                      ClutterEvent  *event)
{
  const gchar *name = clutter_actor_get_name (actor);
  util_show (name);
  return TRUE;
}

/* utility function to remove children of a container.
 */
void util_remove_children (ClutterActor *actor)
{
  GList *children, *c;
  children = clutter_container_get_children (CLUTTER_CONTAINER (actor));
  for (c=children; c; c = c->next)
    {
      clutter_actor_destroy (c->data);
    }
  g_list_free (children);
}

ClutterScript *util_get_script (ClutterActor *actor)
{
  ClutterScript *script = g_object_get_data (G_OBJECT (util_get_root (actor)),
                                             "clutter-script");
  return script;
}


/* replaces a toplevel (as in loaded scripts) container with id "content"
 * with a newly loaded script.
 */
void util_replace_content2 (ClutterActor  *actor,
                           const gchar *name)
{
  ClutterScript *script = util_get_script (actor);
  ClutterActor *content;

  content = CLUTTER_ACTOR (clutter_script_get_object (script, "content"));

  if (!content)
    {
      actor = util_get_root (actor);
      g_assert (actor);
      /* check the parent script, if any, as well */
      if (actor)
        {
          actor = clutter_actor_get_parent (actor);
          script = util_get_script (actor);
          content = CLUTTER_ACTOR (clutter_script_get_object (script, "content"));
        }
    }
  if (!content)
    {
      g_warning ("failed to find id content");
      return;
    }
  util_remove_children (content);
  clutter_container_add_actor (CLUTTER_CONTAINER (content), util_load_json (name));
  return;
}

/* replaces a toplevel (as in loaded scripts) container with id "content"
 * with a newly loaded script.
 */
gboolean util_replace_content (ClutterActor  *actor)
{
  g_print ("%s!\n", G_STRFUNC);
  util_replace_content2 (actor, clutter_actor_get_name (actor));
  return TRUE;
}

static guint movable_capture_handler = 0;
static gint movable_x;
static gint movable_y;

static gboolean
movable_capture (ClutterActor *stage, ClutterEvent *event, gpointer data)
{
  switch (event->any.type)
    {
      case CLUTTER_MOTION:
        {
          gfloat x, y;
          clutter_actor_get_position (data, &x, &y);
          x-= movable_x-event->motion.x;
          y-= movable_y-event->motion.y;
          clutter_actor_set_position (data, x, y);

          movable_x=event->motion.x;
          movable_y=event->motion.y;
        }
        break;
      case CLUTTER_BUTTON_RELEASE:
        g_signal_handler_disconnect (stage,
                                     movable_capture_handler);
        movable_capture_handler = 0;
        break;
    }
  return TRUE;
}

gboolean util_movable_press (ClutterActor  *actor,
                             ClutterEvent  *event)
{
  movable_x = event->button.x;
  movable_y = event->button.y;

  movable_capture_handler = 
     g_signal_connect (clutter_actor_get_stage (actor), "captured-event",
                       G_CALLBACK (movable_capture), actor);
  return TRUE;
}


gboolean util_has_ancestor (ClutterActor *actor,
                            ClutterActor *ancestor)
{
  while (actor)
    {
      if (actor == ancestor)
        return TRUE;
      actor = clutter_actor_get_parent (actor);
    }
  return FALSE;
}


void parasite_pick (ClutterActor       *actor,
                    const ClutterColor *color)
{
  clutter_container_foreach (CLUTTER_CONTAINER (actor), CLUTTER_CALLBACK (clutter_actor_paint), NULL);
  g_signal_stop_emission_by_name (actor, "pick");
}
