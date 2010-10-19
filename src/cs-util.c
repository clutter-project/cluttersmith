/*
 * ClutterSmith - a visual authoring environment for clutter.
 * Copyright (c) 2009, Intel Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU Lesser General Public License,
 * version 2.1, as published by the Free Software Foundation.
 *
 * Alternatively, you can redistribute it and/or modify it
 * under the terms and conditions of the GNU Lesser General Public License,
 * version 3, with the following additional permissions:
 *
 * 1. Intel grants you an additional permission under Section 7 of the
 * GNU General Public License, version 3, exempting you from the
 * requirement in Section 6 of the GNU General Public License, version 3,
 * to accompany Corresponding Source with Installation Information for
 * the Program or any work based on the Program.  You are still required
 * to comply with all other Section 6 requirements to provide
 * Corresponding Source.
 *
 * 2. Intel grants you an additional permission under Section 7 of the
 * GNU General Public License, version 3, allowing you to convey the
 * Program or a work based on the Program in combination with or linked
 * to any works licensed under the GNU General Public License version 2,
 * with the terms and conditions of the GNU General Public License
 * version 2 applying to the combined or linked work as a whole.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Written by: Øyvind Kolås <oyvind.kolas@intel.com>
 */


#include <math.h>
#include <clutter/clutter.h>
#include "cs-util.h"

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


ClutterActor *cs_parse_json (const gchar *json)
{
  ClutterActor *actor;
  ClutterScript *script = clutter_script_new ();
  GError *error = NULL;

  clutter_script_load_from_data (script, json, -1, &error);

  actor = CLUTTER_ACTOR (clutter_script_get_object (script, "actor"));
  if (!actor)
     g_warning ("failed parsing from json %s\n", error?error->message:"");
  clutter_script_connect_signals (script, script);
  g_object_set_data_full (G_OBJECT (actor), "clutter-script", script, g_object_unref);

  return actor;
}

ClutterActor *cs_load_json (const gchar *name)
{
  ClutterActor *actor;
  ClutterScript *script = clutter_script_new ();
  GError *error = NULL;

  if (g_file_test (name, G_FILE_TEST_IS_REGULAR))
    {
      clutter_script_load_from_file (script, name, &error);
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
          clutter_group_add (CLUTTER_GROUP (cluttersmith_get_stage ()), actor);
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
ClutterActor *cs_replace_content (ClutterActor *actor,
                                  const gchar  *name,
                                  const gchar  *filename,
                                  const gchar  *inline_script)
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
      if (filename)
        clutter_container_add_actor (CLUTTER_CONTAINER (content), ret = cs_load_json (filename));
      else if (inline_script)
        clutter_container_add_actor (CLUTTER_CONTAINER (content), ret = cs_parse_json (inline_script));
      else
        clutter_container_add_actor (CLUTTER_CONTAINER (content), ret = empty_scene_new ());
      return ret;
    }

  cs_container_remove_children (content);
  if (filename || inline_script)
    {
      if (filename)
      clutter_container_add_actor (CLUTTER_CONTAINER (content), ret = cs_load_json (filename));
      else
      clutter_container_add_actor (CLUTTER_CONTAINER (content), ret = cs_parse_json (inline_script));

      /* force all actors that are loaded in this manner to have unique
       * id's, this is needed to make undo/redo work
       */
      if (ret)
        {
          GList *a, *added;
          added = cs_container_get_children_recursive (CLUTTER_CONTAINER (ret));
          for (a = added; a; a = a->next)
            {
              cs_actor_make_id_unique (a->data, NULL);
            }
          g_list_free (added);
        }
    }
  else
    clutter_container_add_actor (CLUTTER_CONTAINER (content), ret = empty_scene_new ());
  return ret;
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
  GObject     *object;
  const gchar *property_name;
  GValue       value;
  GType        value_type;
} TransientValue;


