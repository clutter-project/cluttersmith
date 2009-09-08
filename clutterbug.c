#include <nbtk/nbtk.h>
#include <clutter/clutter.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include "util.h"
#include "hrn-popup.h"

#define PKGDATADIR "/home/pippin/src/clutterbug/"

static ClutterColor  white = {0xff,0xff,0xff,0xff};
static ClutterColor  yellow = {0xff,0xff,0x44,0xff};
void init_types (void);

gchar *whitelist[]={"x","y", "depth", "opacity", "width", "height",
                    "scale-x","scale-y", "anchor-x", "color",
                    "anchor-y", "rotation-angle-z",
                    "name", 
                    NULL};

gchar *subtree_to_string (ClutterActor *root);
static void select_item (ClutterActor *button, ClutterActor *item);

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


static gboolean idle_add_stage (gpointer stage)
{
  ClutterActor *actor;
 

  actor = util_load_json (PKGDATADIR "parasite.json");
  g_object_set_data (G_OBJECT (actor), "clutter-bug", (void*)TRUE);
  clutter_container_add_actor (CLUTTER_CONTAINER (stage), actor);
  g_timeout_add (400, keep_on_top, actor);

  g_signal_connect_after (stage, "paint", G_CALLBACK (cb_overlay_paint), NULL);
  nbtk_style_load_from_file (nbtk_style_get_default (), PKGDATADIR "clutterbug.css", NULL);

  init_types ();
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
static ClutterActor  *name, *parents, *property_editors, *scene_graph;
ClutterActor *parasite_root;


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

#define INDENTATION_AMOUNT  20


static void select_item_event (ClutterActor *button, ClutterEvent *event, ClutterActor *item)
{
  select_item (NULL, item);
}

static void
tree_populate_iter (ClutterActor *iter,
                    gfloat *x,
                    gfloat *y)
{
  ClutterActor *label;

  if (iter == NULL ||
      util_has_ancestor (iter, parasite_root))
    {
      return;
    }

  label = clutter_text_new_with_text ("Sans 12px", G_OBJECT_TYPE_NAME (iter));
  if (iter == selected_actor)
    {
      clutter_text_set_color (CLUTTER_TEXT (label), &yellow);
    }
  else
    {
      clutter_text_set_color (CLUTTER_TEXT (label), &white);
    }
  g_signal_connect (label, "button-press-event", G_CALLBACK (select_item_event), iter);
  clutter_actor_set_reactive (label, TRUE);

  clutter_container_add_actor (CLUTTER_CONTAINER (scene_graph), label);
  clutter_actor_set_position (label, *x, *y);

  *y += clutter_actor_get_height (label);

  if (CLUTTER_IS_CONTAINER (iter))
    {
      GList *children, *c;
      children = clutter_container_get_children (CLUTTER_CONTAINER (iter));
      *x += INDENTATION_AMOUNT;
      for (c = children; c; c=c->next)
        {
          tree_populate_iter (c->data, x, y);
        }
      *x -= INDENTATION_AMOUNT;
      g_list_free (children);
    }
}

static void
tree_populate (ClutterActor *actor)
{
  gfloat x=0;
  gfloat y=0;
  util_remove_children (scene_graph);

  tree_populate_iter (clutter_actor_get_stage (actor), &x, &y);
}


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
                  gint k;
                  skip = TRUE;
                  for (k=0;whitelist[k];k++)
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

static void select_item (ClutterActor *button, ClutterActor *item);
static void selected_vanished (gpointer data,
                               GObject *where_the_object_was)
{
  g_warning ("SELECTED vanished\n");
  selected_actor = NULL;
  select_item (NULL, NULL);
}

static void select_item (ClutterActor *button, ClutterActor *item)
{
  if (item)
    clutter_text_set_text (CLUTTER_TEXT (name), G_OBJECT_TYPE_NAME (item));

  util_remove_children (parents);
  util_remove_children (property_editors);
  util_remove_children (scene_graph);

    {
      if (selected_actor)
        {
          g_object_weak_unref (G_OBJECT (selected_actor), selected_vanished, NULL);
          selected_actor = NULL;
        }

      selected_actor = item;
      if (CLUTTER_IS_ACTOR (item))
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


          g_object_weak_ref (G_OBJECT (item), selected_vanished, NULL);

          props_populate (selected_actor); 
          tree_populate (selected_actor);
        }
    }
}



