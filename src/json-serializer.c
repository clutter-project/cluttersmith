#include <mx/mx.h>
#include <clutter/clutter.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include "util.h"
#include "cluttersmith.h"

/**** XXX: This serializer is a hack that will be replaced by
 * json-glib and clutterscript code
 */

static gboolean cs_filter_properties = TRUE;

#define INDENT {gint j;for (j=0;j<*indentation;j++) g_string_append_c (str, ' ');}

static gchar *escape_string (const gchar *in)
{
  GString *str = g_string_new ("");
  const gchar *p;
  gchar *ret;

  for (p=in;*p;p++)
    {
      if (*p=='"')
        g_string_append (str, "\\\"");
      else
        g_string_append_c (str, *p);

    }

  ret = str->str;
  g_string_free (str, FALSE);
  return ret;
}

static void
properties_to_string (GString      *str,
                      ClutterActor *actor,
                      gint         *indentation)
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

  {
    const gchar *id = clutter_scriptable_get_id (CLUTTER_SCRIPTABLE (actor));
    if (id && id[0]!='\0' && !g_str_has_prefix(id, "script-"))
      {
        INDENT;g_string_append_printf (str,"\"id\":\"%s\"\n", id);
      }
  }

  for (i = 0; i < n_properties; i++)
    {
      gint j;
      gboolean skip = FALSE;

      if (cs_filter_properties)
        {
          for (j=0;j<n_actor_properties;j++)
            {
              /* ClutterActor contains so many properties that we restrict our view a bit */
              if (actor_properties[j]==properties[i])
                {
                  gchar *whitelist[]={"x","y", "depth", "opacity", "width", "height",
                                      "scale-x","scale-y", "anchor-x", "color",
                                      "anchor-y", "rotation-angle-z",
                                      "name", 
                                      NULL};
                  gint k;
                  skip = TRUE;
                  for (k=0;whitelist[k];k++)
                    if (g_str_equal (properties[i]->name, whitelist[k]))
                      skip = FALSE;
                }
            }
        }

      if (! ( (properties[i]->flags & G_PARAM_READABLE) &&
              (properties[i]->flags & G_PARAM_WRITABLE)
          ))
        skip = TRUE;

      if (skip)
        continue;

      {
        if (properties[i]->value_type == G_TYPE_FLOAT)
          {
            gfloat value;
            /* XXX: clutter fails to read it back in without truncation */
            g_object_get (actor, properties[i]->name, &value, NULL);
            if (g_str_equal (properties[i]->name, "x")||
                g_str_equal (properties[i]->name, "y")||
                g_str_equal (properties[i]->name, "width")||
                g_str_equal (properties[i]->name, "height"))
              {
                INDENT;g_string_append_printf (str,"\"%s\":%2.0f,\n",
                                        properties[i]->name, value);
              }
                else
              {
                INDENT;g_string_append_printf (str,"\"%s\":%2.3f,\n",
                                        properties[i]->name, value);
              }
          }
        else if (properties[i]->value_type == G_TYPE_DOUBLE)
          {
            gdouble value;
            g_object_get (actor, properties[i]->name, &value, NULL);
            INDENT;g_string_append_printf (str,"\"%s\":%2.3f,\n",
                                           properties[i]->name, value);
          }
        else if (properties[i]->value_type == G_TYPE_UCHAR)
          {
            guchar value;
            g_object_get (actor, properties[i]->name, &value, NULL);
            INDENT;g_string_append_printf (str,"\"%s\":%i,\n",
                                           properties[i]->name, value);
          }
        else if (properties[i]->value_type == G_TYPE_INT)
          {
            gint value;
            g_object_get (actor, properties[i]->name, &value, NULL);
            INDENT;g_string_append_printf (str,"\"%s\":%i,\n",
                                           properties[i]->name, value);
          }
        else if (properties[i]->value_type == G_TYPE_UINT)
          {
            guint value;
            g_object_get (actor, properties[i]->name, &value, NULL);
            INDENT;g_string_append_printf (str,"\"%s\":%u,\n",
                                           properties[i]->name, value);
          }
        else if (properties[i]->value_type == G_TYPE_STRING)
          {
            gchar *value;
            g_object_get (actor, properties[i]->name, &value, NULL);
            if (value)
              {
                gchar *escaped = escape_string (value);
                INDENT;g_string_append_printf (str,"\"%s\":\"%s\",\n",
                                               properties[i]->name, escaped);
                g_free (escaped);
                g_free (value);
              }
          }
        else if (properties[i]->value_type == G_TYPE_BOOLEAN)
          {
            gboolean value;
            g_object_get (actor, properties[i]->name, &value, NULL);
            INDENT;g_string_append_printf (str,"\"%s\":%s,\n",
                                           properties[i]->name, value?"true":"false");
          }
        else if (properties[i]->value_type == CLUTTER_TYPE_COLOR)
          {
            GValue value = {0,};
            GValue str_value = {0,};
            g_value_init (&value, properties[i]->value_type);
            g_value_init (&str_value, G_TYPE_STRING);
            g_object_get_property (G_OBJECT (actor), properties[i]->name, &value);
            if (g_value_transform (&value, &str_value))
              {
                INDENT;g_string_append_printf (str,"\"%s\":\"%s\",\n",
                                               properties[i]->name, g_value_get_string (&str_value));
              }
            else
              {
              }
          }
        else
          {
#if 0
            GValue value = {0,};
            GValue str_value = {0,};
            gchar *initial;
            g_value_init (&value, properties[i]->value_type);
            g_value_init (&str_value, G_TYPE_STRING);
            g_object_get_property (G_OBJECT (actor), properties[i]->name, &value);
            if (g_value_transform (&value, &str_value))
              {
                INDENT;g_string_append_printf (str,"\"%s\":%s,\n",
                                               properties[i]->name, g_value_get_string (&str_value));
              }
            else
              {
              }
#endif
          }
      }
    }
  
  {
    const gchar *name = clutter_actor_get_name (actor);
    if (name && g_str_has_prefix (name, "json-extra:"))
      {
        INDENT; g_string_append_printf (str, name+11);
        g_string_append_printf (str, "\n");
      }
  }
  g_free (properties);


    /* should be split inot its own function */
  if (CLUTTER_IS_ACTOR (actor))
  {
    ClutterActor *parent;
    parent = clutter_actor_get_parent (actor);

    if (parent && CLUTTER_IS_CONTAINER (parent))
      {
        ClutterChildMeta *child_meta;
        GParamSpec **child_properties = NULL;
        guint        n_child_properties=0;
        child_meta = clutter_container_get_child_meta (CLUTTER_CONTAINER (parent), actor);
        if (child_meta)
          {
            child_properties = g_object_class_list_properties (
                               G_OBJECT_GET_CLASS (child_meta),
                               &n_child_properties);
            for (i = 0; i < n_child_properties; i++)
              {
                if (!G_TYPE_IS_OBJECT (child_properties[i]->value_type) &&
                    child_properties[i]->value_type != CLUTTER_TYPE_CONTAINER)
                  {
      gboolean skip = FALSE;
#if 0
      gint j;
      if (cs_filter_properties)
        {
          for (j=0;j<n_actor_properties;j++)
            {
              /* ClutterActor contains so many properties that we restrict our view a bit */
              if (actor_properties[j]==properties[i])
                {
                  gchar *whitelist[]={"x","y", "depth", "opacity", "width", "height",
                                      "scale-x","scale-y", "anchor-x", "color",
                                      "anchor-y", "rotation-angle-z",
                                      "name", 
                                      NULL};
                  gint k;
                  skip = TRUE;
                  for (k=0;whitelist[k];k++)
                    if (g_str_equal (properties[i]->name, whitelist[k]))
                      skip = FALSE;
                }
            }
        }
#endif

      if (! ( (child_properties[i]->flags & G_PARAM_READABLE) &&
              (child_properties[i]->flags & G_PARAM_WRITABLE)
          ))
        skip = TRUE;

      if (skip)
        continue;

      {
        if (child_properties[i]->value_type == G_TYPE_FLOAT)
          {
            gfloat value;
            /* XXX: clutter fails to read it back in without truncation */
            g_object_get (child_meta, child_properties[i]->name, &value, NULL);
            {
              INDENT;g_string_append_printf (str,"\"child::%s\":%2.3f,\n",
                                             child_properties[i]->name, value);
            }
          }
        else if (child_properties[i]->value_type == G_TYPE_DOUBLE)
          {
            gdouble value;
            g_object_get (child_meta, child_properties[i]->name, &value, NULL);
            INDENT;g_string_append_printf (str,"\"child::%s\":%2.3f,\n",
                                           child_properties[i]->name, value);
          }
        else if (child_properties[i]->value_type == G_TYPE_UCHAR)
          {
            guchar value;
            g_object_get (child_meta, child_properties[i]->name, &value, NULL);
            INDENT;g_string_append_printf (str,"\"child::%s\":%i,\n",
                                           child_properties[i]->name, value);
          }
        else if (child_properties[i]->value_type == G_TYPE_INT)
          {
            gint value;
            g_object_get (child_meta, child_properties[i]->name, &value, NULL);
            INDENT;g_string_append_printf (str,"\"child::%s\":%i,\n",
                                           child_properties[i]->name, value);
          }
        else if (child_properties[i]->value_type == G_TYPE_UINT)
          {
            guint value;
            g_object_get (child_meta, child_properties[i]->name, &value, NULL);
            INDENT;g_string_append_printf (str,"\"child::%s\":%u,\n",
                                           child_properties[i]->name, value);
          }
        else if (child_properties[i]->value_type == G_TYPE_STRING)
          {
            gchar *value;
            g_object_get (child_meta, child_properties[i]->name, &value, NULL);
            if (value)
              {
                gchar *escaped = escape_string (value);
                INDENT;g_string_append_printf (str,"\"child::%s\":\"%s\",\n",
                                               child_properties[i]->name, escaped);
                g_free (escaped);
                g_free (value);
              }
          }
        else if (child_properties[i]->value_type == G_TYPE_BOOLEAN)
          {
            gboolean value;
            g_object_get (child_meta, child_properties[i]->name, &value, NULL);
            INDENT;g_string_append_printf (str,"\"child::%s\":%s,\n",
                                           child_properties[i]->name, value?"true":"false");
          }
        else if (child_properties[i]->value_type == CLUTTER_TYPE_COLOR)
          {
            GValue value = {0,};
            GValue str_value = {0,};
            g_value_init (&value, child_properties[i]->value_type);
            g_value_init (&str_value, G_TYPE_STRING);
            g_object_get_property (G_OBJECT (child_meta), child_properties[i]->name, &value);
            if (g_value_transform (&value, &str_value))
              {
                INDENT;g_string_append_printf (str,"\"child::%s\":\"%s\",\n",
                                               child_properties[i]->name, g_value_get_string (&str_value));
              }
            else
              {
              }
          }
        else
          {
#if 0
            GValue value = {0,};
            GValue str_value = {0,};
            gchar *initial;
            g_value_init (&value, child_properties[i]->value_type);
            g_value_init (&str_value, G_TYPE_STRING);
            g_object_get_property (G_OBJECT (child_meta), child_properties[i]->name, &value);
            if (g_value_transform (&value, &str_value))
              {
                INDENT;g_string_append_printf (str,"\"child::%s\":%s,\n",
                                               child_properties[i]->name, g_value_get_string (&str_value));
              }
            else
              {
              }
#endif
          }
      }

                  }
              }
            g_free (child_properties);
          }
      }
  }


}


