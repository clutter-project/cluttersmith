#include <nbtk/nbtk.h>
#include <clutter/clutter.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include "util.h"

static gboolean cb_filter_properties = TRUE;
extern ClutterActor *parasite_root;

#define INDENT {gint j;for (j=0;j<*indentation;j++) g_string_append_c (str, ' ');}

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

      if (cb_filter_properties)
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
                gchar *escaped = g_strdup (value); /* XXX: should do some proper escaping */
                INDENT;g_string_append_printf (str,"\"%s\":\"%s\",\n",
                                               properties[i]->name, value);
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

  g_free (properties);
}


static void
actor_to_string (GString      *str,
                 ClutterActor *iter,
                 gint         *indentation)
{

  if (iter == NULL ||
      util_has_ancestor (iter, parasite_root))
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

gchar *subtree_to_string (ClutterActor *root)
{
  GString *str = g_string_new ("");
  gchar   *ret;
  gint     indentation = 0;
  actor_to_string (str, root, &indentation);
  ret = str->str;
  g_string_free (str, FALSE);
  return ret;
}
