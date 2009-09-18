#include <clutter/clutter.h>
#include <stdlib.h>
#include <string.h>
#include "cluttersmith.h"
#include "util.h"

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
  gchar *config_path = cluttersmith_make_config_file ("session-history");
  gchar *original = NULL;
  util_remove_children (actor);

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
              g_print ("%s\n", start);
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
