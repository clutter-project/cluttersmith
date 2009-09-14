#include <nbtk/nbtk.h>
#include <clutter/clutter.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include "util.h"
#include "hrn-popup.h"

/*#define EDIT_SELF*/

static gboolean remove_from_selection = FALSE; /* is this used? */
#define PKGDATADIR "./" //"/home/pippin/src/clutterbug/"

static ClutterColor  white = {0xff,0xff,0xff,0xff};
static ClutterColor  yellow = {0xff,0xff,0x44,0xff};
void init_types (void);

guint CB_REV       = 0; /* everything that changes state and could be determined
                         * a user change should increment this.
                         */
guint CB_SAVED_REV = 0;

ClutterActor      *lasso;
static gint        lx, ly;
static GHashTable *selection = NULL; /* what would be added/removed by
                                        current lasso*/
static GHashTable *selected  = NULL;


static void init_multi_select (void)
{
  selection = g_hash_table_new_full (g_direct_hash, g_direct_equal, NULL, NULL);
  selected = g_hash_table_new_full (g_direct_hash, g_direct_equal, NULL, NULL);

}


static ClutterActor  *title, *name, *parents,
                     *property_editors, *scene_graph;
ClutterActor *parasite_root, *parasite_ui;

gchar *whitelist[]={"depth", "opacity",
                    "scale-x","scale-y", "anchor-x", "color",
                    "anchor-y", "rotation-angle-z",
                    "name", "reactive",
                    NULL};

void load_file (ClutterActor *actor, const gchar *title);

gchar *blacklist_types[]={"ClutterStage",
                          "ClutterCairoTexture",
                          "ClutterStageGLX",
                          "ClutterStageX11",
                          "ClutterActor",
                          "NbtkWidget",
                          NULL};

gchar *subtree_to_string (ClutterActor *root);
static void select_item (ClutterActor *button, ClutterActor *item);

static gboolean keep_on_top (gpointer actor)
{
  clutter_actor_raise_top (actor);
  return FALSE;
}

static ClutterActor *selected_actor = NULL;


/* snap positions, in relation to actor */
static gint ver_pos = 0; /* 1 = start 2 = mid 3 = end */
static gint hor_pos = 0; /* 1 = start 2 = mid 3 = end */

