#include <nbtk/nbtk.h>
#include <clutter/clutter.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include "util.h"
#include "popup.h"
#include "cluttersmith.h"

static ClutterColor  white = {0xff,0xff,0xff,0xff};  /* XXX: should be in CSS */
/*#define EDIT_SELF*/
static ClutterActor  *title, *name, *parents,
                     *property_editors, *scene_graph;

ClutterActor *parasite_root;
ClutterActor *parasite_ui;
ClutterActor *fake_stage = NULL;

gchar *whitelist[]={"depth", "opacity",
                    "scale-x","scale-y", "anchor-x", "color",
                    "anchor-y", "rotation-angle-z",
                    "name", "reactive",
                    NULL};



static ClutterActor *active_actor = NULL;
void load_file (ClutterActor *actor, const gchar *title);

gchar *blacklist_types[]={"ClutterStage",
                          "ClutterCairoTexture",
                          "ClutterStageGLX",
                          "ClutterStageX11",
                          "ClutterActor",
                          "NbtkWidget",
                          "ClutterIMText",

                          NULL};

void init_types (void);

guint CS_REVISION       = 0; /* everything that changes state and could be determined
                         * a user change should increment this.
                         */
guint CS_STORED_REVISION = 0;

void cb_manipulate_init (ClutterActor *actor);


void stage_size_changed (ClutterActor *stage, gpointer ignored, ClutterActor *bin)
{
  gfloat width, height;
  clutter_actor_get_size (stage, &width, &height);
  g_print ("stage size changed %f %f\n", width, height);
  clutter_actor_set_size (bin, width, height);
}

static gboolean has_chrome = TRUE;
static void cluttsmith_show_chrome (void)
{
  gfloat x, y;
  clutter_actor_get_transformed_position (util_find_by_id_int (clutter_actor_get_stage (parasite_root), "fake-stage-rect"), &x, &y);
  clutter_actor_set_position (fake_stage, x, y);
  clutter_actor_show (parasite_ui);
  has_chrome = TRUE;
}

static void cluttsmith_hide_chrome (void)
{
  gfloat x, y;
  clutter_actor_hide (parasite_ui);
  clutter_actor_get_transformed_position (util_find_by_id_int (clutter_actor_get_stage (parasite_root), "fake-stage-rect"), &x, &y);
  clutter_actor_set_position (fake_stage, 0, 0);
  has_chrome = FALSE;
}

void cluttersmith_set_ui_mode (guint ui_mode)
{
  cluttersmith_ui_mode = ui_mode;

  if (cluttersmith_ui_mode & RUN_MODE_UI)
    {
      cluttsmith_show_chrome ();
    }
  else
    {
      cluttsmith_hide_chrome ();
    }
}


static gboolean keep_on_top (gpointer actor)
{
  clutter_actor_raise_top (actor);
  return FALSE;
}

void cluttersmith_open_layout (const gchar *new_title)
{
   g_object_set (title, "text", new_title, NULL);
}

gboolean idle_add_stage (gpointer stage)
{
  ClutterActor *actor;
  ClutterScript *script;
#ifdef COMPILEMODULE
  actor = util_load_json (PKGDATADIR "cluttersmith-assistant.json");
#else
  actor = util_load_json (PKGDATADIR "cluttersmith.json");
#endif
  g_object_set_data (G_OBJECT (actor), "clutter-smith", (void*)TRUE);
  clutter_container_add_actor (CLUTTER_CONTAINER (stage), actor);
  g_timeout_add (4000, keep_on_top, actor);

  cluttersmith_actor_editing_init (stage);
  nbtk_style_load_from_file (nbtk_style_get_default (), PKGDATADIR "cluttersmith.css", NULL);
  script = util_get_script (actor);

  /* initializing globals */
  title = CLUTTER_ACTOR (clutter_script_get_object (script, "title"));
  name = CLUTTER_ACTOR (clutter_script_get_object (script, "name"));
  parents = CLUTTER_ACTOR (clutter_script_get_object (script, "parents"));
  scene_graph = CLUTTER_ACTOR (clutter_script_get_object (script, "scene-graph"));
  property_editors = CLUTTER_ACTOR (clutter_script_get_object (script, "property-editors"));
  parasite_ui = CLUTTER_ACTOR (clutter_script_get_object (script, "parasite-ui"));
  parasite_root = CLUTTER_ACTOR (clutter_script_get_object (script, "parasite-root"));

    {
  if (parasite_ui)
    {
      g_signal_connect (stage, "notify::width", G_CALLBACK (stage_size_changed), parasite_ui);
      g_signal_connect (stage, "notify::height", G_CALLBACK (stage_size_changed), parasite_ui);
      /* do an initial sync of the ui-size */
      stage_size_changed (stage, NULL, parasite_ui);
    }
    }

  cb_manipulate_init (parasite_root);
  cluttersmith_set_active (stage);


  init_types ();
  return FALSE;
}

#ifdef COMPILEMODULE
static void stage_added (ClutterStageManager *manager,
                         ClutterStage        *stage)
{
  g_timeout_add (100, idle_add_stage, stage);
}

