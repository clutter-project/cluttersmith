#include <clutter/clutter.h>
#include <gjs/gjs.h>
#include <string.h>

typedef struct HistoryItem {
  const gchar    *name;
  gchar          *javascript_do;
  gchar          *javascript_undo;
} HistoryItem;

static GList *undo_commands = NULL;
static GList *redo_commands = NULL;

static void print_undo (void)
{
  GList *i;
  g_print ("undos:\n");
  for (i = g_list_last (undo_commands);
       i;
       i = i->prev)
    {
      HistoryItem *hitem = i->data;
      g_print ("  %s '%s' '%s'\n", hitem->name, hitem->javascript_do,
                                                hitem->javascript_undo);
    }

  g_print ("redos:\n");
  for (i = redo_commands;
       i;
       i = i->next)
    {
      HistoryItem *hitem = i->data;
      g_print ("  %s '%s' '%s'\n", hitem->name, hitem->javascript_do,
                                                hitem->javascript_undo);
    }
}

static inline void
item_free (HistoryItem *item)
{
  g_free (item->javascript_do);
  g_free (item->javascript_undo);
  g_free (item);
}

void
cs_history_do (const gchar *name,
               const gchar *javascript_do,
               const gchar *javascript_undo)
{
  HistoryItem *hitem = g_new0 (HistoryItem, 1);
  hitem->name = g_intern_string (name);
  hitem->javascript_do = g_strdup (javascript_do);
  hitem->javascript_undo = g_strdup (javascript_undo);
  undo_commands = g_list_append (undo_commands, hitem);

  /* empty redo list */
  for (;redo_commands;redo_commands = g_list_remove (redo_commands, redo_commands->data))
    item_free (redo_commands->data);

  {
    GjsContext *js_context;
    GError     *error = NULL;
    gint code;

    g_print ("running: %s\n", hitem->javascript_do);
    js_context = gjs_context_new_with_search_path (NULL);
    if (!gjs_context_eval (js_context, hitem->javascript_do, strlen (hitem->javascript_do),
                           "<code>", &code, &error))
      {
        g_print ("%s", error->message);
      }
    g_object_unref (js_context);
  }

  /* XXX: if no undo command was provided, a soft, application controlled git snapshot could be desirable
   */

  print_undo ();
}

void cs_undo (ClutterActor *ignored)
{
  HistoryItem *hitem;
  g_print ("CS: Undo\n");

  if (!undo_commands)
    {
      g_print ("attemptd undo with no undos\n");
      return;
    }
  hitem = undo_commands->data;
  {
    GjsContext *js_context;
    GError     *error = NULL;
    gint        code;

    g_print ("running: %s\n", hitem->javascript_undo);
    js_context = gjs_context_new_with_search_path (NULL);
    if (!gjs_context_eval (js_context, hitem->javascript_undo, strlen (hitem->javascript_undo),
                           "<code>", &code, &error))
      {
        g_print ("%s", error->message);
      }
    g_object_unref (js_context);
  }

  redo_commands = g_list_prepend (redo_commands, undo_commands->data);
  undo_commands = g_list_remove (undo_commands, redo_commands->data);

  print_undo ();
}

void cs_redo (ClutterActor *ignored)
{
  HistoryItem *hitem;
  g_print ("CS: Redo\n");

  if (!redo_commands)
    {
      g_print ("attemptd redo with no redos\n");
      return;
    }
  hitem = redo_commands->data;
  {
    GjsContext *js_context;
    GError     *error = NULL;
    gint        code;

    g_print ("running: %s\n", hitem->javascript_do);
    js_context = gjs_context_new_with_search_path (NULL);
    if (!gjs_context_eval (js_context, hitem->javascript_do, strlen (hitem->javascript_do),
                           "<code>", &code, &error))
      {
        g_print ("%s", error->message);
      }
    g_object_unref (js_context);
  }
  undo_commands = g_list_prepend (undo_commands, redo_commands->data);
  redo_commands = g_list_remove (redo_commands, undo_commands->data);

  print_undo ();
}