static void
cb_overlay_paint (ClutterActor *actor,
                  gpointer      user_data)
{
  ClutterVertex verts[4];

  if (!selected_actor && g_hash_table_size (selected)==0)
    return;

  if (!CLUTTER_IS_STAGE (selected_actor))
    {
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

            {
              gint i;
              for (i=-1;i<3;i++)
                {
                  gfloat coords[]={ 
                 (verts[1].x+verts[3].x)/2+i, (verts[1].y+verts[3].y)/2+i, 
                 verts[3].x+1, verts[3].y+i, 
                 (verts[2].x+verts[3].x)/2+i, (verts[2].y+verts[3].y)/2+i};
                 cogl_path_polyline (coords, 3);
                 cogl_set_source_color4ub (255, 0, 0, 128);
                 cogl_path_stroke ();
            }
            }
        }

        cogl_set_source_color4ub (128, 128, 255, 255);
        switch (hor_pos)
          {
            case 1:
             {
                gfloat coords[]={ verts[2].x, -2000,
                                  verts[2].x, 2000 };
                cogl_path_polyline (coords, 2);
                cogl_path_stroke ();
             }
             break;
            case 2:
             {
                gfloat coords[]={ (verts[2].x+verts[1].x)/2, -2000,
                                  (verts[2].x+verts[1].x)/2, 2000 };
                cogl_path_polyline (coords, 2);
                cogl_path_stroke ();
             }
             break;
            case 3:
             {
                gfloat coords[]={ verts[1].x, -2000,
                                  verts[1].x, 2000 };
                cogl_path_polyline (coords, 2);
                cogl_path_stroke ();
             }
          }
        switch (ver_pos)
          {
            case 1:
             {
                gfloat coords[]={ -2000, verts[1].y,
                                   2000, verts[1].y};
                cogl_path_polyline (coords, 2);
                cogl_path_stroke ();
             }
             break;
            case 2:
             {
                gfloat coords[]={ -2000, (verts[2].y+verts[1].y)/2,
                                   2000, (verts[2].y+verts[1].y)/2};
                cogl_path_polyline (coords, 2);
                cogl_path_stroke ();
             }
             break;
            case 3:
             {
                gfloat coords[]={ -2000, verts[2].y,
                                   2000, verts[2].y};
                cogl_path_polyline (coords, 2);
                cogl_path_stroke ();
             }
          }
      }
    }

  {
  /* draw selected */
    GHashTableIter      iter;
    gpointer            key, value;

    g_hash_table_iter_init (&iter, selected);
    while (g_hash_table_iter_next (&iter, &key, &value))
      {
        clutter_actor_get_abs_allocation_vertices (key,
                                                   verts);

        cogl_set_source_color4ub (0, 25, 0, 50);

        {
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

    g_hash_table_iter_init (&iter, selection);
    while (g_hash_table_iter_next (&iter, &key, &value))
      {
        clutter_actor_get_abs_allocation_vertices (key,
                                                   verts);

        cogl_set_source_color4ub (0, 0, 25, 50);

        {
          {
            gfloat coords[]={ verts[0].x, verts[0].y, 
               verts[1].x, verts[1].y, 
               verts[3].x, verts[3].y, 
               verts[2].x, verts[2].y, 
               verts[0].x, verts[0].y };

            cogl_path_polyline (coords, 5);
            cogl_set_source_color4ub (0, 0, 255, 128);
            cogl_path_stroke ();
          }
        }
      }
}

}

void cb_manipulate_init (ClutterActor *actor);


void stage_size_changed (ClutterActor *stage, gpointer ignored, ClutterActor *bin)
{
  gfloat width, height;
  clutter_actor_get_size (stage, &width, &height);
  clutter_actor_set_size (bin, width, height);
}



gboolean idle_add_stage (gpointer stage)
{
  ClutterActor *actor;
  ClutterScript *script;

  actor = util_load_json (PKGDATADIR "parasite.json");
  g_object_set_data (G_OBJECT (actor), "clutter-bug", (void*)TRUE);
  clutter_container_add_actor (CLUTTER_CONTAINER (stage), actor);
  g_timeout_add (4000, keep_on_top, actor);

  g_signal_connect_after (stage, "paint", G_CALLBACK (cb_overlay_paint), NULL);
  nbtk_style_load_from_file (nbtk_style_get_default (), PKGDATADIR "clutterbug.css", NULL);
  script = util_get_script (actor);

  /* initializing globals */
  title = CLUTTER_ACTOR (clutter_script_get_object (script, "title"));
  name = CLUTTER_ACTOR (clutter_script_get_object (script, "name"));
  parents = CLUTTER_ACTOR (clutter_script_get_object (script, "parents"));
  scene_graph = CLUTTER_ACTOR (clutter_script_get_object (script, "scene-graph"));
  parasite_ui = CLUTTER_ACTOR (clutter_script_get_object (script, "parasite-ui"));
  property_editors = CLUTTER_ACTOR (clutter_script_get_object (script, "property-editors"));
  parasite_root = actor;

  g_signal_connect (stage, "notify::width", G_CALLBACK (stage_size_changed), actor);
  g_signal_connect (stage, "notify::height", G_CALLBACK (stage_size_changed), actor);
  /* do an initial syncing */
  stage_size_changed (stage, NULL, actor);

  cb_manipulate_init (parasite_root);
  init_multi_select ();
  select_item (NULL, stage);


  init_types ();
  return FALSE;
}

#ifdef CLUTTERBUG
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
#endif

static guint stage_capture_handler = 0;

ClutterActor *property_editor_new (GObject *object,
                                   const gchar *property_name);

gboolean cb_filter_properties = TRUE;

#define INDENTATION_AMOUNT  20


static gboolean select_item_event (ClutterActor *button, ClutterEvent *event, ClutterActor *item)
{
  select_item (NULL, item);
  return TRUE;
}


static ClutterActor *clone = NULL;
static ClutterActor *dragged_item = NULL;
static ClutterActor *dropped_on_target = NULL;
static glong  tree_item_capture_handler = 0;
static gfloat tree_item_x;
static gfloat tree_item_y;

static ClutterActor *get_drop_target (ClutterActor *actor)
{
  if (!actor)
    return NULL;
  actor = g_object_get_data (G_OBJECT (actor), "actor");
  if (util_has_ancestor (actor, dragged_item))
    return NULL;
  if (CLUTTER_IS_CONTAINER (actor))
    return actor;
  return NULL;
}

static ClutterActor *get_nth_child (ClutterContainer *container, gint n)
{
  GList *list = clutter_container_get_children (container);
  ClutterActor *ret = g_list_nth_data (list, n);
  g_list_free (list);
  return ret;
}

static gboolean
tree_item_capture (ClutterActor *stage, ClutterEvent *event, gpointer data)
{
  switch (event->any.type)
    {
      case CLUTTER_MOTION:
        {
          clutter_actor_set_position (clone, event->motion.x, event->motion.y);
        }
        break;
      case CLUTTER_ENTER:
          {
            dropped_on_target = get_drop_target (event->any.source);
          }
        break;
      case CLUTTER_BUTTON_RELEASE:
        clutter_actor_destroy (clone);
        clone = NULL;

        if (dropped_on_target)
          {
            ClutterActor *dragged_actor = g_object_get_data (data, "actor");

            /* Determine where to add the actor, 0 is before existing
             * children 1 is after first child
             */


            if (dragged_actor &&
                !util_has_ancestor (dropped_on_target, dragged_actor) &&
                (dropped_on_target != dragged_actor))
              {
                gfloat x, y;

                clutter_actor_transform_stage_point (event->any.source, event->button.x, event->button.y, &x, &y);

                dropped_on_target = get_drop_target (event->any.source);


                if (dropped_on_target)
                  {
                    gint   add_self = 0;
                    gint   use_child = 0;
                    gint   child_no = 0;
                    ClutterActor *real_container = NULL;
                    GList *e, *existing_children;

                    g_object_ref (dragged_actor);
                    clutter_container_remove_actor (CLUTTER_CONTAINER (clutter_actor_get_parent (dragged_actor)), dragged_actor);
                    g_print ("[%p]\n", clutter_actor_get_parent (dragged_actor));

                    real_container = g_object_get_data (G_OBJECT(event->any.source),"actor");

                    existing_children = clutter_container_get_children (
                        CLUTTER_CONTAINER (get_nth_child (CLUTTER_CONTAINER(event->any.source),1)));
                    for (e=existing_children; e; e = e->next, child_no++)
                      {
                        gfloat child_y2;
                        clutter_actor_transform_stage_point (e->data, 0, event->button.y, NULL, &child_y2);
                        if (use_child == 0 && child_no>=0 &&
                            g_object_get_data (e->data, "actor") == dragged_actor)
                          add_self = 1;
                        if (child_y2 > 0)
                          {
                            use_child = child_no + 1;
                          }
                      }

                    g_list_free (existing_children);

                    existing_children = clutter_container_get_children (CLUTTER_CONTAINER (real_container));

                    for (e=existing_children; e; e = e->next)
                      {
                        g_object_ref (e->data);
                        clutter_container_remove_actor (CLUTTER_CONTAINER (real_container), e->data);
                      }

                    child_no = 0;
                    for (e=existing_children; e; e = e->next, child_no++)
                      {

                        if (use_child == 0 && child_no == 0)
                          {
                            g_print ("droppedA in %i %i %p\n", child_no, add_self, dragged_actor);
                            clutter_container_add_actor (CLUTTER_CONTAINER (real_container), dragged_actor);
                            use_child = -1;
                          }

                        g_assert (e->data != dragged_actor);
                        clutter_container_add_actor (CLUTTER_CONTAINER (real_container), e->data);
                        g_object_unref (e->data);

                        if (child_no == use_child-1 - add_self || (e->next == NULL && use_child != -1))
                          {
                            g_print ("droppedB in %i %i %p\n", child_no, add_self, dragged_actor);
                            clutter_container_add_actor (CLUTTER_CONTAINER (real_container), dragged_actor);
                            use_child = -1;
                          }

                      }

                    g_list_free (existing_children);
                  }
                g_object_unref (dragged_actor);
              }
            if (dragged_actor)
              select_item (NULL, dragged_actor);
          }
        dropped_on_target = NULL;
        dragged_item = NULL;
        g_signal_handler_disconnect (stage,
                                     tree_item_capture_handler);
        tree_item_capture_handler = 0;
      default:
        break;
    }
  return TRUE;
}


static gboolean tree_item_press (ClutterActor  *actor,
                                 ClutterEvent  *event)
{
  tree_item_x = event->button.x;
  tree_item_y = event->button.y;

  if (CLUTTER_IS_STAGE (g_object_get_data (G_OBJECT (actor), "actor")))
    return TRUE;

  tree_item_capture_handler = 
     g_signal_connect (clutter_actor_get_stage (actor), "captured-event",
                       G_CALLBACK (tree_item_capture), actor);

  dragged_item = actor;
  clone = clutter_clone_new (actor);
  clutter_actor_set_opacity (clone, 0xbb);
  clutter_actor_set_position (clone, event->button.x, event->button.y);
  clutter_container_add_actor (CLUTTER_CONTAINER (clutter_actor_get_stage(actor)), clone);

  return TRUE;
}

static void
tree_populate_iter (ClutterActor *current_container,
                    ClutterActor *iter,
                    gint   *count,
                    gint   *level)
{
  ClutterActor *vbox;
  ClutterActor *label;

  if (iter == NULL ||
#ifdef EDIT_SELF
      util_has_ancestor (iter, scene_graph)
#else
      util_has_ancestor (iter, parasite_root)
#endif
      )
    {
      return;
    }

  vbox = g_object_new (NBTK_TYPE_BOX_LAYOUT, "min-width", 100.0, "natural-width", 500.0, "vertical", TRUE,
                       "style-class", 
                
                       ((*count)%2==0)?
                          ((*level)%2==0)?"ParasiteTreeA1":"ParasiteTreeB1":
                          ((*level)%2==0)?"ParasiteTreeA2":"ParasiteTreeB2",
                      
                       NULL);

                       //((*level)%2==0)?"ParasiteTreeA":"ParasiteTreeB",

  label = clutter_text_new_with_text ("Sans 14px", G_OBJECT_TYPE_NAME (iter));
  clutter_actor_set_anchor_point (label, -24.0, 0.0);
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

  clutter_container_add_actor (CLUTTER_CONTAINER (vbox), label);

  clutter_container_add_actor (CLUTTER_CONTAINER (current_container), vbox);
  g_object_set_data (G_OBJECT (vbox), "actor", iter);

  if (CLUTTER_IS_CONTAINER (iter))
    {
      ClutterActor *child_vbox;
      GList *children, *c;
      children = clutter_container_get_children (CLUTTER_CONTAINER (iter));

      child_vbox = g_object_new (NBTK_TYPE_BOX_LAYOUT, "vertical", TRUE, NULL);
      clutter_container_add_actor (CLUTTER_CONTAINER (vbox), child_vbox);
      clutter_actor_set_anchor_point (child_vbox, -24.0, 0.0);

      /*g_signal_connect (vbox, "button-press-event", G_CALLBACK (vbox_press), child_vbox);*/

      (*level) = (*level)+1;
      for (c = children; c; c=c->next)
        {
          tree_populate_iter (child_vbox, c->data, level, count);
        }
      (*level) = (*level)-1;
      g_list_free (children);
    }
  else
    {
      /*g_signal_connect (vbox, "button-press-event", G_CALLBACK (vbox_nop), NULL);*/
    }

  g_signal_connect (vbox, "button-press-event", G_CALLBACK (tree_item_press), NULL);
  clutter_actor_set_reactive (vbox, TRUE);
  (*count) = (*count)+1;
}

static void
tree_populate (ClutterActor *actor)
{
  gint level = 0;
  gint count = 0;
  util_remove_children (scene_graph);

  clutter_actor_set_width (scene_graph, 230);
  tree_populate_iter (scene_graph, clutter_actor_get_stage (actor), &level, &count);
}

static gboolean
update_id (ClutterText *text,
           gpointer     data)
{
  clutter_scriptable_set_id (CLUTTER_SCRIPTABLE (data), clutter_text_get_text (text));
  return TRUE;
}

#define EDITOR_LINE_HEIGHT 24

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
            G_OBJECT_CLASS (g_type_class_ref (CLUTTER_TYPE_ACTOR)),
            &n_actor_properties);


  {
    ClutterActor *hbox = g_object_new (NBTK_TYPE_BOX_LAYOUT, NULL);
    ClutterActor *label;
    ClutterActor *editor; 

    /* special casing of x,y,w,h to make it take up less space and always be first */

    label = clutter_text_new_with_text ("Sans 12px", "pos:");
    clutter_text_set_color (CLUTTER_TEXT (label), &white);
    clutter_actor_set_size (label, 25, EDITOR_LINE_HEIGHT);
    clutter_container_add_actor (CLUTTER_CONTAINER (hbox), label);

    editor = property_editor_new (G_OBJECT (actor), "x");
    clutter_actor_set_size (editor, 50, EDITOR_LINE_HEIGHT);
    clutter_container_add_actor (CLUTTER_CONTAINER (hbox), editor);

    editor = property_editor_new (G_OBJECT (actor), "y");
    clutter_actor_set_size (editor, 50, EDITOR_LINE_HEIGHT);
    clutter_container_add_actor (CLUTTER_CONTAINER (hbox), editor);

    label = clutter_text_new_with_text ("Sans 12px", "size:");
    clutter_text_set_color (CLUTTER_TEXT (label), &white);
    clutter_actor_set_size (label, 25, EDITOR_LINE_HEIGHT);
    clutter_container_add_actor (CLUTTER_CONTAINER (hbox), label);

    editor = property_editor_new (G_OBJECT (actor), "width");
    clutter_actor_set_size (editor, 50, EDITOR_LINE_HEIGHT);
    clutter_container_add_actor (CLUTTER_CONTAINER (hbox), editor);

    editor = property_editor_new (G_OBJECT (actor), "height");
    clutter_actor_set_size (editor, 50, EDITOR_LINE_HEIGHT);
    clutter_container_add_actor (CLUTTER_CONTAINER (hbox), editor);

    clutter_container_add_actor (CLUTTER_CONTAINER (property_editors), hbox);

    /* virtual 'id' property */

    hbox = g_object_new (NBTK_TYPE_BOX_LAYOUT, NULL);
    label = clutter_text_new_with_text ("Sans 12px", "id");

    {
      editor = CLUTTER_ACTOR (nbtk_entry_new (""));
      g_signal_connect (nbtk_entry_get_clutter_text (
                             NBTK_ENTRY (editor)), "text-changed",
                             G_CALLBACK (update_id), actor);
      nbtk_entry_set_text (NBTK_ENTRY (editor), clutter_scriptable_get_id (CLUTTER_SCRIPTABLE (actor)));
    }

    clutter_container_add_actor (CLUTTER_CONTAINER (hbox), label);
    clutter_container_add_actor (CLUTTER_CONTAINER (hbox), editor);
    clutter_text_set_color (CLUTTER_TEXT (label), &white);
    clutter_actor_set_size (label, 130, EDITOR_LINE_HEIGHT);
    clutter_actor_set_size (editor, 130, EDITOR_LINE_HEIGHT);
    clutter_container_add_actor (CLUTTER_CONTAINER (property_editors), hbox);
  }


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
        ClutterActor *hbox = g_object_new (NBTK_TYPE_BOX_LAYOUT, NULL);
        ClutterActor *label = clutter_text_new_with_text ("Sans 12px", properties[i]->name);
        ClutterActor *editor = property_editor_new (G_OBJECT (actor), properties[i]->name);
        clutter_text_set_color (CLUTTER_TEXT (label), &white);
        clutter_actor_set_size (label, 130, EDITOR_LINE_HEIGHT);
        clutter_actor_set_size (editor, 130, EDITOR_LINE_HEIGHT);
        clutter_container_add_actor (CLUTTER_CONTAINER (hbox), label);
        clutter_container_add_actor (CLUTTER_CONTAINER (hbox), editor);
        clutter_container_add_actor (CLUTTER_CONTAINER (property_editors), hbox);
      }
    }

  {
    ClutterActor *parent;
    parent = clutter_actor_get_parent (actor);

    if (parent && CLUTTER_IS_CONTAINER (parent))
      {
        ClutterChildMeta *child_meta;
        GParamSpec **child_properties = NULL;
        guint        n_child_properties=0;
        child_meta = clutter_container_get_child_meta (CLUTTER_CONTAINER (parent), actor);
        if (child_meta)
          {
            child_properties = g_object_class_list_properties (
                               G_OBJECT_GET_CLASS (child_meta),
                               &n_child_properties);
            for (i = 0; i < n_child_properties; i++)
              {
                if (!G_TYPE_IS_OBJECT (child_properties[i]->value_type) &&
                    child_properties[i]->value_type != CLUTTER_TYPE_CONTAINER)
                  {
                    ClutterActor *hbox = g_object_new (NBTK_TYPE_BOX_LAYOUT, NULL);
                    ClutterActor *label = clutter_text_new_with_text ("Sans 12px", child_properties[i]->name);
                    ClutterActor *editor = property_editor_new (G_OBJECT (child_meta), child_properties[i]->name);
                    clutter_text_set_color (CLUTTER_TEXT (label), &white);
                    clutter_actor_set_size (label, 130, EDITOR_LINE_HEIGHT);
                    clutter_actor_set_size (editor, 130, EDITOR_LINE_HEIGHT);
                    clutter_container_add_actor (CLUTTER_CONTAINER (hbox), label);
                    clutter_container_add_actor (CLUTTER_CONTAINER (hbox), editor);
                    clutter_container_add_actor (CLUTTER_CONTAINER (property_editors), hbox);
                  }
              }
            g_free (child_properties);
          }
      }
  }

  g_free (properties);
}