/* XXX: this list is incomplete */
static gchar *whitelist[]={"x", "y", "width","height", "depth", "opacity",
                           "scale-x","scale-y", "anchor-x", "color",
                           "anchor-y", "rotation-angle-z", "rotation-angle-x",
                           "rotation-angle-y",
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
                  gint k;
                  skip = TRUE;
                  for (k=0;whitelist[k];k++)
                    if (g_str_equal (properties[i]->name, whitelist[k]))
                      skip = FALSE;
                }
            }
        }
      if (g_str_equal (properties[i]->name, "child")||
          g_str_equal (properties[i]->name, "cogl-texture")||
          g_str_equal (properties[i]->name, "disable-slicing")||
          g_str_equal (properties[i]->name, "cogl-handle"))
        skip = TRUE; /* to avoid duplicated parenting of MxButton children. */

      if (!(properties[i]->flags & G_PARAM_READABLE))
        skip = TRUE;

      if (skip)
        continue;

        {
          TransientValue *value = g_new0 (TransientValue, 1);
          value->property_name = g_intern_string (properties[i]->name);
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
          if (g_str_equal (value->property_name, properties[i]->name) &&
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
          value->property_name = g_intern_string (properties[i]->name);
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
          if (g_str_equal (value->property_name, properties[i]->name) &&
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
  cs_actor_make_id_unique (new_actor, NULL);
  return new_actor;
}


static void get_all_actors_int (GList **list, ClutterActor *actor, gboolean skip_own);

GList *
cs_container_get_children_recursive (ClutterContainer *container)
{
  GList *ret = NULL;
  get_all_actors_int (&ret, CLUTTER_ACTOR (container), TRUE);
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
  if (!ret)
    {
      g_warning ("not finding %s\n", id);
    }
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

ClutterActor *
cs_container_get_child_no (ClutterContainer *container,
                           gint              pos)
{
  ClutterActor *ret = NULL;
  GList *c, *children;
  gint i = 0;

  if (!container)
    return NULL;
  if (!CLUTTER_IS_CONTAINER (container))
    return NULL;
  children = clutter_container_get_children (CLUTTER_CONTAINER (container));
  for (c = children; c; c = c->next)
    {
      if (i==pos)
        {
          ret = c->data;
          break;
        }
      i++;
    }
  g_list_free (children);
  return ret;
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



void cluttersmith_print (const gchar *string)
{
  g_print ("%s\n", string);
}

gint cs_actor_ancestor_depth (ClutterActor *actor)
{
  gint i=0;
  while (actor && (actor = clutter_actor_get_parent (actor)))
   i++;
  return i;
}

/* Functions below this line need cluttersmith infrastructure */
#include "cluttersmith.h"


/**
 * cluttersmith_get_object:
 * @id: the id to lookup
 *
 * Return value: (transfer none): the actor if any or NULL
 */
GObject *cluttersmith_get_object (const gchar  *id)
{
  if (cs->state_machines)
    {
      GList *a;
      for (a = cs->state_machines; a; a=a->next)
          if (!g_strcmp0 (clutter_scriptable_get_id (a->data), id))
            return a->data;
    }

  return (void*)cs_find_by_id (cluttersmith_get_stage (), id);
}


static void get_all_actors_int (GList         **list,
                                ClutterActor   *actor,
                                gboolean        skip_own)
{
  const gchar *id;
  if (skip_own && cs_actor_has_ancestor (actor, cs->parasite_root))
    return;

  if (!actor)
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


/**
 * cluttersmith_get_stage:
 *
 * Return value: (transfer none): the actor if any or NULL
 */
ClutterActor  *cluttersmith_get_stage (void)
{
  ClutterActor *ret;
  ret = cs->fake_stage;
  return ret;
}


ClutterActor *
cs_find_nearest (ClutterActor *actor,
                 gboolean      vertical,
                 gboolean      reverse)
{
  GList *s, *siblings;
  ClutterActor *best = NULL;
  gfloat bestdist = 200000;
  gfloat selfx, selfy;

  clutter_actor_get_transformed_position (actor, &selfx, &selfy);
  siblings = cs_actor_get_siblings (actor);

  for (s=siblings; s; s=s->next)
    {
      ClutterActor *sib = s->data;
      if (sib != actor)
        {
          gfloat x, y;
          gfloat dist;
          
          clutter_actor_get_transformed_position (sib, &x, &y);

          if (vertical)
            {

#define ASSYMETRY 2
              /* we do not check for exactly equal to avoid osciallating */
              if ( (!reverse && y > selfy) ||
                    (reverse && y < selfy))
                {
                  dist = sqrt ( (selfy-y)*(selfy-y) + ASSYMETRY * (selfx-x)*(selfx-x));
                  if (dist < bestdist)
                    {
                      bestdist = dist;
                      best = sib;
                    }
                }
            }
          else
            {
              if ( (!reverse && x > selfx) ||
                    (reverse && x < selfx))
                {
                  dist = sqrt (ASSYMETRY *(selfy-y)*(selfy-y) + (selfx-x)*(selfx-x));
                  if (dist < bestdist)
                    {
                      bestdist = dist;
                      best = sib;
                    }
                }
            }
        }
    }

  g_list_free (siblings);
  return best;
}


GList *cs_actor_get_siblings (ClutterActor *actor)
{
  ClutterActor *parent;
  if (!actor)
    return NULL;
  parent = clutter_actor_get_parent (actor);
  if (!parent)
    return NULL;
  return clutter_container_get_children (CLUTTER_CONTAINER (parent));
}



static GHashTable *default_values_ht = NULL;

static TransientValue *
tval_new (GObject     *object,
          const gchar *property_name)
{
  TransientValue *tval;
  tval = g_new0 (TransientValue, 1);
  tval->object = object;
  tval->property_name = g_intern_string (property_name);
  return tval;
}

static void
tval_free (TransientValue *tval)
{
  g_free (tval);
}

static guint tval_hash (TransientValue *tval)
{
  return GPOINTER_TO_INT (tval->object) ^ GPOINTER_TO_INT (tval->property_name);
}

static gboolean
tval_equal (TransientValue *a,
            TransientValue *b)
{
  if (a->object == b->object &&
      a->property_name == b->property_name)
    return TRUE;
  return FALSE;
}

void cs_properties_init (void)
{
  if (default_values_ht)
      return;
  default_values_ht = g_hash_table_new_full ((GHashFunc)tval_hash,
                                             (GEqualFunc)tval_equal,
                                             (GDestroyNotify)tval_free,
                                             NULL);
}

void cs_properties_set_value (ClutterActor *actor,
                              const gchar  *property_name,
                              const GValue *value)
{
  TransientValue *tval;
  cs_properties_init ();

  tval = tval_new (G_OBJECT (actor), property_name);
  g_hash_table_insert (default_values_ht, tval, tval);
}

gboolean cs_properties_get_value (GObject      *object,
                                  const gchar  *property_name,
                                  GValue       *value)
{
  TransientValue key;
  TransientValue *tval;

  key.object = object;
  key.property_name = g_intern_string (property_name);

  tval = g_hash_table_lookup (default_values_ht, &key);

  if (tval)
    {
      g_value_copy (&tval->value, value);
      return TRUE;
    }
  return FALSE;
}

static void cs_actor_store_defaults (ClutterActor *actor)
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
                  gint k;
                  skip = TRUE;
                  for (k=0;whitelist[k];k++)
                    if (g_str_equal (properties[i]->name, whitelist[k]))
                      skip = FALSE;
                }
            }
        }
      if (g_str_equal (properties[i]->name, "child")||
          /* avoid duplicated parenting of MxButton children. */
          g_str_equal (properties[i]->name, "cogl-texture")||
          g_str_equal (properties[i]->name, "disable-slicing")||
          g_str_equal (properties[i]->name, "cogl-handle"))
        skip = TRUE; 

      if (!(properties[i]->flags & G_PARAM_READABLE))
        skip = TRUE;
      if (!(properties[i]->flags & G_PARAM_WRITABLE))
        skip = TRUE;

      if (skip)
        continue;
        {
          TransientValue *value = tval_new (G_OBJECT (actor),
                                            properties[i]->name);

          g_value_init (&value->value, properties[i]->value_type);
          g_object_get_property (G_OBJECT (actor), properties[i]->name, &value->value);
          g_hash_table_insert (default_values_ht, value, value);
        }
    }
  g_free (properties);

  /* XXX: also store eventual child and layout_meta's properties */

  if (CLUTTER_IS_CONTAINER (actor))
    {
      GList *children = clutter_container_get_children (
                               CLUTTER_CONTAINER (actor));
      GList *c;

      for (c = children; c; c = c->next)
        {
          cs_actor_store_defaults (c->data);
        }

      g_list_free (children);
    }
}

