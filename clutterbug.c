#include <nbtk/nbtk.h>
#include <clutter/clutter.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include "util.h"
#include "hrn-popup.h"
#include "clutterbug.h"

/*#define EDIT_SELF*/
static ClutterActor  *title, *name, *parents,
                     *property_editors, *scene_graph;
ClutterActor *parasite_root;
ClutterActor *parasite_ui;
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

#define PKGDATADIR "./" //"/home/pippin/src/clutterbug/"

static ClutterColor  white = {0xff,0xff,0xff,0xff};
static ClutterColor  yellow = {0xff,0xff,0x44,0xff};
void init_types (void);

guint CB_REV       = 0; /* everything that changes state and could be determined
                         * a user change should increment this.
                         */
guint CB_SAVED_REV = 0;

void cb_manipulate_init (ClutterActor *actor);


void stage_size_changed (ClutterActor *stage, gpointer ignored, ClutterActor *bin)
{
  gfloat width, height;
  clutter_actor_get_size (stage, &width, &height);
  clutter_actor_set_size (bin, width, height);
}


static gboolean keep_on_top (gpointer actor)
{
  clutter_actor_raise_top (actor);
  return FALSE;
}

void set_title (const gchar *new_title)
{
   g_object_set (title, "text", new_title, NULL);
}

gboolean idle_add_stage (gpointer stage)
{
  ClutterActor *actor;
  ClutterScript *script;

  actor = util_load_json (PKGDATADIR "parasite.json");
  g_object_set_data (G_OBJECT (actor), "clutter-bug", (void*)TRUE);
  clutter_container_add_actor (CLUTTER_CONTAINER (stage), actor);
  g_timeout_add (4000, keep_on_top, actor);

  actor_editing_init (stage);
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

static void selected_vanished (gpointer data,
                               GObject *where_the_object_was)
{
  selected_actor = NULL;
  select_item (NULL, NULL);
}

void select_item (ClutterActor *button, ClutterActor *item)
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


