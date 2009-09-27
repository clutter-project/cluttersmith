/* cluttersmith-context.c */

#include "cluttersmith.h"
#include "cluttersmith-context.h"

G_DEFINE_TYPE (CluttersmithContext, cluttersmith_context, G_TYPE_OBJECT)

#define CONTEXT_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), CLUTTERSMITH_TYPE_CONTEXT, CluttersmithContextPrivate))

enum
{
  PROP_0,
  PROP_UI_MODE,
  PROP_FULLSCREEN,
};


struct _CluttersmithContextPrivate
{
};

static void
cluttersmith_context_get_property (GObject    *object,
                                   guint       property_id,
                                   GValue     *value,
                                   GParamSpec *pspec)
{ 
  CluttersmithContext *context = CLUTTERSMITH_CONTEXT (object);
  switch (property_id)
    {
      case PROP_UI_MODE:
        g_value_set_int (value, context->ui_mode);
        break;
      case PROP_FULLSCREEN:
#if 0
        g_value_set_boolean (value,
                             clutter_stage_get_fullscreen (
                                 CLUTTER_STAGE (clutter_stage_get_default())));
      #endif
        break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
cluttersmith_context_set_property (GObject      *object,
                                   guint         property_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
  switch (property_id)
    {
      case PROP_UI_MODE:
        cluttersmith_set_ui_mode (g_value_get_int (value));
        break;
      case PROP_FULLSCREEN:
#if 0
        clutter_stage_set_fullscreen (CLUTTER_STAGE (
                                         clutter_stage_get_default()),
                                      g_value_get_boolean (value));
#endif
        break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
cluttersmith_context_dispose (GObject *object)
{
  G_OBJECT_CLASS (cluttersmith_context_parent_class)->dispose (object);
}

static void
cluttersmith_context_finalize (GObject *object)
{
  G_OBJECT_CLASS (cluttersmith_context_parent_class)->finalize (object);
}

static void
cluttersmith_context_class_init (CluttersmithContextClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GParamSpec   *pspec;

  g_type_class_add_private (klass, sizeof (CluttersmithContextPrivate));

  object_class->get_property = cluttersmith_context_get_property;
  object_class->set_property = cluttersmith_context_set_property;
  object_class->dispose = cluttersmith_context_dispose;
  object_class->finalize = cluttersmith_context_finalize;

  pspec = g_param_spec_int ("ui-mode", "UI mode", "The user interface mode 0 browse, 1=edit 2=chrome 3=edit chrome", 0, 3, 3, G_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_UI_MODE, pspec);
  pspec = g_param_spec_boolean ("fullscreen", "Fullscreen", "fullscreen", FALSE, G_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_FULLSCREEN, pspec);

}

static void
cluttersmith_context_init (CluttersmithContext *self)
{
  self->priv = CONTEXT_PRIVATE (self);
}

CluttersmithContext *
cluttersmith_context_new (void)
{
  return g_object_new (CLUTTERSMITH_TYPE_CONTEXT, NULL);
}



#include <nbtk/nbtk.h>
#include <clutter/clutter.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include "util.h"
#include "popup.h"
#include "cluttersmith.h"

CluttersmithContext *cluttersmith = NULL;

static guint  CS_REVISION;
static guint  CS_STORED_REVISION;

static ClutterActor  *title, *name, *parents;





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

void cluttersmith_dirtied (void)
{
  CS_REVISION++;
}

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
  clutter_actor_get_transformed_position (util_find_by_id_int (clutter_actor_get_stage (cluttersmith->parasite_root), "fake-stage-rect"), &x, &y);
  clutter_actor_set_position (cluttersmith->fake_stage, x, y);
  clutter_actor_show (cluttersmith->parasite_ui);
  has_chrome = TRUE;
}

static void cluttsmith_hide_chrome (void)
{
  gfloat x, y;
  clutter_actor_hide (cluttersmith->parasite_ui);
  clutter_actor_get_transformed_position (util_find_by_id_int (clutter_actor_get_stage (cluttersmith->parasite_root), "fake-stage-rect"), &x, &y);
  clutter_actor_set_position (cluttersmith->fake_stage, 0, 0);
  has_chrome = FALSE;
}

guint cluttersmith_get_ui_mode (void)
{
  return cluttersmith->ui_mode;
}

void cluttersmith_set_ui_mode (guint ui_mode)
{
  cluttersmith->ui_mode = ui_mode;

  if (cluttersmith->ui_mode & CLUTTERSMITH_UI_MODE_UI)
    {
      cluttsmith_show_chrome ();
    }
  else
    {
      cluttsmith_hide_chrome ();
    }
  g_object_notify (G_OBJECT (cluttersmith), "ui-mode");
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

  cluttersmith = cluttersmith_context_new ();

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
  cluttersmith->property_editors = CLUTTER_ACTOR (clutter_script_get_object (script, "property-editors"));
  cluttersmith->scene_graph = CLUTTER_ACTOR (clutter_script_get_object (script, "scene-graph"));
  cluttersmith->parasite_ui = CLUTTER_ACTOR (clutter_script_get_object (script, "parasite-ui"));
  cluttersmith->parasite_root = CLUTTER_ACTOR (clutter_script_get_object (script, "parasite-root"));

    {
  if (cluttersmith->parasite_ui)
    {
      g_signal_connect (stage, "notify::width", G_CALLBACK (stage_size_changed), cluttersmith->parasite_ui);
      g_signal_connect (stage, "notify::height", G_CALLBACK (stage_size_changed), cluttersmith->parasite_ui);
      /* do an initial sync of the ui-size */
      stage_size_changed (stage, NULL, cluttersmith->parasite_ui);
    }
    }

  cb_manipulate_init (cluttersmith->parasite_root);
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

#define INDENTATION_AMOUNT  20




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

static gboolean
update_id (ClutterText *text,
           gpointer     data)
{
  clutter_scriptable_set_id (CLUTTER_SCRIPTABLE (data), clutter_text_get_text (text));
  return TRUE;
}


void actor_defaults_populate (ClutterActor *container,
                              ClutterActor *actor)
{
    ClutterColor  white = {0xff,0xff,0xff,0xff};  /* XXX: should be in CSS */
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

    clutter_container_add_actor (CLUTTER_CONTAINER (container), hbox);

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
    clutter_container_add_actor (CLUTTER_CONTAINER (container), hbox);
}


void cluttersmith_set_active (ClutterActor *item)
{
  if (item == NULL)
    item = clutter_stage_get_default ();
  if (item)
    clutter_text_set_text (CLUTTER_TEXT (name), G_OBJECT_TYPE_NAME (item));

  util_remove_children (parents);
  util_remove_children (cluttersmith->property_editors);
  util_remove_children (cluttersmith->scene_graph);

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

          actor_defaults_populate (cluttersmith->property_editors, active_actor);
          props_populate (cluttersmith->property_editors, G_OBJECT (active_actor));
          cluttersmith_tree_populate (cluttersmith->scene_graph, active_actor);
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
  util_remove_children (cluttersmith->property_editors);
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
  popup_actor_fixed (cluttersmith->parasite_root, 0,0, actor);
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

      clutter_actor_get_position (cluttersmith->fake_stage, &x, &y);
      clutter_actor_set_position (cluttersmith->fake_stage, 0.0, 0.0);

      content  = json_serialize_subtree (cluttersmith->fake_stage);
      clutter_actor_set_position (cluttersmith->fake_stage, x, y);
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
  util_remove_children (cluttersmith->property_editors);
  util_remove_children (cluttersmith->scene_graph);
  if (g_file_test (filename, G_FILE_TEST_IS_REGULAR))
    {
      session_history_add (cluttersmith_get_project_root ());
      cluttersmith->fake_stage = util_replace_content2 (actor, "fake-stage", filename);
    }
  else
    {
      cluttersmith->fake_stage = util_replace_content2 (actor, "fake-stage", NULL);

    }
  cluttersmith_set_current_container (cluttersmith->fake_stage);
  {
    gfloat x=0, y=0;
      if (has_chrome)
        clutter_actor_get_transformed_position (
           util_find_by_id_int (clutter_actor_get_stage (cluttersmith->parasite_root),
           "fake-stage-rect"), &x, &y);
    clutter_actor_set_position (cluttersmith->fake_stage, x, y);
  }
  CS_REVISION = CS_STORED_REVISION = 0;

  cluttersmith_selected_clear ();
  clutter_actor_raise_top (cluttersmith->parasite_root);
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



void cluttersmith_set_project_root (const gchar *new_root)
{
  if (cluttersmith->project_root &&
      g_str_equal (cluttersmith->project_root, new_root))
    {
      return;
    }

  g_object_set (util_find_by_id_int (clutter_actor_get_stage (cluttersmith->parasite_root), "project-root"),
                "text", new_root, NULL);
}

gchar *cluttersmith_get_project_root (void)
{
  return cluttersmith->project_root;
}


static void project_root_text_changed (ClutterActor *actor)
{
  const gchar *new_text = clutter_text_get_text (CLUTTER_TEXT (actor));
  if (cluttersmith->project_root)
    g_free (cluttersmith->project_root);
  cluttersmith->project_root = g_strdup (new_text);

  if (g_file_test (cluttersmith->project_root, G_FILE_TEST_IS_DIR))
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


void child_expand_init_hack (ClutterActor  *actor)
{
  if (g_object_get_data (G_OBJECT (actor), "child-expand-hack"))
    return;
  g_object_set_data (G_OBJECT (actor), "child-expand-hack", (void*)0xff);
  clutter_container_child_set (CLUTTER_CONTAINER (clutter_actor_get_parent (actor)),
                               actor, "expand", TRUE, NULL);
}