void cs_properties_store_defaults (void)
{
  g_print ("Storing defaults\n");
  cs_properties_init ();
  g_hash_table_remove_all (default_values_ht);
  cs_actor_store_defaults (cs->fake_stage);
}

void cs_properties_restore_defaults (void)
{
  GHashTableIter iter;
  gpointer key, value;
  cs_properties_init ();
  g_print ("restoring defaults\n");

  g_hash_table_iter_init (&iter, default_values_ht);
  while (g_hash_table_iter_next (&iter, &key, &value))
    {
      TransientValue *tval = key;
      g_object_set_property (tval->object, tval->property_name,
                             &tval->value);
    }
}


ClutterAnimator *
cs_state_make_animator (ClutterState *state,
                        const gchar  *source_state,
                        const gchar  *target_state)
{
  ClutterAnimator *animator;
  GList *k, *keys;
  animator = clutter_animator_new ();
  keys = clutter_state_get_keys (state, source_state, target_state, NULL, NULL);

  for (k = keys; k; k = k->next)
    {
      GValue value = {0, };
      GParamSpec      *pspec;
      ClutterStateKey *key;
      gdouble          pre_delay;
      gdouble          post_delay;
      const gchar     *property_name;
      GObject         *object;
      key = k->data;

      object = clutter_state_key_get_object (key);
      property_name = clutter_state_key_get_property_name (key);
      pre_delay = clutter_state_key_get_pre_delay (key);
      post_delay = clutter_state_key_get_post_delay (key);
      pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (object),
                                            property_name);
      g_value_init (&value, pspec->value_type);
      clutter_state_key_get_value (key, &value);

      clutter_animator_set_key (animator, object, property_name,
                                clutter_state_key_get_mode (key),
                                1.0 - post_delay, &value);

      if (source_state == NULL && clutter_state_key_get_source_state_name (key))
        continue;

      if (source_state == NULL)
        {
          clutter_animator_set_key (animator, object, property_name,
                                    clutter_state_key_get_mode (key),
                                    0.0 + pre_delay, &value);
          clutter_animator_property_set_ease_in (animator, object, property_name, TRUE);
          clutter_animator_property_set_interpolation (animator, object, property_name,
                                                       CLUTTER_INTERPOLATION_CUBIC);
        }
      else
        {
           GList *keys2;
           
           keys2 = clutter_state_get_keys (state, NULL, source_state, object, property_name);
           /* XXX: should prefer NULL over specifics.. */
           if (keys2)
             {
                clutter_animator_set_key (animator, object, property_name,
                                          clutter_state_key_get_mode (keys2->data),
                                          0.0 + pre_delay, &value);
             }

           g_list_free (keys2);
        }

      g_value_unset (&value);

    }
  g_list_free (keys);

  return animator;
}