static gboolean
pick_all_capture (ClutterActor *actor, ClutterEvent *event, gpointer data)
{
  switch (event->any.type)
    {
      case CLUTTER_MOTION:
        {
          ClutterActor *hit;

          hit = clutter_stage_get_actor_at_pos (CLUTTER_STAGE (clutter_actor_get_stage (actor)),
                                                CLUTTER_PICK_ALL,
                                                event->motion.x, event->motion.y);
          if (!util_has_ancestor (hit, parasite_root) || 1)
            select_item (NULL, hit);
          else
            g_print ("child of foo!\n");
        }
        break;
      case CLUTTER_BUTTON_PRESS:
        g_signal_handler_disconnect (clutter_actor_get_stage (actor),
                                     stage_capture_handler);
        stage_capture_handler = 0;
        break;
      case CLUTTER_BUTTON_RELEASE:
      case CLUTTER_ENTER:
      case CLUTTER_LEAVE:
        return FALSE;
    }
  return TRUE;
}

static gboolean
pick_reactive_capture (ClutterActor *actor, ClutterEvent *event, gpointer data)
{
  switch (event->any.type)
    {
      case CLUTTER_MOTION:
        {
          ClutterActor *hit;

          hit = clutter_stage_get_actor_at_pos (CLUTTER_STAGE (clutter_actor_get_stage (actor)),
                                                CLUTTER_PICK_REACTIVE,
                                                event->motion.x, event->motion.y);
          if (!util_has_ancestor (hit, parasite_root))
            select_item (NULL, hit);
          else
            g_print ("child of foo!\n");
        }
        break;
      case CLUTTER_BUTTON_PRESS:
        g_signal_handler_disconnect (clutter_actor_get_stage (actor),
                                     stage_capture_handler);
        stage_capture_handler = 0;
        break;
      case CLUTTER_BUTTON_RELEASE:
      case CLUTTER_ENTER:
      case CLUTTER_LEAVE:
        return FALSE;
    }
  return TRUE;
}



static guint manipulate_capture_handler = 0;
static gint manipulate_x;
static gint manipulate_y;

static gboolean
manipulate_move_capture (ClutterActor *stage, ClutterEvent *event, gpointer data)
{
  switch (event->any.type)
    {
      case CLUTTER_MOTION:
        {
          gfloat x, y;
          clutter_actor_get_position (data, &x, &y);
          x-= manipulate_x-event->motion.x;
          y-= manipulate_y-event->motion.y;
          clutter_actor_set_position (data, x, y);
          g_print ("%f %f\n", x, y);

          manipulate_x=event->motion.x;
          manipulate_y=event->motion.y;
        }
        break;
      case CLUTTER_BUTTON_RELEASE:
        g_signal_handler_disconnect (stage,
                                     manipulate_capture_handler);
        manipulate_capture_handler = 0;
        break;
    }
  return TRUE;
}


static gboolean
manipulate_resize_capture (ClutterActor *stage, ClutterEvent *event, gpointer data)
{
  switch (event->any.type)
    {
      case CLUTTER_MOTION:
        {
          gfloat x, y;
          clutter_actor_get_size (data, &x, &y);
          x-= manipulate_x-event->motion.x;
          y-= manipulate_y-event->motion.y;
          clutter_actor_set_size (data, x, y);
          g_print ("%f %f\n", x, y);

          manipulate_x=event->motion.x;
          manipulate_y=event->motion.y;
        }
        break;
      case CLUTTER_BUTTON_RELEASE:
        g_signal_handler_disconnect (stage,
                                     manipulate_capture_handler);
        manipulate_capture_handler = 0;
        break;
    }
  return TRUE;
}