static void select_item (ClutterActor *button, ClutterActor *item);
static void selected_vanished (gpointer data,
                               GObject *where_the_object_was)
{
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
  if (selected_actor)
  clutter_stage_set_key_focus (CLUTTER_STAGE (clutter_actor_get_stage (selected_actor)), NULL);
}


static guint  manipulate_capture_handler = 0;
static gfloat manipulate_x;
static gfloat manipulate_y;

#define SNAP_THRESHOLD  2

static void snap_position (ClutterActor *actor,
                           gfloat        in_x,
                           gfloat        in_y,
                           gfloat       *out_x,
                           gfloat       *out_y)
{
  *out_x = in_x;
  *out_y = in_y;

  ClutterActor *parent;


  parent = clutter_actor_get_parent (actor);

  if (CLUTTER_IS_CONTAINER (parent))
    {
      gfloat in_width = clutter_actor_get_width (actor);
      gfloat in_height = clutter_actor_get_height (actor);
      gfloat in_mid_x = in_x + in_width/2;
      gfloat in_mid_y = in_y + in_height/2;
      gfloat in_end_x = in_x + in_width;
      gfloat in_end_y = in_y + in_height;


      gfloat best_x = 0;
      gfloat best_x_diff = 4096;
      gfloat best_y = 0;
      gfloat best_y_diff = 4096;
      GList *children, *c;
      children = clutter_container_get_children (CLUTTER_CONTAINER (parent));

      hor_pos = 0;
      ver_pos = 0;

      /* We only search our siblings for snapping...
       * perhaps we should search more.
       */

      for (c=children; c; c = c->next)
        {
          gfloat this_x = clutter_actor_get_x (c->data);
          gfloat this_width = clutter_actor_get_width (c->data);
          gfloat this_height = clutter_actor_get_height (c->data);
          gfloat this_end_x = this_x + this_width;
          gfloat this_mid_x = this_x + this_width/2;
          gfloat this_y = clutter_actor_get_y (c->data);
          gfloat this_end_y = this_y + this_height;
          gfloat this_mid_y = this_y + this_height/2;

          /* skip self */
          if (c->data == actor)
            continue;

/* starts aligned
                    this_x     this_mid_x     this_end_x
                    in_x    in_mid_x    in_end_x
*/
          if (abs (this_x - in_x) < best_x_diff)
            {
              best_x_diff = abs (this_x - in_x);
              best_x = this_x;
              hor_pos=1;
            }
          if (abs (this_y - in_y) < best_y_diff)
            {
              best_y_diff = abs (this_y - in_y);
              best_y = this_y;
              ver_pos=1;
            }
/*
 end aligned with start
                    this_x     this_mid_x     this_end_x
in_x    in_mid_x    in_end_x
*/
          if (abs (this_x - in_end_x) < best_x_diff)
            {
              best_x_diff = abs (this_x - in_end_x);
              best_x = this_x - in_width;
              hor_pos=3;
            }
          if (abs (this_y - in_end_y) < best_y_diff)
            {
              best_y_diff = abs (this_y - in_end_y);
              best_y = this_y - in_height;
              ver_pos=3;
            }
/*
 ends aligned
                    this_x     this_mid_x     this_end_x
                          in_x    in_mid_x    in_end_x
*/
          if (abs (this_end_x - in_end_x) < best_x_diff)
            {
              best_x_diff = abs (this_end_x - in_end_x);
              best_x = this_end_x - in_width;
              hor_pos=3;
            }
          if (abs (this_end_y - in_end_y) < best_y_diff)
            {
              best_y_diff = abs (this_end_y - in_end_y);
              best_y = this_end_y - in_height;
              ver_pos=3;
            }
/*
 end aligned with start
                    this_x     this_mid_x     this_end_x
                                              in_x    in_mid_x    in_end_x
                    x=this_end
*/
          if (abs (this_end_x - in_x) < best_x_diff)
            {
              best_x_diff = abs (this_end_x - in_x);
              best_x = this_end_x;
              hor_pos=1;
            }
          if (abs (this_end_y - in_y) < best_y_diff)
            {
              best_y_diff = abs (this_end_y - in_y);
              best_y = this_end_y;
              ver_pos=1;
            }
/*
 middles aligned
                    this_x     this_mid_x     this_end_x
                       in_x    in_mid_x    in_end_x
                    x=this_mid_x-in_width/2
*/
          if (abs (this_mid_x - in_mid_x) < best_x_diff)
            {
              best_x_diff = abs (this_mid_x - in_mid_x);
              best_x = this_mid_x - in_width/2;
              hor_pos=2;
            }
          if (abs (this_mid_y - in_mid_y) < best_y_diff)
            {
              best_y_diff = abs (this_mid_y - in_mid_y);
              best_y = this_mid_y - in_height/2;
              ver_pos=2;
            }
        }

        {
          if (best_x_diff < SNAP_THRESHOLD)
            {
              *out_x = best_x;
            }
          else
            {
              hor_pos = 0;
            }
          if (best_y_diff < SNAP_THRESHOLD)
            {
              *out_y = best_y;
            }
          else
            {
              ver_pos = 0;
            }
        }
    }
}