void
cs_actor_make_id_unique (ClutterActor *actor,
                         const gchar  *stem)
{
  ClutterScriptable *scriptable = CLUTTER_SCRIPTABLE (actor);
  GList *a, *actors;
  GString *str;
  gint     count = 0;

  if (!stem)
   stem = G_OBJECT_TYPE_NAME (actor);

  if (clutter_scriptable_get_id (scriptable))
    str = g_string_new (clutter_scriptable_get_id (scriptable));
  else
    str = g_string_new (stem);

  actors = cs_container_get_children_recursive (CLUTTER_CONTAINER (cs->fake_stage));

again:
  for (a = actors; a; a = a->next)
    {
      if (a->data != actor &&
          CLUTTER_IS_SCRIPTABLE (a->data))
        {
          const gchar *id = clutter_scriptable_get_id (a->data);
          if (id && g_str_equal (id, str->str))
            {
              count ++;
              g_string_printf (str, "%s%i", stem, count);
              goto again;
            }
        }
    }
#ifndef COMPILEMODULE
  clutter_scriptable_set_id (scriptable, str->str);
#endif

  g_list_free (actors);
  g_string_free (str, TRUE);
}

const gchar *
cs_get_id (ClutterActor *actor)
{
  const gchar *id;
  id = clutter_scriptable_get_id (CLUTTER_SCRIPTABLE (actor));
  if (!id || g_str_has_prefix (id, "script-"))
    {
      cs_actor_make_id_unique (actor, G_OBJECT_TYPE_NAME (actor));
      id = clutter_scriptable_get_id (CLUTTER_SCRIPTABLE (actor));
    }
  return id;
}


void cs_draw_actor_outline (ClutterActor *actor,
                            gpointer      data)
{
  ClutterVertex verts[4];
  clutter_actor_get_abs_allocation_vertices (actor,
                                             verts);

  {
    {
      gfloat coords[]={ verts[0].x, verts[0].y, 
         verts[1].x, verts[1].y, 
         verts[3].x, verts[3].y, 
         verts[2].x, verts[2].y, 
         verts[0].x, verts[0].y };

      cogl_path_polyline (coords, 5);
      cogl_path_stroke ();
    }
  }
}


