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
static ClutterActor  *name, *parents, *property_editors;


typedef struct UpdateClosure
{
  ClutterActor *actor;
  gchar *name;
} UpdateClosure;

static void update_closure_free (gpointer data, GClosure *closure)
{
  UpdateClosure *uc = data;
  g_free (uc->name);
  g_free (uc);
}

static gboolean update_float_from_entry (ClutterText *text,
                                         gpointer     data)
{
  UpdateClosure *uc = data;
  g_object_set (uc->actor, uc->name, atof (clutter_text_get_text (text)), NULL);
}

static gboolean update_int_from_entry (ClutterText *text,
                                         gpointer     data)
{
  UpdateClosure *uc = data;
  g_object_set (uc->actor, uc->name, atoi (clutter_text_get_text (text)), NULL);
}


static gboolean update_uint_from_entry (ClutterText *text,
                                         gpointer     data)
{
  UpdateClosure *uc = data;
  g_object_set (uc->actor, uc->name, atoi (clutter_text_get_text (text)), NULL);
}

static gboolean update_string_from_entry (ClutterText *text,
                                         gpointer     data)
{
  UpdateClosure *uc = data;
  g_object_set (uc->actor, uc->name, clutter_text_get_text (text), NULL);
}

static gboolean update_generic_from_entry (ClutterText *text,
                                           gpointer     data)
{
  GParamSpec *pspec;
  GValue value = {0,};
  GValue str_value = {0,};
  UpdateClosure *uc = data;

  pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (uc->actor), uc->name);

  g_value_init (&str_value, G_TYPE_STRING);
  g_value_set_string (&str_value, clutter_text_get_text (text));

  g_value_init (&value, pspec->value_type);

  if (g_value_transform (&str_value, &value))
    {
      g_object_set_property (G_OBJECT (uc->actor), uc->name, &value);
    }
  g_value_unset (&value);
  g_value_unset (&str_value);
}


static gboolean update_boolean (NbtkButton *button,
                                gpointer    data)
{
  UpdateClosure *uc = data;
  g_object_set (uc->actor, uc->name, nbtk_button_get_checked (button), NULL);
    {
      nbtk_button_set_label (button,  nbtk_button_get_checked (button)?" 1 ":" 0 ");
    }
}

gboolean cb_filter_properties = TRUE;


