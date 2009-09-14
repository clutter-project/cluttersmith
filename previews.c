#include <clutter/clutter.h>
#include <stdlib.h>
#include <string.h>
#include "clutterbug.h"

static gboolean load_layout (ClutterActor *actor,
                             ClutterEvent *event,
                             const gchar  *path)
{
  gchar *new_title = g_strdup (path);
  gchar *dot;

  dot = strrchr(new_title, '.');
  if (dot)
    *dot='\0';
  set_title (new_title+5);
  g_free (new_title);
  return TRUE;
}


void previews_container_init_hack (ClutterActor  *actor)
{
  /* we hook this up to the first paint, since no other signal seems to
   * be available to hook up for some additional initialization
   */
  static gboolean done = FALSE; 
  if (done)
    return;
  done = TRUE;

  {
    GDir *dir = g_dir_open ("json", 0, NULL);
    const gchar *name;
    while ((name = g_dir_read_name (dir)))
      {
        ClutterColor  none = {0,0,0,0};
        ClutterColor  white = {0xff,0xff,0,0xff};
        ClutterActor *group;
        ClutterActor *rectangle;

        if (!g_str_has_suffix (name, ".json"))
          continue;

        rectangle = clutter_rectangle_new ();
        group = clutter_group_new ();

        clutter_actor_set_size (group, 100, 100);
        clutter_rectangle_set_color (CLUTTER_RECTANGLE (rectangle), &none);
        clutter_actor_set_reactive (rectangle, TRUE);

        ClutterActor *label;
        {
          gchar *title = g_strdup (name);
          if (strrchr (title, '.'))
            {
              *strrchr (title, '.')='\0';
            }
          label = clutter_text_new_with_text ("Sans 10px", title);
          clutter_text_set_color (CLUTTER_TEXT (label), &white);
          g_free (title);
        }

          {
            gchar *path;
            ClutterActor *oi;
            path = g_strdup_printf ("json/%s", name);
            oi = util_load_json (path);
            if (oi)
              {
                gfloat width, height;
                gfloat scale;
                clutter_actor_get_size (oi, &width, &height);
                width = 800;
                height = 600;
                scale = 100/width;
                if (100/height < scale)
                  scale = 100/height;
                clutter_actor_set_scale (oi, scale, scale);

                clutter_container_add_actor (CLUTTER_CONTAINER (group), oi);
                g_object_set_data_full (G_OBJECT (oi), "path", path, g_free);
                g_signal_connect (rectangle, "button-press-event", G_CALLBACK (load_layout), path);
              }
            else
              {
                g_free (path);
              }
            clutter_actor_set_size (rectangle, 100, 100);
          }
        clutter_container_add_actor (CLUTTER_CONTAINER (group), label);
        clutter_container_add_actor (CLUTTER_CONTAINER (group), rectangle);
        clutter_container_add_actor (CLUTTER_CONTAINER (actor), group);
      }
    g_dir_close (dir);
  }
}


