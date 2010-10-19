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

typedef enum {
  ActionScript,
  ActionState,
  ActionScene
} ActionType;

typedef struct {
  ClutterActor *actor;
  const gchar *signal;
  const gchar *sm_name;
  int no;
} CbData;

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
                       CbData       *cbd)
{
  const gchar *new_text = clutter_text_get_text (text);
  GList *c, *callbacks;
  GHashTable *ht;
  gint i;

  ht = g_object_get_data (G_OBJECT (cbd->actor), "callbacks");
  callbacks = g_hash_table_lookup (ht, cbd->signal);
  for (i=0, c=callbacks; c; c=c->next, i++)
    {
      if (i==cbd->no)
        {
          g_free (c->data);
          c->data = g_strdup (new_text);
          return;
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
callback_removed (MxButton     *button,
                  CbData       *cbd)
{
  GHashTable *ht;
  GList *callbacks;

  ht = g_object_get_data (G_OBJECT (cbd->actor), "callbacks");
  if (!ht)
    return;

  callbacks = g_hash_table_lookup (ht, cbd->signal);
  callbacks = g_list_remove (callbacks, g_list_nth_data (callbacks, cbd->no));
  g_hash_table_insert (ht, g_strdup (cbd->signal), callbacks);
  cs_callbacks_populate (cbd->actor);
}



static void change_sm (MxComboBox *combo_box,
                       GParamSpec *pspec,
                       gpointer    data)
{
  CbData *cbd = data;
  GList *c, *callbacks;
  GHashTable *ht;
  int i;
  const gchar *new_sm= mx_combo_box_get_active_text (combo_box);

  ht = g_object_get_data (G_OBJECT (cbd->actor), "callbacks");
  callbacks = g_hash_table_lookup (ht, cbd->signal);
  for (i=0, c=callbacks; c; c=c->next, i++)
    {
      if (i==cbd->no)
        {
          g_free (c->data);
          c->data = g_strdup_printf ("/*cs:state sm='%s' state=''*/", new_sm);
        }
    }
  cs_callbacks_populate (cbd->actor);
}

static void change_state (MxComboBox *combo_box,
                          GParamSpec *pspec,
                          gpointer    data)
{
  CbData *cbd = data;
  GList *c, *callbacks;
  GHashTable *ht;
  int i;
  const gchar *new_state = mx_combo_box_get_active_text (combo_box);

  ht = g_object_get_data (G_OBJECT (cbd->actor), "callbacks");
  callbacks = g_hash_table_lookup (ht, cbd->signal);
  for (i=0, c=callbacks; c; c=c->next, i++)
    {
      if (i==cbd->no)
        {
          g_free (c->data);
          c->data = g_strdup_printf ("/*cs:state sm='%s' state='%s'*/ $('%s').state='%s'\n",
           cbd->sm_name, new_state, cbd->sm_name, new_state);
        }
    }
  cs_callbacks_populate (cbd->actor);
}

static void change_scene (MxComboBox *combo_box,
                          GParamSpec *pspec,
                          gpointer    data)
{
  CbData *cbd = data;
  GList *c, *callbacks;
  GHashTable *ht;
  int i;
  const gchar *new_scene = mx_combo_box_get_active_text (combo_box);

  ht = g_object_get_data (G_OBJECT (cbd->actor), "callbacks");
  callbacks = g_hash_table_lookup (ht, cbd->signal);
  for (i=0, c=callbacks; c; c=c->next, i++)
    {
      if (i==cbd->no)
        {
          g_free (c->data);
          c->data = g_strdup_printf ("/*cs:scene scene='%s'*/ CS.get_context().scene='%s';\n", new_scene, new_scene);
        }
    }
  cs_callbacks_populate (cbd->actor);
}

static void change_type (MxComboBox *combo_box,
                         GParamSpec *pspec,
                         CbData     *cbd)
{
  GList *c, *callbacks;
  GHashTable *ht;
  int i;
  int new_type = mx_combo_box_get_index (combo_box);

  ht = g_object_get_data (G_OBJECT (cbd->actor), "callbacks");
  callbacks = g_hash_table_lookup (ht, cbd->signal);
  for (i=0, c=callbacks; c; c=c->next, i++)
    {
      if (i==cbd->no)
        {
          g_free (c->data);
          switch (new_type)
            {
              case ActionScript:
                c->data = g_strdup ("/* javascript */");
                break;
              case ActionState:
                c->data = g_strdup ("/*cs:state*/");
                break;
              case ActionScene:
                c->data = g_strdup ("/*cs:scene*/");
                break;
            }
        }
    }
  cs_callbacks_populate (cbd->actor);
}

/*
 * Need a struct holding the triplet of actor, signal and callback no for simpler passing
 * around that gobject data
 */

static void
callbacks_add_cb (ClutterActor *actor,
                  const gchar  *signal,
                  const gchar  *code,
                  gint          no)
{
  CbData *cbd = g_new0 (CbData, 1);
  ClutterActor *hbox   = g_object_new (MX_TYPE_BOX_LAYOUT, NULL);
  ClutterActor *remove = mx_button_new_with_label ("Remove");
  ClutterActor *dropdown = g_object_new (MX_TYPE_COMBO_BOX, NULL);
  ClutterActor *callback;
 
  char *tmp = g_strdup_printf ("On %s", signal);
  callback = mx_label_new_with_text (tmp);
  g_free (tmp);

  mx_combo_box_append_text (MX_COMBO_BOX (dropdown), "script");
  mx_combo_box_append_text (MX_COMBO_BOX (dropdown), "state");
  mx_combo_box_append_text (MX_COMBO_BOX (dropdown), "scene");

  clutter_container_add (CLUTTER_CONTAINER (hbox), callback, dropdown, remove, NULL);
  clutter_container_add_actor (CLUTTER_CONTAINER (cs->callbacks_container), hbox);
  clutter_container_child_set (CLUTTER_CONTAINER (hbox), callback, "expand", TRUE, NULL);

  if (g_str_has_prefix (code, "/*cs:state"))
    {
      ClutterActor *state_machines = g_object_new (MX_TYPE_COMBO_BOX, NULL);
      ClutterActor *states         = g_object_new (MX_TYPE_COMBO_BOX, NULL);
      gchar *name = NULL;
      int machine_no = -1;
      gchar *state_name = NULL;
      int state_no = -1;
      hbox   = g_object_new (MX_TYPE_BOX_LAYOUT, NULL);
      mx_combo_box_set_index (MX_COMBO_BOX (dropdown), ActionState);


      if (strstr (code, "sm='"))
        {
          name = g_strdup (strstr (code, "sm='") + 4);
          if (strchr (name, '\''))
            *strchr (name, '\'') = '\0';
          else
            {
              g_free (name);
              name = NULL;
            }
        }

      if (strstr (code, "state='"))
        {
          state_name = g_strdup (strstr (code, "state='") + 7);
          if (strchr (state_name, '\''))
            *strchr (state_name, '\'') = '\0';
          else
            {
              g_free (state_name);
              state_name = NULL;
            }
        }

      {
        GList *i;int no;
        for (i = cs->state_machines, no=0; i; i=i->next, no++)
          {
            const gchar *id = clutter_scriptable_get_id (i->data);
            mx_combo_box_append_text (MX_COMBO_BOX (state_machines), id);
            if (!g_strcmp0 (name, id))
              {
                GList *list, *s;
                int sno;
                list = clutter_state_get_states (i->data);
                for (s=list, sno = 0; s; s=s->next, sno++)
                  {
                    mx_combo_box_append_text (MX_COMBO_BOX (states), s->data);
                    if (!g_strcmp0 (state_name, s->data))
                      state_no = sno;
                  }
                machine_no = no;
                g_list_free (list);
              }
          }
      }
      if (machine_no>=0)
        mx_combo_box_set_index (MX_COMBO_BOX (state_machines), machine_no);
      if (state_no>=0)
        mx_combo_box_set_index (MX_COMBO_BOX (states), state_no);

      cbd->sm_name = g_intern_string (name);
      g_signal_connect (state_machines, "notify::index", G_CALLBACK (change_sm), cbd);
      g_signal_connect (states, "notify::index", G_CALLBACK (change_state), cbd);

      clutter_container_add (CLUTTER_CONTAINER (hbox), state_machines, states, NULL);
      clutter_container_add_actor (CLUTTER_CONTAINER (cs->callbacks_container), hbox);
    }
  else if (g_str_has_prefix (code, "/*cs:scene"))
    {
      ClutterActor *scenes = g_object_new (MX_TYPE_COMBO_BOX, NULL);
      GDir *dir;
      const gchar *path = cs_get_project_root ();
      gchar *scene_name = NULL;
      int    scene_no = -1;
      ClutterActor *label = mx_label_new_with_text ("Go to scene");
      mx_combo_box_set_index (MX_COMBO_BOX (dropdown), ActionScene);
      hbox   = g_object_new (MX_TYPE_BOX_LAYOUT, NULL);

      if (strstr (code, "scene='"))
        {
          scene_name = g_strdup (strstr (code, "scene='") + 7);
          if (strchr (scene_name, '\''))
            *strchr (scene_name, '\'') = '\0';
          else
            {
              g_free (scene_name);
              scene_name = NULL;
            }
        }

      if (path)
        {
          const gchar *name;
          dir = g_dir_open (path, 0, NULL);
          int no=0;
          while ((name = g_dir_read_name (dir)))
            {
              gchar *name2;
              if (!g_str_has_suffix (name, ".json"))
                continue;
              name2 = g_strdup (name);
              *strstr (name2, ".json")='\0';

              if (!g_strcmp0 (name2, scene_name))
                scene_no = no;
              mx_combo_box_append_text (MX_COMBO_BOX (scenes), name2);
              g_free (name2);
              no++;
            }
          g_dir_close (dir);
        }

      clutter_container_add (CLUTTER_CONTAINER (hbox), label, scenes, NULL);
      clutter_container_child_set (CLUTTER_CONTAINER (hbox), scenes, "expand", TRUE, NULL);
      clutter_container_add_actor (CLUTTER_CONTAINER (cs->callbacks_container), hbox);
      if (scene_no>=0)
        mx_combo_box_set_index (MX_COMBO_BOX (scenes), scene_no);
      g_signal_connect (scenes, "notify::index", G_CALLBACK (change_scene), cbd);
    }
  else /* javascript snippet */
    {
      ClutterColor  white = {0xff,0xff,0xff,0xff};
      ClutterActor *cb = clutter_text_new_with_text ("Mono 10px", code);
      mx_combo_box_set_index (MX_COMBO_BOX (dropdown), ActionScript);
      clutter_text_set_color (CLUTTER_TEXT (cb), &white);
      hbox   = g_object_new (MX_TYPE_BOX_LAYOUT, NULL);
      clutter_container_add (CLUTTER_CONTAINER (hbox), cb, NULL);
      clutter_container_add_actor (CLUTTER_CONTAINER (cs->callbacks_container), hbox);

      g_object_set (G_OBJECT (cb), "editable", TRUE, "selectable", TRUE, "reactive", TRUE, NULL);
      g_signal_connect (cb, "text-changed", G_CALLBACK (callback_text_changed), cbd);
    }

  cbd->actor = actor;
  cbd->no = no;
  cbd->signal = g_intern_string (signal);

  g_signal_connect (remove, "clicked", G_CALLBACK (callback_removed), cbd);
  g_signal_connect (dropdown, "notify::index", G_CALLBACK (change_type), cbd);

  g_object_set_data_full (G_OBJECT (hbox), "cbd", cbd, g_free);
}

/*  */
static GList *list_signals (GObject *object)
{
  GType type = G_OBJECT_TYPE (object);
  GList *ret = NULL;
  while (type)
  {
    guint *list;
    guint count;
    guint i;

    list = g_signal_list_ids (type, &count);
    
    for (i=0; i<count;i++)
      {
        GSignalQuery query;
        gboolean skip = FALSE;
        g_signal_query (list[i], &query);

        if (type == G_TYPE_OBJECT)
          continue;
        if (type == CLUTTER_TYPE_ACTOR)
          {
            gchar *white_list[]={"show", "motion-event", "enter-event", "leave-event", "button-press-event", "button-release-event", NULL};
            gint i;
            skip = TRUE;
            for (i=0;white_list[i];i++)
              if (g_str_equal (white_list[i], query.signal_name))
                skip = FALSE;
          }
        if (!skip)
          ret = g_list_prepend (ret, (void*)query.signal_name);
      }
    type = g_type_parent (type);
    g_free (list);
  }
  return ret;
}

static void create_cb (MxComboBox *combo_box,
                       GParamSpec *pspec,
                       gpointer    actor)
{
  GList *callbacks;
  GHashTable *ht;
  const gchar *signal = mx_combo_box_get_active_text (combo_box);

  ht = g_object_get_data (G_OBJECT (actor), "callbacks");
  if (!ht)
    {
      ht = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
      g_object_set_data_full (G_OBJECT (actor), "callbacks", ht, cs_freecode);
    }

  callbacks = g_hash_table_lookup (ht, signal);
  callbacks = g_list_append (callbacks, g_strdup ("/*cs:state*/"));
  g_hash_table_insert (ht, g_strdup (signal), callbacks);
  cs_callbacks_populate (actor);
}

void
cs_callbacks_populate (ClutterActor *actor)
{
  GList *s, *signals = list_signals (G_OBJECT (actor));
  cs_container_remove_children (cs->callbacks_container);

  {
    ClutterActor *hbox  = g_object_new (MX_TYPE_BOX_LAYOUT, NULL);
    ClutterActor *title = mx_label_new ();
    ClutterActor *dropdown = g_object_new (MX_TYPE_COMBO_BOX, NULL);

    mx_label_set_text (MX_LABEL (title), "Add for: ");
    clutter_container_add (CLUTTER_CONTAINER (hbox), CLUTTER_ACTOR (title),
                                                       CLUTTER_ACTOR (dropdown), NULL);
    clutter_container_add_actor (CLUTTER_CONTAINER (cs->callbacks_container), hbox);
    clutter_container_child_set (CLUTTER_CONTAINER (cs->callbacks_container),
                                 CLUTTER_ACTOR (dropdown),
                                 "expand", TRUE,
                                 NULL);
    for (s = signals; s; s=s->next)
      {
        mx_combo_box_append_text (MX_COMBO_BOX (dropdown), s->data);
      }
    g_signal_connect (dropdown, "notify::index", G_CALLBACK (create_cb), actor);
  }

  for (s = signals; s; s=s->next)
    {
      gint no = 0;
      GList *l, *list = cs_actor_get_js_callbacks (actor, s->data);
      for (l=list;l;l=l->next, no++)
        callbacks_add_cb (actor, s->data, l->data, no);
    }
  g_list_free (signals);
}
