#include <clutter/clutter.h>
#include "util.h"

static GHashTable *layouts = NULL;

static void cs_init (void)
{
  if (layouts)
    return;
  layouts = g_hash_table_new_full (g_str_hash, g_str_equal, NULL, NULL);
}

static ClutterActor *cs_get_root (ClutterActor *actor)
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

gboolean cs_dismiss_cb (ClutterActor *actor,
                        ClutterEvent *event,
                        gpointer      data)
{
  actor = cs_get_root (actor);
  if (actor)
    clutter_actor_hide (actor);
  return TRUE;
}

ClutterActor *cs_load_json (const gchar *name)
{
  ClutterActor *actor;
  ClutterScript *script = clutter_script_new ();
  GError *error = NULL;

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
          clutter_script_load_from_file (script, path, &error);
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
      g_warning ("failed loading from json %s, %s\n", name, error?error->message:"");
    }
  clutter_script_connect_signals (script, script);
  g_object_set_data_full (G_OBJECT (actor), "clutter-script", script, g_object_unref);
  return actor;
}

/* show a singleton actor with name name */
ClutterActor *cs_show (const gchar *name)
{
  ClutterActor *actor;
  cs_init ();
  actor = g_hash_table_lookup (layouts, name);
  
  if (!actor)
    {
      actor = cs_load_json (name);
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

gboolean cs_show_cb (ClutterActor  *actor,
                     ClutterEvent  *event)
{
  const gchar *name = clutter_actor_get_name (actor);
  cs_show (name);
  return TRUE;
}

/* utility function to remove children of a container.
 */
void cs_container_remove_children (ClutterActor *actor)
{
  GList *children, *c;
  if (!actor || !CLUTTER_IS_CONTAINER (actor))
    return;
  children = clutter_container_get_children (CLUTTER_CONTAINER (actor));
  for (c=children; c; c = c->next)
    {
      clutter_actor_destroy (c->data);
    }
  g_list_free (children);
}

ClutterScript *cs_get_script (ClutterActor *actor)
{
  ClutterScript *script;
  ClutterActor *root;
  if (!actor)
    return NULL;
  root = cs_get_root (actor);
  if (!root)
    return NULL;
  script = g_object_get_data (G_OBJECT (root), "clutter-script");
  return script;
}

static ClutterActor *empty_scene_new (void)
{
  ClutterActor *group;
  group = clutter_group_new ();
  clutter_scriptable_set_id (CLUTTER_SCRIPTABLE (group), "actor");
  return group;
}

/* replaces a toplevel (as in loaded scripts) container with id "content"
 * with a newly loaded script.
 */
ClutterActor *cs_replace_content2 (ClutterActor  *actor,
                                   const gchar *name,
                                   const gchar *new_script)
{
  ClutterActor *ret = NULL;
  ClutterActor *iter;
  ClutterScript *script = cs_get_script (actor);
  ClutterActor *content = NULL;

  if (script)
    content = CLUTTER_ACTOR (clutter_script_get_object (script, name));


  if (!content)
    {
      iter = cs_get_root (actor);
      /* check the parent script, if any, as well */
      if (iter)
        {
          iter = clutter_actor_get_parent (iter);
          if (iter)
            {
              script = cs_get_script (iter);
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
        clutter_container_add_actor (CLUTTER_CONTAINER (content), ret = cs_load_json (new_script));
      else
        clutter_container_add_actor (CLUTTER_CONTAINER (content), ret = empty_scene_new ());
      return ret;
    }

  cs_container_remove_children (content);
  if (new_script)
    clutter_container_add_actor (CLUTTER_CONTAINER (content), ret = cs_load_json (new_script));
  else
    clutter_container_add_actor (CLUTTER_CONTAINER (content), ret = empty_scene_new ());
  return ret;
}

/* replaces a toplevel (as in loaded scripts) container with id "content"
 * with a newly loaded script.
 */
gboolean cs_replace_content (ClutterActor  *actor)
{
  cs_replace_content2 (actor, "content", clutter_actor_get_name (actor));
  return TRUE;
}

static guint movable_capture_handler = 0;
static gint movable_x;
static gint movable_y;

static gboolean
cs_movable_capture (ClutterActor *stage,
                    ClutterEvent *event,
                    gpointer      data)
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

gboolean cs_movable_press (ClutterActor  *actor,
                           ClutterEvent  *event)
{
  movable_x = event->button.x;
  movable_y = event->button.y;

  movable_capture_handler = 
     g_signal_connect (clutter_actor_get_stage (actor), "captured-event",
                       G_CALLBACK (cs_movable_capture), actor);
  clutter_actor_raise_top (actor);
  return TRUE;
}


gboolean cs_slider_panel_enter_south (ClutterActor  *actor,
                                      ClutterEvent  *event)
{
  gfloat height = clutter_actor_get_height (actor);
  clutter_actor_animate (actor, CLUTTER_LINEAR, 200, "anchor-y", height, NULL);
  return TRUE;
}

gboolean cs_slider_panel_leave (ClutterActor  *actor,
                                ClutterEvent  *event)
{
  clutter_actor_animate (actor, CLUTTER_LINEAR, 200, "anchor-y", 0.0, NULL);
  return TRUE;
}



gboolean cs_actor_has_ancestor (ClutterActor *actor,
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

/* XXX: this list is incomplete */
static gchar *whitelist[]={"depth", "opacity",
                           "scale-x","scale-y", "anchor-x", "color",
                           "anchor-y", "rotation-angle-z", "rotation-angle-x",
                           "rotation-angle_y",
                           "name", "reactive",
                           NULL};

static void transient_free (GList *transient)
{
  if (transient)
    {
      g_list_foreach (transient, (GFunc)(g_free), NULL);
      g_list_free (transient);
    }
}

/* XXX: should ad-hoc special case scriptable "id" */
static GList *
cs_build_transient (ClutterActor *actor)
{
  GParamSpec **properties;
  GParamSpec **actor_properties;
  guint        n_properties;
  guint        n_actor_properties;
  gint         i;
  GList *transient_values = NULL;

  properties = g_object_class_list_properties (
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
          g_str_equal (properties[i]->name, "disable-slicing")||
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

  return transient_values;
}


static void
cs_apply_transient (ClutterActor *actor, GList *transient_values)
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
}

static GList *
cs_build_child_transient (ClutterActor *actor)
{
  GParamSpec  **properties;
  GParamSpec  **actor_properties;
  guint         n_properties;
  guint         n_actor_properties;
  gint          i;
  ClutterActor *parent;
  GObject      *child_meta;
  GList *transient_values = NULL;

  if (!actor)
    return NULL;
  parent = clutter_actor_get_parent (actor);
  if (!parent)
    return NULL;
  if (!CLUTTER_IS_CONTAINER (parent))
    return NULL;

  child_meta = G_OBJECT (clutter_container_get_child_meta (CLUTTER_CONTAINER (parent),
                                                 actor));
  if (!child_meta)
    return NULL;

  properties = g_object_class_list_properties (
                     G_OBJECT_GET_CLASS (child_meta),
                     &n_properties);
  actor_properties = g_object_class_list_properties (
            G_OBJECT_CLASS (g_type_class_ref (CLUTTER_TYPE_CHILD_META)),
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
                  skip = TRUE;
                }
            }
        }

      if (!(properties[i]->flags & G_PARAM_READABLE))
        skip = TRUE;

      if (skip)
        continue;

        {
          TransientValue *value = g_new0 (TransientValue, 1);
          value->name = properties[i]->name;
          value->value_type = properties[i]->value_type;
          g_value_init (&value->value, properties[i]->value_type);
          g_object_get_property (G_OBJECT (child_meta), properties[i]->name, &value->value);

          transient_values = g_list_prepend (transient_values, value);
        }
    }
  g_free (properties);
  return transient_values;
}

static void
cs_apply_child_transient (ClutterActor *actor,
                            GList        *transient_values)
{
  GParamSpec **properties;
  guint        n_properties;
  gint         i;
  ClutterActor *parent;
  GObject      *child_meta;

  if (!actor)
    return;
  parent = clutter_actor_get_parent (actor);
  if (!parent)
    return;
  if (!CLUTTER_IS_CONTAINER (parent))
    return;

  child_meta = G_OBJECT (clutter_container_get_child_meta (CLUTTER_CONTAINER (parent),
                                                 actor));
  if (!child_meta)
    return;

  properties = g_object_class_list_properties (
                     G_OBJECT_GET_CLASS (child_meta),
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
              g_object_set_property (G_OBJECT (child_meta), properties[i]->name, &value->value);
            }
        }
    }

  g_free (properties);
}