static void snap_size (ClutterActor *actor,
                       gfloat        in_width,
                       gfloat        in_height,
                       gfloat       *out_width,
                       gfloat       *out_height)
{
  *out_width = in_width;
  *out_height = in_height;

  ClutterActor *parent;


  parent = clutter_actor_get_parent (actor);

  if (CLUTTER_IS_CONTAINER (parent))
    {
      gfloat in_x = clutter_actor_get_x (actor);
      gfloat in_y = clutter_actor_get_y (actor);
      gfloat in_end_x = in_x + in_width;
      gfloat in_end_y = in_y + in_height;


      gfloat best_x = 0;
      gfloat best_x_diff = 4096;
      gfloat best_y = 0;
      gfloat best_y_diff = 4096;
      GList *children, *c;
      children = clutter_container_get_children (CLUTTER_CONTAINER (parent));

      hor_pos = 0;
      ver_pos = 0;

      /* We only search our siblings for snapping...
       * perhaps we should search more.
       */

      for (c=children; c; c = c->next)
        {
          gfloat this_x = clutter_actor_get_x (c->data);
          gfloat this_width = clutter_actor_get_width (c->data);
          gfloat this_height = clutter_actor_get_height (c->data);
          gfloat this_end_x = this_x + this_width;
          gfloat this_y = clutter_actor_get_y (c->data);
          gfloat this_end_y = this_y + this_height;

          /* skip self */
          if (c->data == actor)
            continue;

/*
 end aligned with start
                    this_x     this_mid_x     this_end_x
in_x    in_mid_x    in_end_x
*/
          if (abs (this_x - in_end_x) < best_x_diff)
            {
              best_x_diff = abs (this_x - in_end_x);
              best_x = this_x;
              hor_pos=3;
            }
          if (abs (this_y - in_end_y) < best_y_diff)
            {
              best_y_diff = abs (this_y - in_end_y);
              best_y = this_y;
              ver_pos=3;
            }
/*
 ends aligned
                    this_x     this_mid_x     this_end_x
                          in_x    in_mid_x    in_end_x
*/
          if (abs (this_end_x - in_end_x) < best_x_diff)
            {
              best_x_diff = abs (this_end_x - in_end_x);
              best_x = this_end_x;
              hor_pos=3;
            }
          if (abs (this_end_y - in_end_y) < best_y_diff)
            {
              best_y_diff = abs (this_end_y - in_end_y);
              best_y = this_end_y;
              ver_pos=3;
            }
        }

        {
          if (best_x_diff < SNAP_THRESHOLD)
            {
              *out_width = best_x-in_x;
            }
          else
            {
              hor_pos = 0;
            }
          if (best_y_diff < SNAP_THRESHOLD)
            {
              *out_height = best_y-in_y;
            }
          else
            {
              ver_pos = 0;
            }
        }
    }
}

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

          snap_position (data, x, y, &x, &y);

          clutter_actor_set_position (data, x, y);
          CB_REV++;

          manipulate_x=event->motion.x;
          manipulate_y=event->motion.y;

        }
        break;
      case CLUTTER_BUTTON_RELEASE:
        g_signal_handler_disconnect (stage,
                                     manipulate_capture_handler);
        manipulate_capture_handler = 0;
        hor_pos = 0;
        ver_pos = 0;

        clutter_actor_queue_redraw (stage);
      default:
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
  return TRUE;
}


