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


#include "cluttersmith.h"
#include "cs-context.h"
#include <gjs/gjs.h>
#include <gio/gio.h>
#include <string.h>

static GList *cs_actor_get_js_callbacks (ClutterActor *actor,
                                         const gchar  *signal)
{
  GHashTable *ht;
  ht = g_object_get_data (G_OBJECT (actor), "callbacks");
  if (!ht)
    return NULL;
  return g_hash_table_lookup (ht, signal);
}

static void
callback_text_changed (ClutterText  *text,
                       ClutterActor *actor)
{
  const gchar *new_text = clutter_text_get_text (text);
  GList *c, *callbacks;
  GHashTable *ht;
  gint i;
  const gchar *signal = g_object_get_data (G_OBJECT (text), "signal");
  gint no = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (text), "no"));

  ht = g_object_get_data (G_OBJECT (actor), "callbacks");
  callbacks = g_hash_table_lookup (ht, signal);
  for (i=0, c=callbacks; c; c=c->next, i++)
    {
      if (i==no)
        {
          g_free (c->data);
          c->data = g_strdup (new_text);
        }
    }
}


void cs_freecode (gpointer ht) 
{
  GHashTableIter iter;
  gpointer key, value;

  g_hash_table_iter_init (&iter, ht);
  while (g_hash_table_iter_next (&iter, &key, &value)) 
    {
      g_list_foreach (value, (GFunc)g_free, NULL);
      g_list_free (value);
    }
  g_hash_table_destroy (ht);
}


static void
callback_add (MxWidget *button,
              ClutterActor *actor)
{
  GList *callbacks;
  GHashTable *ht;
  gchar *signal = g_object_get_data (G_OBJECT (button), "signal");

  ht = g_object_get_data (G_OBJECT (actor), "callbacks");
  if (!ht)
    {
      ht = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
      g_object_set_data_full (G_OBJECT (actor), "callbacks", ht, cs_freecode);
    }

  callbacks = g_hash_table_lookup (ht, signal);
  callbacks = g_list_append (callbacks, g_strdup ("hello void"));
  g_hash_table_insert (ht, g_strdup (signal), callbacks);
  cs_callbacks_populate (actor);
}

static void
callback_removed (MxButton     *button,
                  ClutterActor *actor)
{
  GHashTable *ht;
  GList *callbacks;
  GObject *cb = g_object_get_data (G_OBJECT (button), "script");
  gchar *signal = g_object_get_data (G_OBJECT (cb), "signal");
  gint no = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (cb), "no"));
  g_print ("trying to remove a %s %i callback from %p\n", signal, no, actor);

  ht = g_object_get_data (G_OBJECT (actor), "callbacks");
  if (!ht)
    return;

  callbacks = g_hash_table_lookup (ht, signal);
  callbacks = g_list_remove (callbacks, g_list_nth_data (callbacks, no));
  g_hash_table_insert (ht, g_strdup (signal), callbacks);
  cs_callbacks_populate (actor);
}

/* popup a choice of,.. code, change scene, run animator 
 */
static void
callbacks_add_cb (ClutterActor *actor,
                  const gchar  *signal,
                  const gchar  *code,
                  gint          no)
{
  ClutterActor *cb = clutter_text_new_with_text ("Mono 10", code);

  ClutterActor *hbox   = g_object_new (MX_TYPE_BOX_LAYOUT, NULL);
  ClutterActor *remove = mx_button_new_with_label ("-");
  clutter_container_add (CLUTTER_CONTAINER (hbox), CLUTTER_ACTOR (remove),
                                                                   CLUTTER_ACTOR (cb), NULL);
  clutter_container_add_actor (CLUTTER_CONTAINER (cs->callbacks_container), hbox);
  g_object_set (G_OBJECT (cb), "editable", TRUE, "selectable", TRUE, "reactive", TRUE, NULL);
  g_object_set_data (G_OBJECT (cb), "no", GINT_TO_POINTER (no));
  g_object_set_data_full (G_OBJECT (cb), "signal", g_strdup (signal), g_free);
  g_object_set_data (G_OBJECT (remove), "script", cb);
  g_signal_connect (cb, "text-changed", G_CALLBACK (callback_text_changed), actor);

  g_signal_connect (remove, "clicked", G_CALLBACK (callback_removed), actor);

  clutter_container_add_actor (CLUTTER_CONTAINER (cs->callbacks_container), hbox);
}

void
cs_callbacks_populate (ClutterActor *actor)
{
  GType type = G_OBJECT_TYPE (actor);

  cs_container_remove_children (cs->callbacks_container);

  while (type)
  {
    guint *list;
    guint count;
    guint i;


    list = g_signal_list_ids (type, &count);
    
    for (i=0; i<count;i++)
      {
        GSignalQuery query;
        g_signal_query (list[i], &query);

        if (type == G_TYPE_OBJECT)
          continue;
        if (type == CLUTTER_TYPE_ACTOR)
          {
            gchar *white_list[]={"show", "motion-event", "enter-event", "leave-event", "button-press-event", "button-release-event", NULL};
            gint i;
            gboolean skip = TRUE;
            for (i=0;white_list[i];i++)
              if (g_str_equal (white_list[i], query.signal_name))
                skip = FALSE;
            if (skip)
              continue;
          }


          {
            ClutterActor *hbox  = g_object_new (MX_TYPE_BOX_LAYOUT, NULL);
            ClutterActor *title = mx_label_new ();
            ClutterActor *add   = mx_button_new_with_label ("+");

            mx_label_set_text (MX_LABEL (title), query.signal_name);

            g_object_set_data_full (G_OBJECT (add), "signal", g_strdup (query.signal_name), g_free);
            g_signal_connect (add, "clicked", G_CALLBACK (callback_add), actor);

            clutter_container_add (CLUTTER_CONTAINER (hbox), CLUTTER_ACTOR (add),
                                                             CLUTTER_ACTOR (title), NULL);
            clutter_container_add_actor (CLUTTER_CONTAINER (cs->callbacks_container), hbox);
            clutter_container_child_set (CLUTTER_CONTAINER (cs->callbacks_container),
                                         CLUTTER_ACTOR (title),
                                         "expand", TRUE,
                                         NULL);

            {
              gint no = 0;
              GList *l, *list = cs_actor_get_js_callbacks (actor, query.signal_name);
              for (l=list;l;l=l->next, no++)
                {
                  callbacks_add_cb (actor, query.signal_name, l->data, no);
                }
            }
          }
      }
    type = g_type_parent (type);
    g_free (list);
  }
}