static void _cluttersmith_init(void)
    __attribute__ ((constructor));
static void _cluttersmith_fini(void)
    __attribute__ ((destructor));

static void _cluttersmith_init(void) {
  g_type_init ();
  ClutterStageManager *manager = clutter_stage_manager_get_default ();

  g_signal_connect (manager, "stage-added", G_CALLBACK (stage_added), NULL);
}

static void _cluttersmith_fini(void) {
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
  active_actor = NULL;
  cluttersmith_set_active (NULL);
}

static void cluttersmith_set_active_event (ClutterActor *button, ClutterActor *item)
{
  cluttersmith_set_active (item);
}


ClutterActor *cluttersmith_get_active (void)
{
  return active_actor;
}

void cluttersmith_set_active (ClutterActor *item)
{
  if (item == NULL)
    item = clutter_stage_get_default ();
  if (item)
    clutter_text_set_text (CLUTTER_TEXT (name), G_OBJECT_TYPE_NAME (item));

  util_remove_children (parents);
  util_remove_children (property_editors);
  util_remove_children (scene_graph);

    {
      if (active_actor)
        {
          g_object_weak_unref (G_OBJECT (active_actor), selected_vanished, NULL);
          active_actor = NULL;
        }

      active_actor = item;
      if (CLUTTER_IS_ACTOR (item))
        {
          ClutterActor *iter = clutter_actor_get_parent (item);
          while (iter && CLUTTER_IS_ACTOR (iter))
            {
              ClutterActor *new;
              new = CLUTTER_ACTOR (nbtk_button_new_with_label (G_OBJECT_TYPE_NAME (iter)));
              g_signal_connect (new, "clicked", G_CALLBACK (cluttersmith_set_active_event), iter);
              clutter_container_add_actor (CLUTTER_CONTAINER (parents), new);
              iter = clutter_actor_get_parent (iter);
            }

          g_object_weak_ref (G_OBJECT (item), selected_vanished, NULL);

          props_populate (active_actor);
          cluttersmith_tree_populate (scene_graph, active_actor);
        }
    }
#if 0
  if (active_actor)
    clutter_stage_set_key_focus (CLUTTER_STAGE (clutter_actor_get_stage (parasite_root)), NULL);
#endif
}

gchar *json_serialize_subtree (ClutterActor *root);

static GList *actor_types_build (GList *list, GType type)
{
  GType *types;
  guint  children;
  gint   no;

  if (!type)
    return list;

  list = g_list_prepend (list, (void*)g_type_name (type));

  types = g_type_children (type, &children);

  for (no=0; no<children; no++)
    {
      list = actor_types_build (list, types[no]);
    }
  if (types)
    g_free (types);
  return list;
}

static void change_type (ClutterActor *actor,
                         const gchar  *new_type)
{
  /* XXX: we need to recreate our correct position in parent as well */
  ClutterActor *new_actor, *parent;

  popup_close ();
  if (CLUTTER_IS_STAGE (active_actor))
    {
      g_warning ("refusing to change type of stage");
      return;
    }

  new_actor = g_object_new (g_type_from_name (new_type), NULL);
  parent = clutter_actor_get_parent (active_actor);

    util_build_transient (active_actor);

    if (CLUTTER_IS_CONTAINER (active_actor) && CLUTTER_IS_CONTAINER (new_actor))
      {
        GList *c, *children;
        children = clutter_container_get_children (CLUTTER_CONTAINER (active_actor));
        for (c = children; c; c = c->next)
          {
            ClutterActor *child = g_object_ref (c->data);
            clutter_container_remove_actor (CLUTTER_CONTAINER (active_actor), child);
            clutter_container_add_actor (CLUTTER_CONTAINER (new_actor), child);

          }
        g_list_free (children);
      }

  util_apply_transient (new_actor);
  util_remove_children (property_editors);
  clutter_actor_destroy (active_actor);
  clutter_container_add_actor (CLUTTER_CONTAINER (parent), new_actor);

  if (g_str_equal (new_type, "ClutterText"))
    {
      g_object_set (G_OBJECT (new_actor), "text", "New Text", NULL);
    }

  cluttersmith_set_active (new_actor);
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

  if (!active_actor)
    return;
 
  if (!types)
    {
       types = actor_types_build (NULL, CLUTTER_TYPE_ACTOR);
       types = g_list_sort (types, (void*)strcmp);
    }
  actor = CLUTTER_ACTOR (nbtk_grid_new ());
  g_object_set (actor, "height", 600.0, "column-major", TRUE, "homogenous-columns", TRUE, NULL);
  g_list_foreach (types, (void*)printname, actor);
  popup_actor_fixed (parasite_root, 0,0, actor);
}

void session_history_add (const gchar *dir);

static gchar *filename = NULL;