/* XXX: needs rewrite to not need parent argument */
ClutterActor *cs_duplicator (ClutterActor *actor, ClutterActor *parent)
{
  ClutterActor *new_actor;
  GList *transient_values;

  new_actor = g_object_new (G_OBJECT_TYPE (actor), NULL);
  transient_values = cs_build_transient (actor);
  cs_apply_transient (new_actor, transient_values);
  transient_free (transient_values);

  /* recurse through children? */
  if (CLUTTER_IS_CONTAINER (new_actor))
    {
      GList *children, *c;
      children = clutter_container_get_children (CLUTTER_CONTAINER (actor));
      for (c = children; c; c = c->next)
        {
          cs_duplicator (c->data, new_actor);
        }
      g_list_free (children);
    }

  clutter_container_add_actor (CLUTTER_CONTAINER (parent), new_actor);
  return new_actor;
}


static void get_all_actors_int (GList **list, ClutterActor *actor, gboolean skip_own);

GList *
cs_container_get_children_recursive (ClutterActor *actor)
{
  GList *ret = NULL;
  get_all_actors_int (&ret, actor, TRUE);
  return ret;
}


static ClutterActor *
cs_find_by_id2 (ClutterActor *stage, const gchar *id, gboolean skip_int)
{
  GList *l, *list = NULL;
  get_all_actors_int (&list, stage, skip_int);
  ClutterActor *ret = NULL;
  for (l=list;l && !ret;l=l->next)
    {
      if (CLUTTER_SCRIPTABLE (l->data))
        {
          const gchar *gotid = clutter_scriptable_get_id (CLUTTER_SCRIPTABLE (l->data));
          if (gotid && g_str_equal (gotid, id))
            {
              ret = l->data;
              break;
            }
        }
    }
  g_list_free (list);
  return ret;
}