static void
actor_to_string (GString      *str,
                 ClutterActor *iter,
                 gint         *indentation)
{

  if (iter == NULL ||
      cs_actor_has_ancestor (iter, cluttersmith->parasite_root))
    {
      return;
    }

  INDENT;
  g_string_append_printf (str, "{\n");
  *indentation+=2;
  INDENT;

  if (CLUTTER_IS_STAGE (iter))
    {
      g_print ("DUMPING stage!\n");
      g_string_append_printf (str, "\"id\":\"actor\",\n");
      g_string_append_printf (str, "\"type\":\"ClutterGroup\",\n");
    }
  else
    {
      g_string_append_printf (str, "\"type\":\"%s\",\n", G_OBJECT_TYPE_NAME (iter));
      properties_to_string (str, iter, indentation);
    }

  if (CLUTTER_IS_CONTAINER (iter))
    {
      GList *children, *c;
      children = clutter_container_get_children (CLUTTER_CONTAINER (iter));

      INDENT;
      g_string_append_printf (str, "\"children\":\[\n");
      for (c = children; c; c=c->next)
        {
          actor_to_string (str, c->data, indentation);
          if (c->next)
            {
              INDENT;
              g_string_append_printf (str, ",\n");
            }
        }
      INDENT;
      g_string_append_printf (str, "]\n");
      g_list_free (children);
    }
  *indentation-=2;
  INDENT;
  g_string_append_printf (str, "}\n");
}