static gboolean
manipulate_resize_capture (ClutterActor *stage, ClutterEvent *event, gpointer data)
{
  switch (event->any.type)
    {
      case CLUTTER_MOTION:
        {
          gfloat ex, ey;
          clutter_actor_transform_stage_point (data, event->motion.x, event->motion.y,
                                               &ex, &ey);
          gfloat w, h;
          clutter_actor_get_size (data, &w, &h);

          w-= manipulate_x-ex;
          h-= manipulate_y-ey;

          snap_size (data, w, h, &w, &h);

          clutter_actor_set_size (data, w, h);
          CB_REV++;

          manipulate_x=ex;
          manipulate_y=ey;
        }
        break;
      case CLUTTER_BUTTON_RELEASE:
        hor_pos = 0;
        ver_pos = 0;
        clutter_actor_queue_redraw (stage);
        g_signal_handler_disconnect (stage,
                                     manipulate_capture_handler);
        manipulate_capture_handler = 0;
      default:
        break;
    }
  return TRUE;
}

static gboolean manipulate_resize_press (ClutterActor  *actor,
                                         ClutterEvent  *event)
{
  manipulate_x = event->button.x;
  manipulate_y = event->button.y;

  clutter_actor_transform_stage_point (actor, event->button.x, event->button.y, &manipulate_x, &manipulate_y);

  manipulate_capture_handler = 
     g_signal_connect (clutter_actor_get_stage (actor), "captured-event",
                       G_CALLBACK (manipulate_resize_capture), actor);
  return TRUE;
}

static void get_all_actors_int (GList **list, ClutterActor *actor)
{
  if (util_has_ancestor (actor, parasite_root))
    return;

  if (!CLUTTER_IS_STAGE (actor))
    *list = g_list_prepend (*list, actor);

  if (CLUTTER_IS_CONTAINER (actor))
    {
      GList *children, *c;
      children = clutter_container_get_children (CLUTTER_CONTAINER (actor));
      for (c = children; c; c=c->next)
        {
          get_all_actors_int (list, c->data);
        }
      g_list_free (children);
    }
}
static GList *get_all_actors (ClutterActor *actor)
{
  GList *ret = NULL;
  get_all_actors_int (&ret, actor);
  return ret;
}


static gboolean
intersects (gint min, gint max, gint minb, gint maxb)
{
  if (minb <= max && minb >= min)
    return TRUE;
  if (min <= maxb && min >= minb)
    return TRUE;
  return FALSE;
}

#define LASSO_BORDER 1


