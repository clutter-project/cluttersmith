#include <nbtk/nbtk.h>
#include <clutter/clutter.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include "util.h"

static gboolean keep_on_top (gpointer actor)
{
  clutter_actor_raise_top (actor);
  return TRUE;
}

static ClutterActor *selected_actor = NULL;

static void
cb_overlay_paint (ClutterActor *actor,
                  gpointer      user_data)
{
  ClutterVertex verts[4];

  if (!selected_actor)
    return;

  clutter_actor_get_abs_allocation_vertices (selected_actor,
                                             verts);

  cogl_set_source_color4ub (0, 25, 0, 50);

  {
    CoglTextureVertex tverts[] = 
       {
         {verts[0].x, verts[0].y, },
         {verts[2].x, verts[2].y, },
         {verts[3].x, verts[3].y, },
         {verts[1].x, verts[1].y, },
       };
    cogl_polygon (tverts, 4, FALSE);

    {
      gfloat coords[]={ verts[0].x, verts[0].y, 
         verts[1].x, verts[1].y, 
         verts[3].x, verts[3].y, 
         verts[2].x, verts[2].y, 
         verts[0].x, verts[0].y };

      cogl_path_polyline (coords, 5);
      cogl_set_source_color4ub (255, 0, 0, 128);
      cogl_path_stroke ();
    }
  }

}

#define PKGDATADIR "/home/pippin/src/clutterbug/"

static gboolean idle_add_stage (gpointer stage)
{
  ClutterActor *actor = util_load_json (PKGDATADIR "parasite.json");
    //cp_parasite_new (CLUTTER_STAGE (stage));
  clutter_container_add_actor (CLUTTER_CONTAINER (stage), actor);
  g_signal_connect_after (stage, "paint", G_CALLBACK (cb_overlay_paint), NULL);
  g_timeout_add (400, keep_on_top, actor);

  nbtk_style_load_from_file (nbtk_style_get_default (), PKGDATADIR "clutterbug.css", NULL);

  return FALSE;
}

static void stage_added (ClutterStageManager *manager,
                         ClutterStage        *stage)
{
  g_timeout_add (100, idle_add_stage, stage);
}

static void _clutterbug_init(void)
    __attribute__ ((constructor));
static void _clutterbug_fini(void)
    __attribute__ ((destructor));

static void _clutterbug_init(void) {
  g_type_init ();
  ClutterStageManager *manager = clutter_stage_manager_get_default ();

  g_signal_connect (manager, "stage-added", G_CALLBACK (stage_added), NULL);
}

static void _clutterbug_fini(void) {
}

static guint stage_capture_handler = 0;
static ClutterActor  *name, *parents, *properties;





static void
props_to_str (ClutterActor *actor,
              GString      *str)
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


  for (i = 0; i < n_properties; i++)
    {
      gint j;
      gboolean skip = FALSE;
      for (j=0;j<n_actor_properties;j++)
        {
          if (actor_properties[j]==properties[i])
            {
              gchar *keep[]={"x","y","width","heigth","scale-x","scale-y", "anchor-x", "anchor-y", "rotation-angle-z", NULL};
              gint k;
              skip = TRUE;
              for (k=0;keep[k];k++)
                if (g_str_equal (properties[i]->name, keep[k]))
                  skip = FALSE;
            }
        }

      if (!(properties[i]->flags & G_PARAM_READABLE))
        skip = TRUE;

      if (skip)
        continue;
      if (properties[i]->value_type == G_TYPE_FLOAT)
        {
          gfloat value;
          g_object_get (actor, properties[i]->name, &value, NULL);
          g_string_append_printf (str, "%s:%2.2f\n", properties[i]->name, value);
        }
      else if (properties[i]->value_type == G_TYPE_DOUBLE)
        {
          gdouble value;
          g_object_get (actor, properties[i]->name, &value, NULL);
          g_string_append_printf (str, "%s:%2.2f\n", properties[i]->name, value);
        }
      else if (properties[i]->value_type == G_TYPE_INT)
        {
          gint  value;
          g_object_get (actor, properties[i]->name, &value, NULL);
          g_string_append_printf (str, "%s:%i\n", properties[i]->name, value);
        }
      else if (properties[i]->value_type == G_TYPE_BOOLEAN)
        {
          gboolean value;
          g_object_get (actor, properties[i]->name, &value, NULL);
          g_string_append_printf (str, "%s:%s\n", properties[i]->name, value?"true":"false");
        }
      else if (properties[i]->value_type == G_TYPE_STRING)
        {
          gchar *value;
          g_object_get (actor, properties[i]->name, &value, NULL);
          g_string_append_printf (str, "%s:%s\n", properties[i]->name, value);
          g_free (value);
        }
      else
        {
          g_string_append_printf (str, "%s:--%s--\n", properties[i]->name,
                           g_type_name (properties[i]->value_type));
        }
    }

  g_free (properties);
}

