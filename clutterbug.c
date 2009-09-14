#include <nbtk/nbtk.h>
#include <clutter/clutter.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include "util.h"
#include "hrn-popup.h"
#include "clutterbug.h"

static ClutterColor  white = {0xff,0xff,0xff,0xff};  /* XXX: should be in CSS */
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
          tree_populate (scene_graph, selected_actor);
        }
    }
  if (selected_actor)
  clutter_stage_set_key_focus (CLUTTER_STAGE (clutter_actor_get_stage (selected_actor)), NULL);
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



void entry_text_changed (ClutterActor *actor)
{
  const gchar *title = clutter_text_get_text (CLUTTER_TEXT (actor));
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