static gboolean
manipulate_lasso_capture (ClutterActor *stage, ClutterEvent *event, gpointer data)
{
  switch (event->any.type)
    {
      case CLUTTER_MOTION:
        {
          gfloat ex=event->motion.x;
          gfloat ey=event->motion.y;

          gint mx = MIN (ex, lx);
          gint my = MIN (ey, ly);
          gint mw = MAX (ex, lx) - mx;
          gint mh = MAX (ey, ly) - my;

          clutter_actor_set_position (lasso, mx - LASSO_BORDER, my - LASSO_BORDER);
          clutter_actor_set_size (lasso, mw + LASSO_BORDER*2, mh+LASSO_BORDER*2);

          manipulate_x=ex;
          manipulate_y=ey;

          g_hash_table_remove_all (selection);

          {
            gint no;
            GList *j, *list;
           
            list = get_all_actors (stage);

            for (no = 0, j=list; j;no++,j=j->next)
              {
                gfloat cx, cy;
                gfloat cw, ch;
                clutter_actor_get_transformed_position (j->data, &cx, &cy);
                clutter_actor_get_transformed_size (j->data, &cw, &ch);

                if (intersects (mx, mx + mw, cx, cx + cw) &&
                    intersects (my, my + mh, cy, cy + ch))
                  {
                    g_hash_table_insert (selection, j->data, j->data);
                  }
              }
            g_list_free (list);

            if (clutter_event_get_state (event) & CLUTTER_CONTROL_MASK)
              remove_from_selection = TRUE;
            else
              remove_from_selection = FALSE;
          }
        }
        break;
      case CLUTTER_BUTTON_RELEASE:
         {
          ClutterModifierType state = event->button.modifier_state;
          GHashTableIter      iter;
          gpointer            key, value;

          g_hash_table_iter_init (&iter, selection);
          while (g_hash_table_iter_next (&iter, &key, &value))
            {
              if (state & CLUTTER_CONTROL_MASK)
                {
                  if (g_hash_table_lookup (selected, key))
                    {
                      g_hash_table_remove (selected, key);
                    }
                  else
                    {
                      g_hash_table_insert (selected, key, value);
                    }
                }
              else
                {
                  g_hash_table_insert (selected, key, value);
                }
            }
        }
        g_hash_table_remove_all (selection);

        clutter_actor_queue_redraw (stage);
        g_signal_handler_disconnect (stage,
                                     manipulate_capture_handler);
        manipulate_capture_handler = 0;
        clutter_actor_destroy (lasso);
        lasso = NULL;
      default:
        break;
    }
  return TRUE;
}



static gboolean manipulate_select_press (ClutterActor  *actor,
                                         ClutterEvent  *event)
{
   ClutterActor *hit;
   ClutterModifierType state = event->button.modifier_state;


   hit = clutter_stage_get_actor_at_pos (CLUTTER_STAGE (clutter_actor_get_stage (actor)),
                                         CLUTTER_PICK_ALL,
                                         event->button.x, event->button.y);

  if (!((state & CLUTTER_SHIFT_MASK) ||
        (state & CLUTTER_CONTROL_MASK)))
    {
      g_hash_table_remove_all (selected);
    }

  if (hit == clutter_actor_get_stage (actor))
    {
      if (state & CLUTTER_CONTROL_MASK)
        {
          remove_from_selection = TRUE;
        }
      else
        {
          remove_from_selection = FALSE;
        }

      if (lasso == NULL)
        {
          ClutterColor lassocolor       = {0xff,0x0,0x0,0x11};
          ClutterColor lassobordercolor = {0xff,0x0,0x0,0x88};
          lasso = clutter_rectangle_new_with_color (&lassocolor);
          clutter_rectangle_set_border_color (CLUTTER_RECTANGLE (lasso), &lassobordercolor);
          clutter_rectangle_set_border_width (CLUTTER_RECTANGLE (lasso), LASSO_BORDER);
          clutter_container_add_actor (CLUTTER_CONTAINER (parasite_root), lasso);
        }
      lx = event->button.x;
      ly = event->button.y;

      clutter_actor_set_position (lasso, lx-LASSO_BORDER, ly-LASSO_BORDER);
      clutter_actor_set_size (lasso, LASSO_BORDER*2, LASSO_BORDER*2);

      manipulate_x = event->button.x;
      manipulate_y = event->button.y;

      manipulate_capture_handler = 
         g_signal_connect (clutter_actor_get_stage (actor), "captured-event",
                           G_CALLBACK (manipulate_lasso_capture), actor);
    }
  else
    {
#ifdef EDIT_SELF
     select_item (NULL, hit);
#else
     if (!util_has_ancestor (hit, parasite_root) || 1)
       select_item (NULL, hit);
     else
       g_print ("child of foo!\n");
#endif
    }


  return TRUE;
}

typedef enum RunMode {
  RUN_MODE_UI   = 1,
  RUN_MODE_EDIT = 2
} RunMode;

static gint run_mode = RUN_MODE_EDIT|RUN_MODE_UI;

static ClutterActor *edited_text = NULL;
static gboolean text_was_editable = FALSE;
static gboolean text_was_reactive = FALSE;

static gboolean edit_text_start (ClutterActor *actor)
{
  if (NBTK_IS_LABEL (actor))
    actor = nbtk_label_get_clutter_text (NBTK_LABEL (actor));
  edited_text = actor;

  g_object_get (edited_text, "editable", &text_was_editable,
                             "reactive", &text_was_reactive, NULL);
  g_object_set (edited_text, "editable", TRUE,
                             "reactive", TRUE, NULL);
  clutter_stage_set_key_focus (CLUTTER_STAGE (clutter_actor_get_stage (actor)), actor);
  return TRUE;
}

static gboolean edit_text_end (void)
{
  g_object_set (edited_text, "editable", text_was_editable,
                             "reactive", text_was_reactive, NULL);
  edited_text = NULL;
  return TRUE;
}

gboolean manipulator_key_pressed (ClutterActor *stage, guint key);