static gboolean manipulate_move_press (ClutterActor  *actor,
                                  ClutterEvent  *event)
{
  manipulate_x = event->button.x;
  manipulate_y = event->button.y;

  manipulate_capture_handler = 
     g_signal_connect (clutter_actor_get_stage (actor), "captured-event",
                       G_CALLBACK (manipulate_move_capture), actor);
  clutter_actor_raise_top (actor);
  return TRUE;
}


static gboolean manipulate_resize_press (ClutterActor  *actor,
                                         ClutterEvent  *event)
{
  manipulate_x = event->button.x;
  manipulate_y = event->button.y;

  manipulate_capture_handler = 
     g_signal_connect (clutter_actor_get_stage (actor), "captured-event",
                       G_CALLBACK (manipulate_resize_capture), actor);
  clutter_actor_raise_top (actor);
  return TRUE;
}

static gboolean
manipulate_capture (ClutterActor *actor, ClutterEvent *event, gpointer data)
{
  if (util_has_ancestor (event->any.source, parasite_root))
    {
      return FALSE;
    }
  if (!selected_actor)
    return TRUE;
  switch (event->any.type)
    {
      case CLUTTER_MOTION:
        {
#if 0
          ClutterActor *hit;

          hit = clutter_stage_get_actor_at_pos (CLUTTER_STAGE (clutter_actor_get_stage (actor)),
                                                CLUTTER_PICK_ALL,
                                                event->motion.x, event->motion.y);
          if (!util_has_ancestor (hit, parasite_root))
            select_item (NULL, hit);
          else
            g_print ("child of foo!\n");
#endif
        }
        break;
      case CLUTTER_BUTTON_PRESS:
        {
          gfloat x = event->button.x;
          gfloat y = event->button.y;
          gfloat w,h;
          clutter_actor_get_size (selected_actor, &w, &h);
          clutter_actor_transform_stage_point (selected_actor, x, y, &x, &y);
          x/=w;
          y/=h;
          if (x>0.5 && y>0.5)
            {
              manipulate_resize_press (selected_actor, event);
            }
          else
            {
              manipulate_move_press (selected_actor, event);
            }

        }
        break;
      case CLUTTER_BUTTON_RELEASE:
      case CLUTTER_ENTER:
      case CLUTTER_LEAVE:
        return FALSE;
    }
  return TRUE;
}

void cb_manipulate (ClutterActor *actor)
{
  ClutterScript *script = util_get_script (actor);

  name = CLUTTER_ACTOR (clutter_script_get_object (script, "name"));
  parents = CLUTTER_ACTOR (clutter_script_get_object (script, "parents"));
  scene_graph = CLUTTER_ACTOR (clutter_script_get_object (script, "scene-graph"));
  parasite_root = CLUTTER_ACTOR (clutter_script_get_object (script, "actor"));
  property_editors = CLUTTER_ACTOR (clutter_script_get_object (script, "property-editors"));

  if (stage_capture_handler)
    {
        g_signal_handler_disconnect (clutter_actor_get_stage (actor),
                                     stage_capture_handler);
    }

  stage_capture_handler = 
     g_signal_connect_after (clutter_actor_get_stage (actor), "captured-event",
                             G_CALLBACK (manipulate_capture), NULL);
}


void cb_select_all (ClutterActor *actor)
{
  ClutterScript *script = util_get_script (actor);

  name = CLUTTER_ACTOR (clutter_script_get_object (script, "name"));
  parents = CLUTTER_ACTOR (clutter_script_get_object (script, "parents"));
  scene_graph = CLUTTER_ACTOR (clutter_script_get_object (script, "scene-graph"));
  parasite_root = CLUTTER_ACTOR (clutter_script_get_object (script, "actor"));
  property_editors = CLUTTER_ACTOR (clutter_script_get_object (script, "property-editors"));

  stage_capture_handler = 
     g_signal_connect (clutter_actor_get_stage (actor), "captured-event",
                       G_CALLBACK (pick_all_capture), NULL);
}


