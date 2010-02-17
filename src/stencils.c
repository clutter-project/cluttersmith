#include <clutter/clutter.h>
#include <stdlib.h>
#include <string.h>
#include "cluttersmith.h"
#include "util.h"

static gboolean add_stencil (ClutterActor *actor,
                             ClutterEvent *event,
                             const gchar  *path)
{
  ClutterActor *parent;
  ClutterActor *new_actor;

  parent = cs_get_current_container ();
  new_actor = cs_load_json (path);
  
  if (new_actor)
    {
      gfloat sw, sh, w, h;

      clutter_container_add_actor (CLUTTER_CONTAINER (parent), new_actor);
      clutter_actor_get_size (clutter_actor_get_stage (actor), &sw, &sh);
      clutter_actor_get_size (new_actor, &w, &h);
      clutter_actor_set_position (new_actor, (sw-w)/2, (sh-h)/2);

      cs_selected_clear ();
      cs_selected_add (new_actor);
      clutter_scriptable_set_id (CLUTTER_SCRIPTABLE (new_actor), "");
    }

  cs_dirtied ();
  return TRUE;
}



void templates_container_init_hack (ClutterActor  *actor)
{
  /* we hook this up to the first paint, since no other signal seems to
   * be available to hook up for some additional initialization
   */
  static gboolean done = FALSE; 
  if (done)
    return;
  done = TRUE;

  {
    GDir *dir = g_dir_open (PKGDATADIR "templates", 0, NULL);
    const gchar *name;

    while ((name = g_dir_read_name (dir)))
      {
        ClutterColor  none = {0,0,0,0};
        ClutterActor *group;
        ClutterActor *rectangle;

        if (!g_str_has_suffix (name, ".json"))
          continue;

        rectangle = clutter_rectangle_new ();
        group = clutter_group_new ();


        clutter_rectangle_set_color (CLUTTER_RECTANGLE (rectangle), &none);
        clutter_actor_set_reactive (rectangle, TRUE);
          {
            gchar *path;
            ClutterActor *oi;
            path = g_strdup_printf (PKGDATADIR "templates/%s", name);
            oi = cs_load_json (path);
            if (oi)
              {
#define DIM 120.0

                gfloat width, height;
                gfloat scale;
                clutter_actor_get_size (oi, &width, &height);
                scale = DIM/width;
                if (DIM/height < scale)
                  scale = DIM/height;
                clutter_actor_set_scale (oi, scale, scale);
                clutter_actor_set_size (group, width*scale, height*scale);
                clutter_actor_set_size (rectangle, DIM, height*scale);

                clutter_container_add_actor (CLUTTER_CONTAINER (group), oi);
                clutter_container_add_actor (CLUTTER_CONTAINER (group), rectangle);
                g_object_set_data_full (G_OBJECT (oi), "path", path, g_free);
                g_signal_connect (rectangle, "button-press-event", G_CALLBACK (add_stencil), path);
              }
              else
              {
                g_free (path);
                }
          }
        clutter_container_add_actor (CLUTTER_CONTAINER (actor), group);
      }
    g_dir_close (dir);
  }
}