static void
props_populate (ClutterActor *actor)
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

      if (cb_filter_properties)
        {
          for (j=0;j<n_actor_properties;j++)
            {
              /* ClutterActor contains so many properties that we restrict our view a bit */
              if (actor_properties[j]==properties[i])
                {
                  gchar *whitelist[]={"x","y", "depth", "opacity", "width","heigth",
                                      "scale-x","scale-y", "anchor-x", "color", 
                                      "anchor-y", "rotation-angle-z",
                                      "name", 
                                      NULL};
                  gint k;
                  skip = TRUE;
                  for (k=0;keep[k];k++)
                    if (g_str_equal (properties[i]->name, whitelist[k]))
                      skip = FALSE;
                }
            }
        }

      if (!(properties[i]->flags & G_PARAM_READABLE))
        skip = TRUE;

      if (skip)
        continue;

      {
        ClutterColor  white = {0xff,0xff,0xff,0xff};
        ClutterActor *label = clutter_text_new_with_text ("Sans 12px", properties[i]->name);
        ClutterActor *editor = NULL;

        UpdateClosure *uc = g_new0 (UpdateClosure, 1);
        uc->name = g_strdup (properties[i]->name);
        uc->actor = actor;

        clutter_text_set_color (CLUTTER_TEXT (label), &white);
        if (properties[i]->value_type == G_TYPE_FLOAT)
          {

            gchar *initial;
            gfloat value;
            g_object_get (actor, properties[i]->name, &value, NULL);
            initial = g_strdup_printf ("%2.3f", value);
            editor = CLUTTER_ACTOR (nbtk_entry_new (initial));
            g_free (initial);

            g_signal_connect_data (nbtk_entry_get_clutter_text (NBTK_ENTRY (editor)), "text-changed",
                                   G_CALLBACK (update_float_from_entry), uc, update_closure_free, 0);
          }
        else if (properties[i]->value_type == G_TYPE_DOUBLE)
          {
            gchar *initial;
            gdouble value;
            g_object_get (actor, properties[i]->name, &value, NULL);
            initial = g_strdup_printf ("%2.2f", value);
            editor = CLUTTER_ACTOR (nbtk_entry_new (initial));
            g_free (initial);

            g_signal_connect_data (nbtk_entry_get_clutter_text (NBTK_ENTRY (editor)), "text-changed",
                                   G_CALLBACK (update_float_from_entry), uc, update_closure_free, 0);

          }
        else if (properties[i]->value_type == G_TYPE_UCHAR)
          {
            gchar *initial;
            guchar value;
            g_object_get (actor, properties[i]->name, &value, NULL);
            initial = g_strdup_printf ("%i", (gint)value);
            editor = CLUTTER_ACTOR (nbtk_entry_new (initial));
            g_free (initial);

            g_signal_connect_data (nbtk_entry_get_clutter_text (NBTK_ENTRY (editor)), "text-changed",
                                   G_CALLBACK (update_int_from_entry), uc, update_closure_free, 0);

          }
        else if (properties[i]->value_type == G_TYPE_INT)
          {
            gchar *initial;
            gint value;
            g_object_get (actor, properties[i]->name, &value, NULL);
            initial = g_strdup_printf ("%i", value);
            editor = CLUTTER_ACTOR (nbtk_entry_new (initial));
            g_free (initial);

            g_signal_connect_data (nbtk_entry_get_clutter_text (NBTK_ENTRY (editor)), "text-changed",
                                   G_CALLBACK (update_int_from_entry), uc, update_closure_free, 0);

          }
        else if (properties[i]->value_type == G_TYPE_UINT)
          {
            gchar *initial;
            guint value;
            g_object_get (actor, properties[i]->name, &value, NULL);
            initial = g_strdup_printf ("%u", value);
            editor = CLUTTER_ACTOR (nbtk_entry_new (initial));
            g_free (initial);

            g_signal_connect_data (nbtk_entry_get_clutter_text (NBTK_ENTRY (editor)), "text-changed",
                                   G_CALLBACK (update_uint_from_entry), uc, update_closure_free, 0);

          }
        else if (properties[i]->value_type == G_TYPE_STRING)
          {
            gchar *value;
            g_object_get (actor, properties[i]->name, &value, NULL);
            editor = CLUTTER_ACTOR (nbtk_entry_new (value));
            g_free (value);

            g_signal_connect_data (nbtk_entry_get_clutter_text (NBTK_ENTRY (editor)), "text-changed",
                                   G_CALLBACK (update_string_from_entry), uc, update_closure_free, 0);

          }
        else if (properties[i]->value_type == G_TYPE_BOOLEAN)
          {
            gboolean value;
            g_object_get (actor, properties[i]->name, &value, NULL);
            editor = CLUTTER_ACTOR (nbtk_button_new_with_label (value?" 1 ":" 0 "));
            g_object_set (editor, "toggle-mode", TRUE, "checked", value, NULL);

            g_signal_connect_data (editor, "clicked",
                                   G_CALLBACK (update_boolean), uc, update_closure_free, 0);

          }
        else
          {
            GValue value = {0,};
            GValue str_value = {0,};
            gchar *initial;
            g_value_init (&value, properties[i]->value_type);
            g_value_init (&str_value, G_TYPE_STRING);
            g_object_get_property (G_OBJECT (actor), properties[i]->name, &value);
            if (g_value_transform (&value, &str_value))
              {
                initial = g_strdup_printf ("%s", g_value_get_string (&str_value));
                editor = CLUTTER_ACTOR (nbtk_entry_new (initial));

                g_free (initial);

                g_signal_connect_data (nbtk_entry_get_clutter_text (NBTK_ENTRY (editor)), "text-changed",
                                       G_CALLBACK (update_generic_from_entry), uc, update_closure_free, 0);
              }
            else
              {
                initial = g_strdup_printf ("%s", g_type_name (properties[i]->value_type));
                editor = clutter_text_new_with_text ("Sans 12px", initial);
                update_closure_free (uc, NULL);
              }
            /*g_value_unset (&value);
            g_value_unset (&str_value);
            g_free (initial);*/
          }

        {
          ClutterActor *hbox = g_object_new (NBTK_TYPE_BOX_LAYOUT, NULL);
          clutter_actor_set_size (label, 130, 32);
          clutter_actor_set_size (editor, 130, 32);
          clutter_container_add_actor (CLUTTER_CONTAINER (hbox), label);
          clutter_container_add_actor (CLUTTER_CONTAINER (hbox), editor);
          clutter_container_add_actor (CLUTTER_CONTAINER (property_editors), hbox);
        }
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

      util_remove_children (property_editors);
      if (CLUTTER_IS_ACTOR (item))
        {
          selected_actor = item;
          props_populate (selected_actor); 
        }
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
  property_editors = CLUTTER_ACTOR (clutter_script_get_object (script, "property-editors"));

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