void cb_select_reactive (ClutterActor *actor)
{
  ClutterScript *script = util_get_script (actor);

  name = CLUTTER_ACTOR (clutter_script_get_object (script, "name"));
  parents = CLUTTER_ACTOR (clutter_script_get_object (script, "parents"));
  property_editors = CLUTTER_ACTOR (clutter_script_get_object (script, "property-editors"));
  scene_graph = CLUTTER_ACTOR (clutter_script_get_object (script, "scene-graph"));
  parasite_root = CLUTTER_ACTOR (clutter_script_get_object (script, "actor"));

  stage_capture_handler = 
     g_signal_connect (clutter_actor_get_stage (actor), "captured-event",
                       G_CALLBACK (pick_reactive_capture), NULL);

}

void cb_collapse_panel (ClutterActor *actor)
{
  ClutterScript *script = util_get_script (actor);
  ClutterActor  *parasite = CLUTTER_ACTOR (clutter_script_get_object (script, "prop-editor"));
  static gboolean collapsed = FALSE;

  parasite = clutter_actor_get_parent (clutter_actor_get_parent (actor));
  collapsed = !collapsed;

  if (collapsed)
    clutter_actor_animate (parasite, CLUTTER_LINEAR, 200, "height", 22.0, NULL);
  else
    clutter_actor_animate (parasite, CLUTTER_LINEAR, 200, "height", 400.0, NULL);
}


void cb_remove_selected (ClutterActor *actor)
{
  if (selected_actor)
    {
      clutter_actor_destroy (selected_actor);
      selected_actor = NULL;
    }
}

void cb_raise_selected (ClutterActor *actor)
{
  if (selected_actor)
    {
      clutter_actor_raise (selected_actor, NULL);
    }
}

void cb_lower_selected (ClutterActor *actor)
{
  if (selected_actor)
    {
      clutter_actor_lower (selected_actor, NULL);
    }
}


void cb_add (ClutterActor *actor)
{
  ClutterActor *container = selected_actor;
  ClutterActor *new;

  if (selected_actor)
    {
      while (!CLUTTER_IS_CONTAINER (container))
       {
         container = clutter_actor_get_parent (container);
       }
    }
  else
    {
      container = clutter_actor_get_stage (actor);
    }

  new = clutter_rectangle_new ();
  clutter_actor_set_size (new, 100, 100);
  clutter_container_add_actor (CLUTTER_CONTAINER (container), new);
  select_item (NULL, new);
}

void entry_text_changed (ClutterActor *actor)
{
  const gchar *title = clutter_text_get_text (CLUTTER_TEXT (actor));
  gchar *filename;
  filename = g_strdup_printf ("json/%s.json", title);
  if (g_file_test (filename, G_FILE_TEST_IS_REGULAR))
    {
      util_replace_content2 (actor, "content", filename);
    }
}

void search_entry_init_hack (ClutterActor  *actor)
{
  /* we hook this up to the first paint, since no other signal seems to
   * be available to hook up for some additional initialization
   */
  static gboolean done = FALSE; 
  if (done)
    return;
  done = TRUE;

  g_signal_connect (nbtk_entry_get_clutter_text (NBTK_ENTRY (actor)), "text-changed",
                    G_CALLBACK (entry_text_changed), NULL);
}

static GList *
actor_types_build (GList *list, GType type)
{
  GType *ops;
  guint  children;
  gint   no;

  if (!type)
    return list;

  list = g_list_prepend (list, (void*)g_type_name (type));

  ops = g_type_children (type, &children);

  for (no=0; no<children; no++)
    {
      list = actor_types_build (list, ops[no]);
    }
  if (ops)
    g_free (ops);
  return list;
}

