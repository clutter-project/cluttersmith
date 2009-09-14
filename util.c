#include <clutter/clutter.h>

extern ClutterActor *parasite_root;
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
    actor = CLUTTER_ACTOR (clutter_script_get_object (script, "parasite"));
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
  if (!actor || !CLUTTER_IS_CONTAINER (actor))
    return;
  children = clutter_container_get_children (CLUTTER_CONTAINER (actor));
  for (c=children; c; c = c->next)
    {
#if 0
      if (CLUTTER_IS_CONTAINER (c->data))
        {
          GList *children, *b;
          children = clutter_container_get_children (CLUTTER_CONTAINER (c->data));
          for (b=children; b; b = b->next)
            {
              clutter_actor_destroy (b->data);
            }
          g_list_free (children);
        }
#endif
      clutter_actor_destroy (c->data);
    }
  g_list_free (children);
}

ClutterScript *util_get_script (ClutterActor *actor)
{
  ClutterScript *script;
  ClutterActor *root;
  if (!actor)
    return NULL;
  root = util_get_root (actor);
  if (!root)
    return NULL;
  script = g_object_get_data (G_OBJECT (root), "clutter-script");
  return script;
}


/* replaces a toplevel (as in loaded scripts) container with id "content"
 * with a newly loaded script.
 */
void util_replace_content2 (ClutterActor  *actor,
                            const gchar *name,
                            const gchar *new_script)
{
  ClutterActor *iter;
  ClutterScript *script = util_get_script (actor);
  ClutterActor *content;

  if (script)
    content = CLUTTER_ACTOR (clutter_script_get_object (script, name));


  if (!content)
    {
      iter = util_get_root (actor);
      /* check the parent script, if any, as well */
      if (iter)
        {
          iter = clutter_actor_get_parent (iter);
          if (iter)
            {
              script = util_get_script (iter);
              if (script)
                content = CLUTTER_ACTOR (clutter_script_get_object (script, name));
            }
        }
    }
  if (!content)
    {
      GList *children, *c;
      content = clutter_stage_get_default ();
      children = clutter_container_get_children (CLUTTER_CONTAINER (content));

      for (c=children; c; c = c->next)
        {
          if (g_object_get_data (c->data, "clutter-smith") == NULL)
            clutter_actor_destroy (c->data);
        }
      g_list_free (children);
      if (new_script)
        clutter_container_add_actor (CLUTTER_CONTAINER (content), util_load_json (new_script));

      return;
    }

  util_remove_children (content);
  if (new_script)
    clutter_container_add_actor (CLUTTER_CONTAINER (content), util_load_json (new_script));
  return;
}

/* replaces a toplevel (as in loaded scripts) container with id "content"
 * with a newly loaded script.
 */