ClutterActor *
cs_find_by_id (ClutterActor *stage, const gchar *id)
{
  return cs_find_by_id2 (stage, id, TRUE);
}


ClutterActor *
cs_find_by_id_int (ClutterActor *stage, const gchar *id)
{
  return cs_find_by_id2 (stage, id, FALSE);
}


/* like foreach, but returns the first non NULL return value (and
 * stops iterating at that stage)
 */
gpointer cs_list_match (GList *list, GCallback match_fun, gpointer data)
{
  gpointer ret = NULL;
  gpointer (*match)(gpointer item, gpointer data)=(void*)match_fun;
  GList *l;
  for (l=list; l && ret == NULL; l=l->next)
    {
      ret = match (l->data, data);
    }
  return ret;
}


gboolean cs_block_event (ClutterActor *actor)
{
  return TRUE;
}

static gint sort_spatial (gconstpointer a,
                          gconstpointer b)
{
  gfloat ax,ay,bx,by;
  clutter_actor_get_position (CLUTTER_ACTOR(a), &ax, &ay);
  clutter_actor_get_position (CLUTTER_ACTOR(b), &bx, &by);

  if (ay == by)
    return by-ay;
  return bx-ax;
}
                          

void cs_container_visual_sort  (ClutterContainer *container)
{
  GList *c, *children = NULL;
  children = clutter_container_get_children (container);

  children = g_list_sort (children, sort_spatial);

  for (c = children; c; c = c->next)
    {
      GList *transient_child_props = cs_build_child_transient (c->data);
      if (transient_child_props)
        g_object_set_data (c->data, "cs-transient", transient_child_props);
      g_object_ref (c->data);
      clutter_container_remove_actor (container, c->data);
    }

  for (c = children; c; c = c->next)
    {
      GList *transient_child_props = g_object_get_data (c->data, "cs-transient");
      clutter_container_add_actor (container, c->data);
      if (transient_child_props)
        {
          cs_apply_child_transient (c->data, transient_child_props);
          g_object_set_data (c->data, "cs-transient", NULL);
        }
      g_object_unref (c->data);
    }
  g_list_free (children);
}



