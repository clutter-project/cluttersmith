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


#include <clutter/clutter.h>
#include <gjs/gjs.h>
#include <string.h>
#include "cluttersmith.h"

typedef struct HistoryItem {
  const gchar    *name;
  gchar          *javascript_do;
  gchar          *javascript_undo;
} HistoryItem;

static GList *undo_commands = NULL;
static GList *redo_commands = NULL;

static inline void
item_free (HistoryItem *item)
{
  g_free (item->javascript_do);
  g_free (item->javascript_undo);
  g_free (item);
}


void
cs_history_add (const gchar *name,
                const gchar *javascript_do,
                const gchar *javascript_undo)
{
  HistoryItem *hitem = g_new0 (HistoryItem, 1);
  hitem->name = g_intern_string (name);
  hitem->javascript_do = g_strdup (javascript_do);
  hitem->javascript_undo = g_strdup (javascript_undo);
  undo_commands = g_list_prepend (undo_commands, hitem);

  /* XXX: Todo if the previous modification was a modification of the same
   * property, collapse the edits.
   */ 

  /* empty redo list */
  for (;redo_commands;redo_commands = g_list_remove (redo_commands, redo_commands->data))
    item_free (redo_commands->data);
}

void
cs_history_do (const gchar *name,
               const gchar *javascript_do,
               const gchar *javascript_undo)
{
  HistoryItem *hitem;
  cs_history_add (name, javascript_do, javascript_undo);
  hitem = undo_commands->data;

  {
    GjsContext *js_context;
    GError     *error = NULL;
    gint code;

    js_context = gjs_context_new_with_search_path (NULL);
    gjs_context_eval (js_context, JS_PREAMBLE, strlen (JS_PREAMBLE), "<code>", &code, &error);

    if (!gjs_context_eval (js_context, hitem->javascript_do, strlen (hitem->javascript_do),
                           "<code>", &code, &error))
      {
        g_warning ("%s", error->message);
      }
    g_object_unref (js_context);
  }
}


void cs_history_undo (ClutterActor *ignored)
{
  HistoryItem *hitem;
  gint group_level = 0;
   

  if (!undo_commands)
    {
      g_warning ("Undo attempted with no undos in history\n");
      return;
    }

  do 
    {
      hitem = undo_commands->data;
      
      if (!strcmp (hitem->name, "("))
        {
          if (group_level == 0)
            g_warning ("%s unexpected undogroup start\n", G_STRLOC);
          group_level --;
        }
      else if (!strcmp (hitem->name, ")"))
        {
          group_level++;
        }
      else
        {
          GjsContext *js_context;
          GError     *error = NULL;
          gint        code;

          js_context = gjs_context_new_with_search_path (NULL);
          gjs_context_eval (js_context, JS_PREAMBLE, strlen (JS_PREAMBLE), "<code>", &code, &error);
          if (!gjs_context_eval (js_context, hitem->javascript_undo, strlen (hitem->javascript_undo),
                                 "<code>", &code, &error))
            {
              g_warning ("%s", error->message);
            }
          g_object_unref (js_context);
        }

      redo_commands = g_list_prepend (redo_commands, undo_commands->data);
      undo_commands = g_list_remove (undo_commands, redo_commands->data);
    }
  while (group_level > 0);
}

void cs_history_redo (ClutterActor *ignored)
{
  HistoryItem *hitem;
  gint group_level = 0;

  if (!redo_commands)
    {
      g_warning ("Redo attempted with no redos in history\n");
      return;
    }

  do 
    {
      hitem = redo_commands->data;

      if (!strcmp (hitem->name, "("))
        {
          group_level ++;
        }
      else if (!strcmp (hitem->name, ")"))
        {
          if (group_level == 0)
            g_warning ("%s unexpected undogroup end", G_STRLOC);
          group_level--;
        }
      else
        {
          GjsContext *js_context;
          GError     *error = NULL;
          gint        code;

          js_context = gjs_context_new_with_search_path (NULL);
          gjs_context_eval (js_context, JS_PREAMBLE, strlen (JS_PREAMBLE), "<code>", &code, &error);
          if (!gjs_context_eval (js_context, hitem->javascript_do, strlen (hitem->javascript_do),
                                 "<code>", &code, &error))
            {
              g_warning ("%s", error->message);
            }
          g_object_unref (js_context);
        }
      undo_commands = g_list_prepend (undo_commands, redo_commands->data);
      redo_commands = g_list_remove (redo_commands, undo_commands->data);
    }
  while (group_level > 0);
}

/*
 * can be used to create a group of commands that
 * belong together.
 */
void cs_history_start_group (const gchar *group_name)
{
  cs_history_add ("(", NULL, NULL);
}
void cs_history_end_group   (const gchar *group_name)
{
  cs_history_add (")", NULL, NULL);
}