static gint compare_type_names (gconstpointer a,
                                gconstpointer b)
{
  return strcmp (a, b);
}

typedef struct TransientValue {
  gchar  *name;
  GValue  value;
  GType   value_type;
} TransientValue;

static GList *transient_values = NULL;

static void
build_transient (ClutterActor *actor)
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
                  gint k;
                  skip = TRUE;
                  for (k=0;whitelist[k];k++)
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
          TransientValue *value = g_new0 (TransientValue, 1);
          value->name = properties[i]->name;
          value->value_type = properties[i]->value_type;
          g_value_init (&value->value, properties[i]->value_type);
          g_object_get_property (G_OBJECT (actor), properties[i]->name, &value->value);

          transient_values = g_list_prepend (transient_values, value);
        }
    }

  g_free (properties);
}


static void
apply_transient (ClutterActor *actor)
{
  GParamSpec **properties;
  GParamSpec **actor_properties;
  guint        n_properties;
  guint        n_actor_properties;
  gint         i;

  properties = g_object_class_list_properties (
                     G_OBJECT_GET_CLASS (actor),
                     &n_properties);

  for (i = 0; i < n_properties; i++)
    {
      gint j;
      gboolean skip = FALSE;
      GList *val;

      if (!(properties[i]->flags & G_PARAM_WRITABLE))
        skip = TRUE;

      if (skip)
        continue;

      for (val=transient_values;val;val=val->next)
        {
          TransientValue *value = val->data;
          if (g_str_equal (value->name, properties[i]->name) &&
                          value->value_type == properties[i]->value_type)
            {
              g_object_set_property (G_OBJECT (actor), properties[i]->name, &value->value);
              continue;
            }
        }
    }

  g_free (properties);
  transient_values = NULL;
  /* XXX: free list of transient values */
}


static void change_type (ClutterActor *actor,
                         const gchar  *new_type)
{
  ClutterActor *new_actor, *parent;
  gfloat x, y, width, height, depth;

  hrn_popup_close ();

  new_actor = g_object_new (g_type_from_name (new_type), NULL);
  parent = clutter_actor_get_parent (selected_actor);

    build_transient (selected_actor);

    if (CLUTTER_IS_CONTAINER (selected_actor))
      {
        GList *c, *children;
        children = clutter_container_get_children (CLUTTER_CONTAINER (selected_actor));
        for (c = children; c; c = c->next)
          {
            ClutterActor *child = g_object_ref (c->data);
            clutter_container_remove_actor (CLUTTER_CONTAINER (selected_actor), child);
            clutter_container_add_actor (CLUTTER_CONTAINER (new_actor), child);

          }
        g_list_free (children);
      }

    apply_transient (new_actor);
    clutter_container_remove_actor (CLUTTER_CONTAINER (parent), selected_actor);
    clutter_container_add_actor (CLUTTER_CONTAINER (parent), new_actor);
  select_item (NULL, new_actor);
  select_item (NULL, new_actor);
}

void printname (gchar *name, ClutterActor *container)
{
  ClutterActor *button = CLUTTER_ACTOR (nbtk_button_new_with_label (name));
  clutter_container_add_actor (CLUTTER_CONTAINER (container), button);
  clutter_actor_set_width (button, 200);
  g_signal_connect (button, "clicked", G_CALLBACK (change_type), name);
}

void cb_change_type (ClutterActor *actor)
{
  static GList *types = NULL;

  if (!selected_actor)
    return;
 
  if (!types)
    {
       types = actor_types_build (NULL, CLUTTER_TYPE_ACTOR);
       types = g_list_sort (types, (void*)strcmp);
    }
  actor = CLUTTER_ACTOR (nbtk_grid_new ());
  g_object_set (actor, "height", 600.0, "column-major", TRUE, "homogenous-columns", TRUE, NULL);
  g_list_foreach (types, (void*)printname, actor);
  hrn_popup_actor_fixed (clutter_stage_get_default(), 0,0, actor);
}