void cs_container_add_actor_at (ClutterContainer *container,
                                ClutterActor     *actor,
                                gint              pos)
{
  GList *c, *children, *remainder = NULL;
  gint i = 0;
  children = clutter_container_get_children (container);

  /* remove all children after to insertion point */
  for (c = children; c; c = c->next)
    {
      if (i>=pos)
        {
          remainder = c;
          break;
        }
      i++;
    }

  for (c = remainder; c; c = c->next)
    {
      GList *transient_child_props = cs_build_child_transient (c->data);
      if (transient_child_props)
        g_object_set_data (c->data, "cs-transient", transient_child_props);
      g_object_ref (c->data);
      clutter_container_remove_actor (container, c->data);
    }
  clutter_container_add_actor (container, actor);
  for (c = remainder; c; c = c->next)
    {
      GList *transient_child_props = g_object_get_data (c->data, "cs-transient");
      clutter_container_add_actor (container, c->data);
      if (transient_child_props)
        {
          cs_apply_child_transient (c->data, transient_child_props);
          g_object_set_data (c->data, "cs-transient", NULL);
        }
      g_object_unref (c->data);
    }
  g_list_free (children);
}

gint cs_get_sibling_no (ClutterActor *actor)
{
  ClutterActor *parent;
  GList *c, *children;
  gint i = 0;

  if (!actor)
    return 0;
  parent = clutter_actor_get_parent (actor);
  if (!parent)
    return 0;
  if (!CLUTTER_IS_CONTAINER (parent))
    return 0;
  children = clutter_container_get_children (CLUTTER_CONTAINER (parent));
  for (c = children; c; c = c->next)
    {
      if (c->data == actor)
        {
          g_list_free (children);
          return i;
        }
      i++;
    }
  g_list_free (children);
  return 0;
}

void cs_container_replace_child (ClutterContainer *container,
                                   ClutterActor     *old,
                                   ClutterActor     *new)
{
  gint sibling_no;
  sibling_no = cs_get_sibling_no (old);
  clutter_actor_destroy (old);
  cs_container_add_actor_at (container, new, sibling_no);
}


ClutterActor *cs_actor_change_type (ClutterActor *actor,
                                    const gchar  *new_type)
{
  ClutterActor *new_actor, *parent;
  GList *transient_values;
  GList *transient_child_values;

  if (CLUTTER_IS_STAGE (actor))
    {
      g_warning ("refusing to change type of stage");
      return actor;
    }

  if (CLUTTER_IS_GROUP (actor))
    cs_container_visual_sort(CLUTTER_CONTAINER (actor));

  new_actor = g_object_new (g_type_from_name (new_type), NULL);
  parent = clutter_actor_get_parent (actor);

  transient_values = cs_build_transient (actor);
  transient_child_values = cs_build_child_transient (actor);

    if (CLUTTER_IS_CONTAINER (actor) && CLUTTER_IS_CONTAINER (new_actor))
      {
        GList *c, *children;
        children = clutter_container_get_children (CLUTTER_CONTAINER (actor));
        for (c = children; c; c = c->next)
          {
            ClutterActor *child = g_object_ref (c->data);
            clutter_container_remove_actor (CLUTTER_CONTAINER (actor), child);
            clutter_container_add_actor (CLUTTER_CONTAINER (new_actor), child);

          }
        g_list_free (children);
      }

  cs_apply_transient (new_actor, transient_values);
  cs_container_replace_child (CLUTTER_CONTAINER (parent), actor, new_actor);
  cs_apply_child_transient (new_actor, transient_child_values);
  transient_free (transient_values);
  transient_free (transient_child_values);

  return new_actor;
}



/* Functions below this line need cluttersmith infrastructure */
#include "cluttersmith.h"

static void get_all_actors_int (GList         **list,
                                ClutterActor   *actor,
                                gboolean        skip_own)
{
  const gchar *id;
  if (skip_own && cs_actor_has_ancestor (actor, cluttersmith->parasite_root))
    return;
  id = clutter_scriptable_get_id (CLUTTER_SCRIPTABLE (actor));

  if (!CLUTTER_IS_STAGE (actor))
    *list = g_list_prepend (*list, actor);

  if (CLUTTER_IS_CONTAINER (actor) &&
      (!(id && g_str_equal (id, "previews-container"))))
    {
      GList *children, *c;
      children = clutter_container_get_children (CLUTTER_CONTAINER (actor));
      for (c = children; c; c=c->next)
        {
          get_all_actors_int (list, c->data, skip_own);
        }
      g_list_free (children);
    }
}