char *
cs_make_config_file (const char *filename)
{
  const char *base;
  char *path, *full;

  base = g_getenv ("XDG_CONFIG_HOME");
  if (base) {
    path = g_strdup_printf ("%s/cluttersmith", base);
    full = g_strdup_printf ("%s/cluttersmith/%s", base, filename);
  } else {
    path = g_strdup_printf ("%s/.config/cluttersmith", g_get_home_dir ());
    full = g_strdup_printf ("%s/.config/cluttersmith/%s", g_get_home_dir (),
                            filename);
  }

  /* Make sure the directory exists */
  if (g_file_test (path, G_FILE_TEST_EXISTS) == FALSE) {
    g_mkdir_with_parents (path, 0700);
  }
  g_free (path);

  return full;
}

gboolean
cs_state_has_state (ClutterState *state,
                    const gchar  *state_name)
{
  GList *states;
  gboolean found = FALSE;
  states = clutter_state_get_states (state);
  if (g_list_find (states, g_intern_string (state_name)))
    found = TRUE;
  g_list_free (states);
  return found;
}


static gboolean modal_key_capture (ClutterText *text,
                                   ClutterEvent *event,
                                   gpointer      group)
{
  if (clutter_event_type (event) == CLUTTER_KEY_PRESS && 
      clutter_event_get_key_symbol (event) == CLUTTER_Escape)
    {
      clutter_actor_destroy (group);
      return TRUE;
    }
  return FALSE;
}


void
cs_modal_editor (gfloat x,
                 gfloat y,
                 gfloat width,
                 gfloat height,
                 const gchar *start_text,
                 void (*activate)(ClutterText *text))
{
  ClutterActor *stage = clutter_actor_get_stage (cs->parasite_root);
  ClutterActor *group;
  ClutterActor *rectangle;
  ClutterActor *text;
  ClutterColor black = {0x0,0x0,0x0,0xff};
  ClutterColor white = {0xff,0xff,0xff,0xff};

  group = clutter_group_new ();
  rectangle = clutter_rectangle_new_with_color (&black);
  clutter_actor_set_size (rectangle, clutter_actor_get_width (stage),
                                     clutter_actor_get_height (stage));
  text = g_object_new (CLUTTER_TYPE_TEXT,
                       "x", x,
                       "y", y,
                       "width", width,
                       "height", height,
                       "text", start_text,
                       "editable", TRUE,
                       "reactive", TRUE,
                       "activatable", TRUE,
                       "color", &white,
                       NULL);
  clutter_actor_set_opacity (rectangle, 0xbb);
  clutter_actor_set_reactive (rectangle, TRUE);

  clutter_container_add (CLUTTER_CONTAINER (group), rectangle, text, NULL);
  clutter_container_add_actor (CLUTTER_CONTAINER (cs->parasite_root), group);
  clutter_stage_set_key_focus (CLUTTER_STAGE (stage), text);

  g_signal_connect (text, "captured-event",
                    G_CALLBACK (modal_key_capture), group);
  g_signal_connect_swapped (rectangle, "button-release-event",
                    G_CALLBACK (clutter_actor_destroy), group);
  g_signal_connect (text, "activate", G_CALLBACK (activate), NULL);
  g_signal_connect_swapped (text, "activate",
                    G_CALLBACK (clutter_actor_destroy), group);
}


void
cs_actor_move_anchor_point (ClutterActor *self,
                            gfloat        anchor_x,
                            gfloat        anchor_y)
{
  gfloat old_anchor_x, old_anchor_y;
  gfloat x, y;

  g_return_if_fail (CLUTTER_IS_ACTOR (self));

  clutter_actor_get_anchor_point (self,
                                  &old_anchor_x,
                                  &old_anchor_y);

  {
    ClutterVertex new_anchor = {anchor_x, anchor_y, 0.0};
    ClutterVertex old_anchor = {old_anchor_x, old_anchor_y, 0.0};
    ClutterVertex new_anchor_parent, old_anchor_parent;

    clutter_actor_apply_relative_transform_to_point (self,
                             clutter_actor_get_parent (self),
                             &new_anchor, &new_anchor_parent);
    clutter_actor_apply_relative_transform_to_point (self,
                             clutter_actor_get_parent (self),
                             &old_anchor, &old_anchor_parent);

    clutter_actor_get_position (self, &x, &y);

    x += new_anchor_parent.x - old_anchor_parent.x;
    y += new_anchor_parent.y - old_anchor_parent.y;

    g_object_freeze_notify (G_OBJECT (self));

    clutter_actor_set_anchor_point (self, anchor_x, anchor_y);
    clutter_actor_set_position (self, x, y);

    g_object_thaw_notify (G_OBJECT (self));
  }
}