gboolean util_replace_content (ClutterActor  *actor)
{
  g_print ("%s!\n", G_STRFUNC);
  util_replace_content2 (actor, "content", clutter_actor_get_name (actor));
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
      default:
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
  clutter_actor_raise_top (actor);
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

void no_pick (ClutterActor       *actor,
              const ClutterColor *color)
{
  g_signal_stop_emission_by_name (actor, "pick");
}



typedef struct TransientValue {
  gchar  *name;
  GValue  value;
  GType   value_type;
} TransientValue;

static GList *transient_values = NULL;

static gchar *whitelist[]={"depth", "opacity",
                           "scale-x","scale-y", "anchor-x", "color",
                           "anchor-y", "rotation-angle-z",
                           "name", "reactive",
                           NULL};


void util_build_transient (ClutterActor *actor)
{
  GParamSpec **properties;
  GParamSpec **actor_properties;
  guint        n_properties;
  guint        n_actor_properties;
  gint         i;

  properties = g_object_class_list_properties (
                     G_OBJECT_GET_CLASS (actor),
                     &n_properties);
  actor_properties = g_object_class_list_properties (
                     G_OBJECT_GET_CLASS (actor),
                     &n_properties);
  actor_properties = g_object_class_list_properties (
            G_OBJECT_CLASS (g_type_class_ref (CLUTTER_TYPE_ACTOR)),
            &n_actor_properties);


  for (i = 0; i < n_properties; i++)
    {
      gint j;
      gboolean skip = FALSE;

        {
          for (j=0;j<n_actor_properties;j++)
            {
              /* ClutterActor contains so many properties that we restrict our view a bit,
               * applying all values seems to make clutter hick-up as well. 
               */
              if (actor_properties[j]==properties[i])
                {
                  gchar *whitelist2[]={"x","y","width","height", NULL};
                  gint k;
                  skip = TRUE;
                  for (k=0;whitelist[k];k++)
                    if (g_str_equal (properties[i]->name, whitelist[k]))
                      skip = FALSE;
                  for (k=0;whitelist2[k];k++)
                    if (g_str_equal (properties[i]->name, whitelist2[k]))
                      skip = FALSE;
                }
            }
        }
      if (g_str_equal (properties[i]->name, "child")||
          g_str_equal (properties[i]->name, "cogl-texture")||
          g_str_equal (properties[i]->name, "cogl-handle"))
        skip = TRUE; /* to avoid duplicated parenting of NbtkButton children. */

      if (!(properties[i]->flags & G_PARAM_READABLE))
        skip = TRUE;

      if (skip)
        continue;

        {
          TransientValue *value = g_new0 (TransientValue, 1);
          value->name = properties[i]->name;
          value->value_type = properties[i]->value_type;
          g_value_init (&value->value, properties[i]->value_type);
          g_object_get_property (G_OBJECT (actor), properties[i]->name, &value->value);

          transient_values = g_list_prepend (transient_values, value);
        }
    }

  g_free (properties);
}


void
util_apply_transient (ClutterActor *actor)
{
  GParamSpec **properties;
  guint        n_properties;
  gint         i;

  properties = g_object_class_list_properties (
                     G_OBJECT_GET_CLASS (actor),
                     &n_properties);

  for (i = 0; i < n_properties; i++)
    {
      gboolean skip = FALSE;
      GList *val;

      if (!(properties[i]->flags & G_PARAM_WRITABLE))
        skip = TRUE;

      if (skip)
        continue;

      for (val=transient_values;val;val=val->next)
        {
          TransientValue *value = val->data;
          if (g_str_equal (value->name, properties[i]->name) &&
                          value->value_type == properties[i]->value_type)
            {
              g_object_set_property (G_OBJECT (actor), properties[i]->name, &value->value);
            }
        }
    }

  g_free (properties);
  transient_values = NULL;
  /* XXX: free list of transient values */
}



/* XXX: needs rewrite to not take the parent */
ClutterActor *util_duplicator (ClutterActor *actor, ClutterActor *parent)
{
  ClutterActor *new_actor;

  new_actor = g_object_new (G_OBJECT_TYPE (actor), NULL);
  util_build_transient (actor);
  util_apply_transient (new_actor);

  /* recurse through children? */
  if (CLUTTER_IS_CONTAINER (new_actor))
    {
      GList *children, *c;
      children = clutter_container_get_children (CLUTTER_CONTAINER (actor));
      for (c = children; c; c = c->next)
        {
          util_duplicator (c->data, new_actor);
        }
      g_list_free (children);
    }

  clutter_container_add_actor (CLUTTER_CONTAINER (parent), new_actor);
  return new_actor;
}


static void get_all_actors_int (GList **list, ClutterActor *actor)
{
  if (util_has_ancestor (actor, parasite_root))
    return;

  if (!CLUTTER_IS_STAGE (actor))
    *list = g_list_prepend (*list, actor);

  if (CLUTTER_IS_CONTAINER (actor))
    {
      GList *children, *c;
      children = clutter_container_get_children (CLUTTER_CONTAINER (actor));
      for (c = children; c; c=c->next)
        {
          get_all_actors_int (list, c->data);
        }
      g_list_free (children);
    }
}

GList *
clutter_container_get_children_recursive (ClutterActor *actor)
{
  GList *ret = NULL;
  get_all_actors_int (&ret, actor);
  return ret;
}

ClutterActor *
util_find_by_id (ClutterActor *stage, const gchar *id)
{
  GList *l, *list = clutter_container_get_children_recursive (stage);
  ClutterActor *ret = NULL;
  for (l=list;l && !ret;l=l->next)
    {
      if (g_str_equal (clutter_scriptable_get_id (CLUTTER_SCRIPTABLE (l->data)), "id"))
        {
          ret = l->data;
        }
    }
  g_list_free (list);
  return ret;
}