static gboolean
manipulate_capture (ClutterActor *actor, ClutterEvent *event, gpointer data)
{
  /* pass events through to text being edited */
  if (edited_text)
    { 
      if (event->any.type == CLUTTER_KEY_PRESS && event->key.keyval == CLUTTER_Escape)
        {
          edit_text_end ();
          return TRUE;
        }
      if (event->any.source == edited_text)
        return FALSE;
      else
        {
          /* break out when presses occur outside the ClutterText,
           * this currently means that proper mouse movement and
           * selection is not possible for NbtkLabels and Entries
           * when edited
           */
          switch (event->any.type)
            {
              case CLUTTER_BUTTON_PRESS:
                edit_text_end ();
              default:
                return TRUE;
            }
        }
    }

  if (event->any.type == CLUTTER_KEY_PRESS)
    {
      if (event->key.keyval == CLUTTER_Scroll_Lock ||
          event->key.keyval == CLUTTER_Caps_Lock)
        {
          switch (run_mode)
            {
              case 0:
                run_mode = 2;
                g_print ("Run mode : edit only\n");
                break;
              case 2:
                run_mode = 3;
                g_print ("Run mode : ui with edit\n");
                break;
              default: 
                run_mode = 0;
                g_print ("Run mode : browse\n");
                break;
            }

          if (run_mode & RUN_MODE_UI)
            {
              clutter_actor_show (parasite_ui);
            }
          else
            {
              clutter_actor_hide (parasite_ui);
            }
          return TRUE;
        }
    }

  if (!(run_mode & RUN_MODE_EDIT))
    {
      /* check if it is child of a link, if it is then we override anyways...
       *
       * Otherwise this is the case that slips contorl through to the
       * underlying scene graph.
       */
      if (event->any.type == CLUTTER_BUTTON_PRESS)
        {
           ClutterActor *hit;

           hit = clutter_stage_get_actor_at_pos (CLUTTER_STAGE (clutter_actor_get_stage (actor)),
                                                 CLUTTER_PICK_ALL,
                                                 event->button.x, event->button.y);
           while (hit)
             {
               const gchar *name;
               name = clutter_actor_get_name (hit);
               if (name && g_str_has_prefix (name, "link="))
                 {
                   gchar *filename;
                   filename = g_strdup_printf ("json/%s.json", name+5);

                   g_object_set (title, "text", name +5, NULL);
                   select_item (NULL, clutter_actor_get_stage (parasite_root));
                   return TRUE;
                 }
               hit = clutter_actor_get_parent (hit);
             }
          
        } 
      return FALSE;
    }
  
  if ((clutter_get_motion_events_enabled()==FALSE) ||
      util_has_ancestor (event->any.source, parasite_root))
    {
      return FALSE;
    }
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

          if (selected_actor && clutter_event_get_click_count (event) > 1)
            {
              if (CLUTTER_IS_TEXT (selected_actor) ||
                  NBTK_IS_LABEL (selected_actor))
                {
                  edit_text_start (selected_actor);
                  return TRUE;
                }
            }

          if (selected_actor)
            {
              clutter_actor_get_size (selected_actor, &w, &h);
              clutter_actor_transform_stage_point (selected_actor, x, y, &x, &y);
              x/=w;
              y/=h;
            }

          if (x<0.0 || y < 0.0 || x > 1.0 || y > 1.0 ||
              selected_actor == NULL ||
              (selected_actor && (selected_actor == clutter_actor_get_stage (selected_actor))))
            {
              manipulate_select_press (parasite_root, event);
            }
          else if (x>0.5 && y>0.5)
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
        return TRUE;
      case CLUTTER_KEY_PRESS:
        manipulator_key_pressed (actor, event->key.keyval);
      case CLUTTER_ENTER:
      case CLUTTER_LEAVE:
        return FALSE;
      default:
        break;
    }
  return TRUE;
}

