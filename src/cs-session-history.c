#include <clutter/clutter.h>
#include <stdlib.h>
#include <string.h>
#include "cluttersmith.h"

#define SIZE 70


static gboolean set_working_dir (ClutterText *text)
{
  const gchar *dest = clutter_text_get_text (text);
  cluttersmith_set_project_root (dest);
  return TRUE;
}

static gboolean link_enter (ClutterText *text)
{
  ClutterColor color = {0xff,0x00,0x00,0xff};
  clutter_text_set_color (text, &color);
  clutter_actor_queue_redraw (CLUTTER_ACTOR (text));
  return TRUE;
}

static gboolean link_leave (ClutterText *text)
{
  ClutterColor color = {0x00,0x00,0x00,0xff};
  clutter_text_set_color (text, &color);
  clutter_actor_queue_redraw (CLUTTER_ACTOR (text));
  return TRUE;
}

void session_history_init_hack (ClutterActor  *actor)
{
  /* we hook this up to the first paint, since no other signal seems to
   * be available to hook up for some additional initialization
   */
  if (g_object_get_data (G_OBJECT (actor), "init-hack-done"))
    return;
  g_object_set_data (G_OBJECT (actor), "init-hack-done", (void*)0xff);

    {
  gchar *config_path = cs_make_config_file ("session-history");
  gchar *original = NULL;
  cs_container_remove_children (actor);

  if (g_file_get_contents (config_path, &original, NULL, NULL))
    {
      gchar *start, *end;
      start=end=original;
      while (*start)
        {
          end = strchr (start, '\n');
          if (*end)
            {
              ClutterActor *foo;
              *end = '\0';
              foo = clutter_text_new_with_text ("Sans 15px", start);
              clutter_container_add_actor (CLUTTER_CONTAINER (actor), foo);
              clutter_actor_set_reactive (foo, TRUE);
              g_signal_connect (foo, "button-press-event", G_CALLBACK (set_working_dir), NULL);
              g_signal_connect (foo, "enter-event", G_CALLBACK (link_enter), NULL);
              g_signal_connect (foo, "leave-event", G_CALLBACK (link_leave), NULL);
              clutter_actor_set_name (foo, "cluttersmith-is-interactive");
              start = end+1;
            }
          else
            {
              start = end;
            }
        }
      g_free (original);
    }
  }
}

void cs_session_history_add (const gchar *dir)
{
  gchar *config_path = cs_make_config_file ("session-history");
  gchar *start, *end;
  gchar *original = NULL;
  GList *iter, *session_history = NULL;
  
  if (g_file_get_contents (config_path, &original, NULL, NULL))
    {
      start=end=original;
      while (*start)
        {
          end = strchr (start, '\n');
          if (*end)
            {
              *end = '\0';
              if (!g_str_equal (start, dir))
                session_history = g_list_append (session_history, start);
              start = end+1;
            }
          else
            {
              start = end;
            }
        }
    }
  session_history = g_list_prepend (session_history, (void*)dir);
  {
    GString *str = g_string_new ("");
    gint i=0;
    for (iter = session_history; iter && i<10; iter=iter->next, i++)
      {
        g_string_append_printf (str, "%s\n", (gchar*)iter->data);
      }
    g_file_set_contents (config_path, str->str, -1, NULL);
    g_string_free (str, TRUE);
  }

  if (original)
    g_free (original);
  g_list_free (session_history);
}