static void cluttersmith_save (void)
{
  /* Save if we've changed */
  if (CS_REVISION != CS_STORED_REVISION)
    {
      gchar *content;
      gfloat x, y;

      clutter_actor_get_position (fake_stage, &x, &y);
      clutter_actor_set_position (fake_stage, 0.0, 0.0);

      content  = json_serialize_subtree (fake_stage);
      clutter_actor_set_position (fake_stage, x, y);
      if (filename)
        {
          g_file_set_contents (filename, content, -1, NULL);
        }
      g_free (content);
      CS_STORED_REVISION = CS_REVISION;
    }
}

gboolean cluttersmith_save_timeout (gpointer data)
{
  cluttersmith_save ();
  return TRUE;
}

static void title_text_changed (ClutterActor *actor)
{
  const gchar *title = clutter_text_get_text (CLUTTER_TEXT (actor));

  cluttersmith_save ();

  filename = g_strdup_printf ("%s/%s.json", cluttersmith_get_project_root(),
                              title);
  util_remove_children (property_editors);
  util_remove_children (scene_graph);
  if (g_file_test (filename, G_FILE_TEST_IS_REGULAR))
    {
      session_history_add (cluttersmith_get_project_root ());
      fake_stage = util_replace_content2 (actor, "fake-stage", filename);
    }
  else
    {
      fake_stage = util_replace_content2 (actor, "fake-stage", NULL);

    }
  cluttersmith_set_current_container (fake_stage);
  {
    gfloat x=0, y=0;
      if (has_chrome)
        clutter_actor_get_transformed_position (
           util_find_by_id_int (clutter_actor_get_stage (parasite_root),
           "fake-stage-rect"), &x, &y);
    clutter_actor_set_position (fake_stage, x, y);
  }
  CS_REVISION = CS_STORED_REVISION = 0;

  cluttersmith_selected_clear ();
  clutter_actor_raise_top (parasite_root);
}

char *
cluttersmith_make_config_file (const char *filename)
{
  const char *base;
  char *path, *full;

  base = g_getenv ("XDG_CONFIG_HOME");
  if (base) {
    path = g_strdup_printf ("%s/cluttersmith", base);
    full = g_strdup_printf ("%s/cluttersmith/%s", base, filename);
  } else {
    path = g_strdup_printf ("%s/.config/cluttersmith", g_get_home_dir ());
    full = g_strdup_printf ("%s/.config/cluttersmith/%s", g_get_home_dir (),
                            filename);
  }

  /* Make sure the directory exists */
  if (g_file_test (path, G_FILE_TEST_EXISTS) == FALSE) {
    g_mkdir_with_parents (path, 0700);
  }
  g_free (path);

  return full;
}

void session_history_add (const gchar *dir)
{
  gchar *config_path = cluttersmith_make_config_file ("session-history");
  gchar *start, *end;
  gchar *original = NULL;
  GList *iter, *session_history = NULL;
  
  if (g_file_get_contents (config_path, &original, NULL, NULL))
    {
      start=end=original;
      while (*start)
        {
          end = strchr (start, '\n');
          if (*end)
            {
              *end = '\0';
              if (!g_str_equal (start, dir))
                session_history = g_list_append (session_history, start);
              g_print ("%s\n", start);
              start = end+1;
            }
          else
            {
              start = end;
            }
        }
    }
  session_history = g_list_prepend (session_history, (void*)dir);
  {
    GString *str = g_string_new ("");
    gint i=0;
    for (iter = session_history; iter && i<10; iter=iter->next, i++)
      {
        g_string_append_printf (str, "%s\n", (gchar*)iter->data);
      }
    g_file_set_contents (config_path, str->str, -1, NULL);
    g_string_free (str, TRUE);
  }

  if (original)
    g_free (original);
  g_list_free (session_history);
}

static gchar *project_root = NULL; /* The directory we are loading stuff from */


void cluttersmith_set_project_root (const gchar *new_root)
{
  if (project_root && g_str_equal (project_root, new_root))
    {
      return;
    }

  g_object_set (util_find_by_id_int (clutter_actor_get_stage (parasite_root), "project-root"),
                "text", new_root, NULL);
}

gchar *cluttersmith_get_project_root (void)
{
  return project_root;
}


static void project_root_text_changed (ClutterActor *actor)
{
  const gchar *new_text = clutter_text_get_text (CLUTTER_TEXT (actor));
  if (project_root)
    g_free (project_root);
  project_root = g_strdup (new_text);

  if (g_file_test (project_root, G_FILE_TEST_IS_DIR))
    {
      previews_reload (util_find_by_id_int (clutter_actor_get_stage(actor), "previews-container"));
      cluttersmith_open_layout ("index");
    }
}

void project_root_init_hack (ClutterActor  *actor)
{
  /* we hook this up to the first paint, since no other signal seems to
   * be available to hook up for some additional initialization
   */
  static gboolean done = FALSE; 
  if (done)
    return;
  done = TRUE;

  g_signal_connect (nbtk_entry_get_clutter_text (NBTK_ENTRY (actor)), "text-changed",
                    G_CALLBACK (project_root_text_changed), NULL);
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
                    G_CALLBACK (title_text_changed), NULL);
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