void cb_manipulate_init (ClutterActor *actor)
{
  if (stage_capture_handler)
    {
      g_signal_handler_disconnect (clutter_actor_get_stage (actor),
                                   stage_capture_handler);
      stage_capture_handler = 0;
    }

  stage_capture_handler = 
     g_signal_connect_after (clutter_actor_get_stage (actor), "captured-event",
                             G_CALLBACK (manipulate_capture), NULL);
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




gchar *subtree_to_string (ClutterActor *root);

/* XXX: needs rewrite to not take the parent */
ClutterActor *util_duplicator (ClutterActor *actor, ClutterActor *parent)
{
  ClutterActor *new_actor;

  new_actor = g_object_new (G_OBJECT_TYPE (actor), NULL);
  util_build_transient (actor);
  util_apply_transient (new_actor);

  /* recurse through children? */
  if (CLUTTER_IS_CONTAINER (new_actor))
    {
      GList *children, *c;
      children = clutter_container_get_children (CLUTTER_CONTAINER (actor));
      for (c = children; c; c = c->next)
        {
          util_duplicator (c->data, new_actor);
        }
      g_list_free (children);
    }

  clutter_container_add_actor (CLUTTER_CONTAINER (parent), new_actor);
  return new_actor;
}

static ClutterActor *copy_buf = NULL; /* XXX: should be a GList */

void cb_duplicate_selected (ClutterActor *actor)
{
  if (selected_actor)
    {
      ClutterActor *new_actor;
      ClutterActor *parent;
      
      parent = clutter_actor_get_parent (selected_actor);
      new_actor = util_duplicator (selected_actor, parent);
      {
        gfloat x, y;
        clutter_actor_get_position (new_actor, &x, &y);
        x+=10;y+=10;
        clutter_actor_set_position (new_actor, x, y);
      }
      select_item (NULL, new_actor);
    }
}


void cb_remove_selected (ClutterActor *actor)
{
  if (selected_actor)
    {
      ClutterActor *old_selected = selected_actor;
      if (selected_actor == clutter_actor_get_stage (actor))
        return;
      select_item (NULL, clutter_actor_get_parent (selected_actor));
      clutter_actor_destroy (old_selected);

      CB_REV++;
    }
}


void cb_cut_selected (ClutterActor *actor)
{
  if (selected_actor)
    {
      ClutterActor *parent;

      parent = clutter_actor_get_parent (selected_actor);
      g_object_ref (selected_actor);
      clutter_container_remove_actor (CLUTTER_CONTAINER (parent), selected_actor);
      if (copy_buf)
        g_object_unref (copy_buf);
      copy_buf = selected_actor;
      select_item (NULL, parent);
    }
}

void cb_copy_selected (ClutterActor *actor)
{
  if (selected_actor)
    {
      ClutterActor *new_actor, *parent;

      parent = clutter_actor_get_parent (selected_actor);
      new_actor = util_duplicator (selected_actor, parent);
      {
        gfloat x, y;
        clutter_actor_get_position (new_actor, &x, &y);
        x+=10;y+=10;
        clutter_actor_set_position (new_actor, x, y);
      }
      g_object_ref (new_actor);
      clutter_container_remove_actor (CLUTTER_CONTAINER (parent), new_actor);
      if (copy_buf)
        g_object_unref (copy_buf);
      copy_buf = new_actor;
    }
}

void cb_paste_selected (ClutterActor *actor)
{
  if (selected_actor && copy_buf)
    {
      ClutterActor *new_actor, *parent;

      if (CLUTTER_IS_CONTAINER (selected_actor))
        {
          parent = selected_actor;
        }
      else
        {
          parent = clutter_actor_get_parent (selected_actor);
        }
      new_actor = util_duplicator (copy_buf, parent);
      {
        gfloat x, y;
        clutter_actor_get_position (new_actor, &x, &y);
        x+=10;y+=10;
        clutter_actor_set_position (new_actor, x, y);
      }
      select_item (NULL, new_actor);
    }
}

void cb_raise_selected (ClutterActor *actor)
{
  if (selected_actor)
    {
      clutter_actor_raise (selected_actor, NULL);
      CB_REV++;
    }
}

void cb_lower_selected (ClutterActor *actor)
{
  if (selected_actor)
    {
      clutter_actor_lower (selected_actor, NULL);
      CB_REV++;
    }
}


void cb_raise_top_selected (ClutterActor *actor)
{
  if (selected_actor)
    {
      clutter_actor_raise_top (selected_actor);
      CB_REV++;
    }
}

void cb_lower_bottom_selected (ClutterActor *actor)
{
  if (selected_actor)
    {
      clutter_actor_lower_bottom (selected_actor);
      CB_REV++;
    }
}


void cb_reset_size (ClutterActor *actor)
{
  if (selected_actor)
    {
      clutter_actor_set_size (selected_actor, -1, -1);
      CB_REV++;
    }
}

static GList *actor_types_build (GList *list, GType type)
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

static void change_type (ClutterActor *actor,
                         const gchar  *new_type)
{
  /* XXX: we need to recreate our correct position in parent as well */
  ClutterActor *new_actor, *parent;

  g_print ("CHANGE type\n");
  hrn_popup_close ();

  new_actor = g_object_new (g_type_from_name (new_type), NULL);
  parent = clutter_actor_get_parent (selected_actor);

    util_build_transient (selected_actor);

    if (CLUTTER_IS_CONTAINER (selected_actor) && CLUTTER_IS_CONTAINER (new_actor))
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

  util_apply_transient (new_actor);
  util_remove_children (property_editors);
  clutter_actor_destroy (selected_actor);
  clutter_container_add_actor (CLUTTER_CONTAINER (parent), new_actor);

  if (g_str_equal (new_type, "ClutterText"))
    {
      g_object_set (G_OBJECT (new_actor), "text", "New Text", NULL);
    }

  select_item (NULL, new_actor);
}


static void printname (gchar *name, ClutterActor *container)
{
  ClutterActor *button;
  gint i;
  for (i=0;blacklist_types[i];i++)
    {
      if (g_str_equal (blacklist_types[i], name))
        return;
    }
  button = CLUTTER_ACTOR (nbtk_button_new_with_label (name));
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
  hrn_popup_actor_fixed (parasite_root, 0,0, actor);
}

void cb_quit (ClutterActor *actor)
{
  clutter_main_quit ();
}


void cb_focus_entry (ClutterActor *actor)
{
  clutter_stage_set_key_focus (CLUTTER_STAGE (clutter_actor_get_stage (actor)),
                               title);
}


void load_file (ClutterActor *actor, const gchar *title)
{
  static gchar *filename = NULL; /* this needs to be more global and accessable, at
                                  * least through some form of getter function.
                                  */

  if (CB_REV != CB_SAVED_REV)
    {
      ClutterActor *root;
      gchar *content;

      root = clutter_actor_get_stage (actor);
      {
        GList *children, *c;
        children = clutter_container_get_children (CLUTTER_CONTAINER (root));
        for (c=children;c;c=c->next)
          {
            const gchar *id = clutter_scriptable_get_id (CLUTTER_SCRIPTABLE (c->data));
            if (id && g_str_equal (id, "actor"))
              {
                root = c->data;
                break;
              }
          }
        g_list_free (children);
      }

      if (root == clutter_actor_get_stage (actor))
        {
          g_print ("didn't find nuthin but root\n");
        }
      else
        {
        }

      content  = subtree_to_string (root);
      if (filename)
        {
          g_print ("Saved changes to %s:\n%s\n\n", filename, content);
          g_file_set_contents (filename, content, -1, NULL);
        }
      g_free (content);
    }

  filename = g_strdup_printf ("json/%s.json", title);
  util_remove_children (property_editors);
  util_remove_children (scene_graph);
  if (g_file_test (filename, G_FILE_TEST_IS_REGULAR))
    {
      util_replace_content2 (actor, "content", filename);
      CB_REV = CB_SAVED_REV = 0;
    }
  else
    {
      util_replace_content2 (clutter_stage_get_default(), "content", NULL);
      CB_REV = CB_SAVED_REV = 0;
    }
  g_hash_table_remove_all (selected);
}

void entry_text_changed (ClutterActor *actor)
{
  const gchar *title = clutter_text_get_text (CLUTTER_TEXT (actor));
  load_file (actor, title);
  clutter_actor_raise_top (parasite_root);
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


void parasite_rectangle_init_hack (ClutterActor  *actor)
{
  static gboolean done = FALSE; 
  if (done)
    return;
  done = TRUE;
  clutter_container_child_set (CLUTTER_CONTAINER (clutter_actor_get_parent (actor)),
                               actor, "expand", TRUE, NULL);
}

static gboolean add_stencil (ClutterActor *actor,
                             ClutterEvent *event,
                             const gchar  *path)
{
  ClutterActor *new_actor;

  new_actor = util_load_json (path);
  if (new_actor)
    {
      gfloat sw, sh, w, h;
      clutter_container_add_actor (CLUTTER_CONTAINER (clutter_actor_get_stage (actor)), new_actor);
      clutter_actor_get_size (clutter_actor_get_stage (actor), &sw, &sh);
      clutter_actor_get_size (new_actor, &w, &h);
      clutter_actor_set_position (new_actor, (sw-w)/2, (sh-h)/2);

      select_item (NULL, new_actor);
    }

  return TRUE;
}


static gboolean load_layout (ClutterActor *actor,
                             ClutterEvent *event,
                             const gchar  *path)
{
  gchar *new_title = g_strdup (path);
  gchar *dot;

  dot = strrchr(new_title, '.');
  if (dot)
    *dot='\0';
  g_object_set (title, "text", new_title +5, NULL);
  g_free (new_title);
  return TRUE;
}


void stencils_container_init_hack (ClutterActor  *actor)
{
  /* we hook this up to the first paint, since no other signal seems to
   * be available to hook up for some additional initialization
   */
  static gboolean done = FALSE; 
  if (done)
    return;
  done = TRUE;

  {
    GDir *dir = g_dir_open ("stencils", 0, NULL);
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
            path = g_strdup_printf ("stencils/%s", name);
            oi = util_load_json (path);
            if (oi)
              {
                gfloat width, height;
                gfloat scale;
                clutter_actor_get_size (oi, &width, &height);
                scale = 100/width;
                if (100/height < scale)
                  scale = 100/height;
                clutter_actor_set_scale (oi, scale, scale);
                clutter_actor_set_size (group, width*scale, height*scale);
                clutter_actor_set_size (rectangle, 100, height*scale);

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


