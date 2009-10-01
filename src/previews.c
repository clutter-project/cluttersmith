#include <clutter/clutter.h>
#include <stdlib.h>
#include <string.h>
#include "cluttersmith.h"
#include "util.h"

#define SIZE 70

static gboolean load_layout (ClutterActor *actor,
                             ClutterEvent *event,
                             const gchar  *path)
{
  gchar *new_title = g_strdup (path);
  gchar *dot;

  dot = strrchr(new_title, '.');
  if (dot)
    *dot='\0';
  
  cluttersmith_open_layout (new_title);
  g_free (new_title);
  return TRUE;
}

ClutterActor *preview_make (const gchar *name, const gchar *path)
{
  ClutterColor  none = {0,0,0,0};
  ClutterColor  white = {0xff,0xff,0,0xff};
  ClutterActor *group;
  ClutterActor *rectangle;
  ClutterActor *texture;
  ClutterActor *label;
  gchar *title = g_strdup (name);

  rectangle = clutter_rectangle_new ();
  group = clutter_group_new ();
  texture = clutter_texture_new ();

  clutter_actor_set_size (group, SIZE, SIZE);
  clutter_rectangle_set_color (CLUTTER_RECTANGLE (rectangle), &none);
  clutter_actor_set_reactive (rectangle, TRUE);
  clutter_actor_set_reactive (texture, TRUE);
  clutter_actor_set_size (rectangle, SIZE, SIZE);
  clutter_actor_set_size (texture, SIZE, SIZE);
  g_signal_connect (rectangle, "button-press-event", G_CALLBACK (load_layout), title);
  g_signal_connect (texture, "button-press-event", G_CALLBACK (load_layout), title);
  /* storing it as a data on one of the objects that has the
     * same lifetime as the callback using the string */
  g_object_set_data_full (G_OBJECT (group), "path", title, g_free);

    if (strrchr (title, '.'))
      {
        *strrchr (title, '.')='\0';
      }
    label = clutter_text_new_with_text ("Sans 9px", title);
    clutter_text_set_color (CLUTTER_TEXT (label), &white);

    if(0) /* disabled for now, we need to make screenshots of the scenes,
            a png exporter of some kind would be best, allowing a texture
            per stage */
      {
      ClutterActor *oi;
      oi = util_load_json (path);
      if (oi)
        {
          gfloat width, height;
          gfloat scale;
          clutter_actor_get_size (oi, &width, &height);
          width = 800;
          height = 600;
          scale = SIZE/width;
          if (SIZE/height < scale)
            scale = SIZE/height;
          clutter_actor_set_scale (oi, scale, scale);

          clutter_container_add_actor (CLUTTER_CONTAINER (group), oi);
        }
      else
        {
          g_free (title);
        }
    }
  clutter_container_add_actor (CLUTTER_CONTAINER (group), label);
  clutter_container_add_actor (CLUTTER_CONTAINER (group), rectangle);
  clutter_container_add_actor (CLUTTER_CONTAINER (group), texture);
  clutter_actor_hide (texture);
  return group;
}

void previews_reload (ClutterActor *actor)
{
  GDir *dir;
  const gchar *path = cluttersmith_get_project_root ();
  const gchar *name;

  if (!path)
    return;

  util_remove_children (actor);
  dir = g_dir_open (path, 0, NULL);
  while ((name = g_dir_read_name (dir)))
    {
      ClutterActor *preview;
      gchar *path;

      if (!g_str_has_suffix (name, ".json"))
        continue;

      path = g_strdup_printf ("%s/%s", cluttersmith_get_project_root (),
                              name);
      preview = preview_make (name, path);
      g_free (path);
      clutter_container_add_actor (CLUTTER_CONTAINER (actor), preview);
    }
  g_dir_close (dir);
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

  previews_reload (actor);
}



/* XXX: merge this hack into the above */
void previews_parent_init_hack (ClutterActor  *actor)
{
  static gboolean done = FALSE; 
  if (done)
    return;
  done = TRUE;

  clutter_container_child_set (CLUTTER_CONTAINER (clutter_actor_get_parent (actor)),
                               actor, "expand", TRUE, NULL);
}