gchar *json_serialize_subtree (ClutterActor *root)
{
  GString *str = g_string_new ("");
  gchar   *ret;
  gint     indentation = 0;
  actor_to_string (str, root, &indentation);
  ret = str->str;
  g_string_free (str, FALSE);
  return ret;
}


static void
animator_to_string (GString         *str,
                    ClutterAnimator *animator,
                    gint            *indentation)
{
  GList *keys = clutter_animator_get_keys (animator, NULL, NULL, -1.0);
  INDENT;
  g_string_append_printf (str, "{\n");
  *indentation+=2;
  INDENT;
  g_string_append_printf (str, "\"id\":\"%s\",\n", clutter_scriptable_get_id (CLUTTER_SCRIPTABLE (animator)));
  INDENT;
  g_string_append_printf (str, "\"type\":\"ClutterAnimator\",\n");
  INDENT;
  g_string_append_printf (str, "\"duration\":%i,\n", clutter_animator_get_duration (animator));
  INDENT;
  g_string_append_printf (str, "\"properties\":[\n");
  
  *indentation+=2;
  {
    GList *iter;
    const gchar *curprop = NULL;
    GObject *curobject = NULL;
    gboolean gotprop = FALSE;

    for (iter = keys; iter; iter=iter->next)
      {
        ClutterAnimatorKey *key = iter->data;
        const gchar *prop     = clutter_animator_key_get_property_name (key);
        GObject     *object   = clutter_animator_key_get_object (key);
        gdouble      progress = clutter_animator_key_get_progress (key);
        guint        mode     = clutter_animator_key_get_mode (key);


        if (object != curobject ||
            prop != curprop)
          {
            if (curobject == NULL)
              {
                INDENT;
                g_string_append_printf (str, "{\n");
                *indentation+=2;
              }
            else
              {
                *indentation-=2;
                INDENT;
                g_string_append_printf (str, "]\n");
                *indentation-=2;
                INDENT;
                g_string_append_printf (str, "},{\n");
                *indentation+=2;
              }
            INDENT;
            g_string_append_printf (str, "\"object\":\"%s\",\n", clutter_scriptable_get_id (CLUTTER_SCRIPTABLE (object)));
            INDENT;
            g_string_append_printf (str, "\"name\":\"%s\",\n", prop);
            INDENT;
            g_string_append_printf (str, "\"ease-in\":%s,\n",
              clutter_animator_property_get_ease_in (animator, object, prop)?"true":"false");
            INDENT;
            g_string_append_printf (str, "\"interpolation\":\"%s\",\n",
              clutter_animator_property_get_interpolation (animator, object, prop) ==
              CLUTTER_INTERPOLATION_LINEAR?"linear":"cubic");
            INDENT;
            g_string_append_printf (str, "\"keys\": [\n");
            gotprop = FALSE;
            *indentation+=2;
          }
        INDENT;

        {
          GValue value = {0,};
          g_value_init (&value, G_TYPE_STRING); /* XXX: this isnt very robust */
          clutter_animator_key_get_value (key, &value);
          g_string_append_printf (str, "%s[%f, \"%s\", %s]\n",
                                  gotprop?",":" ", progress, "linear", g_value_get_string (&value));
          g_value_unset (&value);
        }
        gotprop=TRUE;
        mode = 0; //XXX just to use it
        
        curobject = object;
        curprop = prop;
      }
    if (curobject)
      {
        *indentation-=2;
      }
  }
  INDENT;
  g_string_append_printf (str, "]\n");
  *indentation-=2;

  INDENT;
  g_string_append_printf (str, "}\n");
  *indentation-=2;

  INDENT;
  g_string_append_printf (str, "]\n");

  *indentation-=2;
  INDENT;
  g_string_append_printf (str, "}\n");

  g_list_free (keys);
}

gchar *json_serialize_animator (ClutterAnimator *animator)
{
  GString *str = g_string_new ("");
  gchar   *ret;
  gint     indentation = 0;
  animator_to_string (str, animator, &indentation);
  ret = str->str;
  g_string_free (str, FALSE);
  return ret;
}