static void select_item (ClutterActor *button, ClutterActor *item)
{
          clutter_text_set_text (CLUTTER_TEXT (name), G_OBJECT_TYPE_NAME (item));

          util_remove_children (parents);

          if (!CLUTTER_IS_ACTOR (item))
            {
              g_print ("hit non actor\n");
            }
          else
            {
              ClutterActor *iter = clutter_actor_get_parent (item);
              while (iter && CLUTTER_IS_ACTOR (iter))
                {
                  ClutterActor *new;
                  new = CLUTTER_ACTOR (nbtk_button_new_with_label (G_OBJECT_TYPE_NAME (iter)));
                  g_signal_connect (new, "clicked", G_CALLBACK (select_item), iter);
                  clutter_container_add_actor (CLUTTER_CONTAINER (parents), new);
                  iter = clutter_actor_get_parent (iter);
                }

              GString *str = g_string_new ("");
              if (CLUTTER_IS_ACTOR (item))
                props_to_str (item, str); 
              clutter_text_set_text (CLUTTER_TEXT (properties), str->str);

              selected_actor = item;
            }
}

static gboolean
pick_capture (ClutterActor *actor, ClutterEvent *event, gpointer data)
{
  switch (event->any.type)
    {
      case CLUTTER_MOTION:
        {
          ClutterActor *hit;

          hit = clutter_stage_get_actor_at_pos (CLUTTER_STAGE (clutter_actor_get_stage (actor)),
                                                CLUTTER_PICK_ALL,
                                                event->motion.x, event->motion.y);
          select_item (NULL, hit);
        }
        break;
      case CLUTTER_BUTTON_PRESS:
        g_signal_handler_disconnect (clutter_actor_get_stage (actor),
                                     stage_capture_handler);
        stage_capture_handler = 0;
        break;
    }
  return TRUE;
}

void cb_select (ClutterActor *actor)
{
  ClutterScript *script = util_get_script (actor);

  name = CLUTTER_ACTOR (clutter_script_get_object (script, "name"));
  parents = CLUTTER_ACTOR (clutter_script_get_object (script, "parents"));
  properties = CLUTTER_ACTOR (clutter_script_get_object (script, "properties"));

  stage_capture_handler = 
     g_signal_connect (clutter_actor_get_stage (actor), "captured-event",
                       G_CALLBACK (pick_capture), NULL);

}

void cb_collapse (ClutterActor *actor)
{
  ClutterScript *script = util_get_script (actor);
  ClutterActor  *parasite = CLUTTER_ACTOR (clutter_script_get_object (script, "actor"));
  static gboolean collapsed = FALSE;

  collapsed = !collapsed;

  if (collapsed)
    clutter_actor_animate (parasite, CLUTTER_LINEAR, 200, "height", 22.0, NULL);
  else
    clutter_actor_animate (parasite, CLUTTER_LINEAR, 200, "height", 400.0, NULL);
}

