  /* cluttersmith-context.c */

#include "cluttersmith.h"
#include "cs-context.h"
#include <gjs/gjs.h>
#include <string.h>

  G_DEFINE_TYPE (CSContext, cs_context, G_TYPE_OBJECT)

#define CONTEXT_PRIVATE(o) \
    (G_TYPE_INSTANCE_GET_PRIVATE ((o), CS_TYPE_CONTEXT, CSContextPrivate))

  enum
  {
    PROP_0,
    PROP_UI_MODE,
    PROP_FULLSCREEN,
    PROP_ZOOM,
    PROP_ORIGIN_X,
    PROP_ORIGIN_Y,
    PROP_CANVAS_WIDTH,
    PROP_CANVAS_HEIGHT,
  };


  struct _CSContextPrivate
  {
    gfloat zoom;
    gfloat origin_x;
    gfloat origin_y;
    gfloat canvas_width;
    gfloat canvas_height;
    GjsContext  *page_js_context;
  };

static void cs_context_get_property (GObject    *object,
                                     guint       property_id,
                                     GValue     *value,
                                     GParamSpec *pspec);
static void cs_context_set_property (GObject      *object,
                                     guint         property_id,
                                     const GValue *value,
                                     GParamSpec   *pspec);

static void
cs_context_dispose (GObject *object)
{
  G_OBJECT_CLASS (cs_context_parent_class)->dispose (object);
}

static void
cs_context_finalize (GObject *object)
{
  G_OBJECT_CLASS (cs_context_parent_class)->finalize (object);
}

static void
cs_context_class_init (CSContextClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GParamSpec   *pspec;

  g_type_class_add_private (klass, sizeof (CSContextPrivate));

  object_class->get_property = cs_context_get_property;
  object_class->set_property = cs_context_set_property;
  object_class->dispose = cs_context_dispose;
  object_class->finalize = cs_context_finalize;

  pspec = g_param_spec_int ("ui-mode", "UI mode", "The user interface mode 0 browse, 1=edit 2=chrome 3=edit chrome", 0, 3, 3, G_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_UI_MODE, pspec);


  pspec = g_param_spec_float ("zoom", "Zoom", "zoom level", 0, 3200, 100, G_PARAM_READWRITE|G_PARAM_CONSTRUCT);
  g_object_class_install_property (object_class, PROP_ZOOM, pspec);

  pspec = g_param_spec_int ("canvas-width", "canvas-width", "width of canvas being edited", 0, 1000, 800, G_PARAM_READWRITE|G_PARAM_CONSTRUCT);
  g_object_class_install_property (object_class, PROP_CANVAS_WIDTH, pspec);

  pspec = g_param_spec_int ("canvas-height", "canvas-height", "height of canvas being edited", 0, 1000, 480, G_PARAM_READWRITE|G_PARAM_CONSTRUCT);

  g_object_class_install_property (object_class, PROP_CANVAS_HEIGHT, pspec);
  pspec = g_param_spec_float ("origin-x", "origin X", "origin X coordinate", -2000, 2000, 0, G_PARAM_READWRITE|G_PARAM_CONSTRUCT);
  g_object_class_install_property (object_class, PROP_ORIGIN_X, pspec);

  pspec = g_param_spec_float ("origin-y", "origin Y", "origin Y coordinate", -2000, 2000, 0, G_PARAM_READWRITE|G_PARAM_CONSTRUCT);
  g_object_class_install_property (object_class, PROP_ORIGIN_Y, pspec);


  pspec = g_param_spec_boolean ("fullscreen", "Fullscreen", "fullscreen", FALSE, G_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_FULLSCREEN, pspec);

}



static void
cs_context_init (CSContext *self)
{
  self->priv = CONTEXT_PRIVATE (self);
}

CSContext *
cs_context_new (void)
{
  return g_object_new (CS_TYPE_CONTEXT, NULL);
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

CSContext *cluttersmith = NULL;

static guint  CS_REVISION;
static guint  CS_STORED_REVISION;

static ClutterActor  *title, *name, *name2, *parents;



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

void cs_dirtied (void)
{
  CS_REVISION++;
}

void cs_manipulate_init (ClutterActor *actor);


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
  clutter_actor_get_transformed_position (cs_find_by_id_int (clutter_actor_get_stage (cluttersmith->parasite_root), "fake-stage-rect"), &x, &y);

  /* move fake_stage and canvas into a single group that is transformed? */
  clutter_actor_set_position (cluttersmith->fake_stage, 
    x-cluttersmith->priv->origin_x,
    y-cluttersmith->priv->origin_y);

  clutter_actor_set_position (cluttersmith->fake_stage_canvas, 
    x-cluttersmith->priv->origin_x,
    y-cluttersmith->priv->origin_y);

  clutter_actor_show (cluttersmith->parasite_ui);
  clutter_actor_set_scale (cluttersmith->fake_stage, cluttersmith->priv->zoom/100.0,
                                                     cluttersmith->priv->zoom/100.0);

  clutter_actor_set_scale (cluttersmith->fake_stage_canvas, cluttersmith->priv->zoom/100.0,
                                                     cluttersmith->priv->zoom/100.0);

  has_chrome = TRUE;
}

static void cluttsmith_hide_chrome (void)
{
  gfloat x, y;
  clutter_actor_hide (cluttersmith->parasite_ui);
  clutter_actor_get_transformed_position (cs_find_by_id_int (clutter_actor_get_stage (cluttersmith->parasite_root), "fake-stage-rect"), &x, &y);
  clutter_actor_set_position (cluttersmith->fake_stage, 
    -cluttersmith->priv->origin_x,
    -cluttersmith->priv->origin_y);

  clutter_actor_set_position (cluttersmith->fake_stage_canvas, 
    -cluttersmith->priv->origin_x,
    -cluttersmith->priv->origin_y);

  clutter_actor_set_scale (cluttersmith->fake_stage, cluttersmith->priv->zoom/100.0,
                                                     cluttersmith->priv->zoom/100.0);

  clutter_actor_set_scale (cluttersmith->fake_stage_canvas, cluttersmith->priv->zoom/100.0,
                                                     cluttersmith->priv->zoom/100.0);

  has_chrome = FALSE;
}

guint cs_get_ui_mode (void)
{
  return cluttersmith->ui_mode;
}

static gboolean cs_sync_chrome_idle (gpointer data)
{
  if (!cluttersmith) /*the singleton might not be made yet */
    return FALSE;
  clutter_actor_set_size (cluttersmith->fake_stage_canvas,
                          cluttersmith->priv->canvas_width,
                          cluttersmith->priv->canvas_height);
  if (cluttersmith->ui_mode & CS_UI_MODE_UI)
    {
      cluttsmith_show_chrome ();
    }
  else
    {
      cluttsmith_hide_chrome ();
    }
  return FALSE;
}

void cs_sync_chrome (void)
{
  g_idle_add (cs_sync_chrome_idle, NULL);
  if (cluttersmith->ui_mode & CS_UI_MODE_UI)
    {
      cluttsmith_show_chrome ();
    }
  else
    {
      cluttsmith_hide_chrome ();
    }
}

void cs_set_ui_mode (guint ui_mode)
{
  cluttersmith->ui_mode = ui_mode;
  cs_selected_clear ();
  cs_sync_chrome ();
  g_object_notify (G_OBJECT (cluttersmith), "ui-mode");
}


static void
cs_context_get_property (GObject    *object,
                         guint       property_id,
                         GValue     *value,
                         GParamSpec *pspec)
{ 
  CSContext *context = CS_CONTEXT (object);
  CSContextPrivate *priv = context->priv;
  switch (property_id)
    {
      case PROP_UI_MODE:
        g_value_set_int (value, context->ui_mode);
        break;
      case PROP_ZOOM:
        g_value_set_float (value, priv->zoom);
        break;
      case PROP_ORIGIN_X:
        g_value_set_float (value, priv->origin_x);
        break;
      case PROP_ORIGIN_Y:
        g_value_set_float (value, priv->origin_y);
        break;
      case PROP_CANVAS_WIDTH:
        g_value_set_int (value, priv->canvas_width);
        break;
      case PROP_CANVAS_HEIGHT:
        g_value_set_int (value, priv->canvas_height);
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
cs_context_set_property (GObject      *object,
                         guint         property_id,
                         const GValue *value,
                         GParamSpec   *pspec)
{
  CSContext *context = CS_CONTEXT (object);
  CSContextPrivate *priv = context->priv;
  switch (property_id)
    {
      case PROP_UI_MODE:
        cs_set_ui_mode (g_value_get_int (value));
        break;
      case PROP_ZOOM:
        priv->zoom = g_value_get_float (value);
        cs_sync_chrome_idle (NULL);
        break;
      case PROP_CANVAS_WIDTH:
        priv->canvas_width = g_value_get_int (value);
        cs_sync_chrome_idle (NULL);
        break;
      case PROP_CANVAS_HEIGHT:
        priv->canvas_height = g_value_get_int (value);
        cs_sync_chrome_idle (NULL);
        break;
      case PROP_ORIGIN_X:
        priv->origin_x = g_value_get_float (value);
        cs_sync_chrome_idle (NULL);
        break;
      case PROP_ORIGIN_Y:
        priv->origin_y = g_value_get_float (value);
        cs_sync_chrome_idle (NULL);
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

void cs_open_layout (const gchar *new_title)
{
   g_object_set (title, "text", new_title, NULL);
}

gboolean idle_add_stage (gpointer stage)
{
  ClutterActor *actor;
  ClutterScript *script;

  cluttersmith = cs_context_new ();

#ifdef COMPILEMODULE
  actor = cs_load_json (PKGDATADIR "cluttersmith-assistant.json");
#else
  actor = cs_load_json (PKGDATADIR "cluttersmith.json");
#endif
  g_object_set_data (G_OBJECT (actor), "clutter-smith", (void*)TRUE);
  clutter_container_add_actor (CLUTTER_CONTAINER (stage), actor);

  cs_actor_editing_init (stage);
  nbtk_style_load_from_file (nbtk_style_get_default (), PKGDATADIR "cluttersmith.css", NULL);
  script = cs_get_script (actor);

  /* initializing globals */
  title = CLUTTER_ACTOR (clutter_script_get_object (script, "title"));
  name = CLUTTER_ACTOR (clutter_script_get_object (script, "name"));
  name2 = CLUTTER_ACTOR (clutter_script_get_object (script, "name2"));
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

  cluttersmith->fake_stage_canvas = CLUTTER_ACTOR (clutter_script_get_object (script, "fake-stage-canvas"));
  cluttersmith->resize_handle = CLUTTER_ACTOR (clutter_script_get_object (script, "resize-handle"));
  cluttersmith->move_handle = CLUTTER_ACTOR (clutter_script_get_object (script, "move-handle"));
  cluttersmith->active_panel = CLUTTER_ACTOR (clutter_script_get_object (script, "cs-active-panel"));
  cluttersmith->active_container = CLUTTER_ACTOR (clutter_script_get_object (script, "cs-active-container"));
  cluttersmith->dialog_config = CLUTTER_ACTOR (clutter_script_get_object (script, "cs-dialog-config"));
  cluttersmith->dialog_tree = CLUTTER_ACTOR (clutter_script_get_object (script, "cs-dialog-tree"));
  cluttersmith->dialog_toolbar = CLUTTER_ACTOR (clutter_script_get_object (script, "cs-dialog-toolbar"));
  cluttersmith->dialog_scenes = CLUTTER_ACTOR (clutter_script_get_object (script, "cs-dialog-scenes"));
  cluttersmith->dialog_templates = CLUTTER_ACTOR (clutter_script_get_object (script, "cs-dialog-templates"));
  cluttersmith->dialog_editor= CLUTTER_ACTOR (clutter_script_get_object (script, "cs-dialog-editor"));
  cluttersmith->dialog_property_inspector = CLUTTER_ACTOR (clutter_script_get_object (script, "cs-dialog-property-inspector"));


  cs_manipulate_init (cluttersmith->parasite_root);
  cs_set_active (clutter_actor_get_stage(cluttersmith->parasite_root));

  init_types ();
  return FALSE;
}

#ifdef COMPILEMODULE
static void stage_added (ClutterStageManager *manager,
                         ClutterStage        *stage)
{
  g_timeout_add (100, idle_add_stage, stage);
}

static void _cs_init(void)
    __attribute__ ((constructor));
static void _cs_fini(void)
    __attribute__ ((destructor));

static void _cs_init(void) {
  g_type_init ();
  ClutterStageManager *manager = clutter_stage_manager_get_default ();

  g_signal_connect (manager, "stage-added", G_CALLBACK (stage_added), NULL);
}

static void _cs_fini(void) {
}
#endif


ClutterActor *property_editor_new (GObject *object,
                                   const gchar *property_name);

#define INDENTATION_AMOUNT  20




static void selected_vanished (gpointer data,
                               GObject *where_the_object_was)
{
  active_actor = NULL;
  cs_set_active (NULL);
}

static void cs_set_active_event (ClutterActor *button, ClutterActor *item)
{
  cs_set_active (item);
}


ClutterActor *cs_get_active (void)
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

    label = clutter_text_new_with_text (CS_EDITOR_LABEL_FONT, "pos:");
    clutter_text_set_color (CLUTTER_TEXT (label), &white);
    clutter_actor_set_height (label, EDITOR_LINE_HEIGHT);
    clutter_container_add_actor (CLUTTER_CONTAINER (hbox), label);

    editor = property_editor_new (G_OBJECT (actor), "x");
    clutter_actor_set_size (editor, 40, EDITOR_LINE_HEIGHT);
    clutter_container_add_actor (CLUTTER_CONTAINER (hbox), editor);

    editor = property_editor_new (G_OBJECT (actor), "y");
    clutter_actor_set_size (editor, 40, EDITOR_LINE_HEIGHT);
    clutter_container_add_actor (CLUTTER_CONTAINER (hbox), editor);

    label = clutter_text_new_with_text (CS_EDITOR_LABEL_FONT, "size:");
    clutter_text_set_color (CLUTTER_TEXT (label), &white);
    clutter_actor_set_height (label, EDITOR_LINE_HEIGHT);
    clutter_container_add_actor (CLUTTER_CONTAINER (hbox), label);

    editor = property_editor_new (G_OBJECT (actor), "width");
    clutter_actor_set_size (editor, 40, EDITOR_LINE_HEIGHT);
    clutter_container_add_actor (CLUTTER_CONTAINER (hbox), editor);

    editor = property_editor_new (G_OBJECT (actor), "height");
    clutter_actor_set_size (editor, 40, EDITOR_LINE_HEIGHT);
    clutter_container_add_actor (CLUTTER_CONTAINER (hbox), editor);

    clutter_container_add_actor (CLUTTER_CONTAINER (container), hbox);

    /* virtual 'id' property */

    hbox = g_object_new (NBTK_TYPE_BOX_LAYOUT, NULL);
    label = clutter_text_new_with_text (CS_EDITOR_LABEL_FONT, "id");

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


    clutter_actor_set_size (label, CS_PROPEDITOR_LABEL_WIDTH,
                                   EDITOR_LINE_HEIGHT);
    clutter_actor_set_size (editor, CS_PROPEDITOR_EDITOR_WIDTH,
                                   EDITOR_LINE_HEIGHT);
    clutter_container_add_actor (CLUTTER_CONTAINER (container), hbox);
}


void cs_set_active (ClutterActor *item)
{
  if (item == NULL)
    item = clutter_stage_get_default ();
  if (item)
    {
      clutter_text_set_text (CLUTTER_TEXT (name), G_OBJECT_TYPE_NAME (item));
      clutter_text_set_text (CLUTTER_TEXT (name2), G_OBJECT_TYPE_NAME (item));
    }

  cs_container_remove_children (parents);
  cs_container_remove_children (cluttersmith->property_editors);
  cs_container_remove_children (cluttersmith->scene_graph);

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
              g_signal_connect (new, "clicked", G_CALLBACK (cs_set_active_event), iter);
              clutter_container_add_actor (CLUTTER_CONTAINER (parents), new);
              iter = clutter_actor_get_parent (iter);
            }

          g_object_weak_ref (G_OBJECT (item), selected_vanished, NULL);

          actor_defaults_populate (cluttersmith->property_editors, active_actor);
          props_populate (cluttersmith->property_editors, G_OBJECT (active_actor));
          cs_tree_populate (cluttersmith->scene_graph, active_actor);

          props_populate (cluttersmith->active_container, G_OBJECT (active_actor));

        }
    }
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


static void change_type2 (ClutterActor *button,
                          const gchar  *name)
{
  ClutterActor *actor = cs_selected_get_any ();
  popup_close ();
  cs_container_remove_children (cluttersmith->property_editors);
  if (!actor)
    {
      return;
    }
  actor = cs_actor_change_type (actor, name);

  if (g_str_equal (name, "ClutterText"))
    {
      g_object_set (G_OBJECT (actor), "text", "New Text", NULL);
    }

  cs_selected_clear ();
  cs_selected_add (actor);
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
  g_signal_connect (button, "clicked", G_CALLBACK (change_type2), name);
}


void cs_change_type (ClutterActor *actor)
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

static void cs_save (void)
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

gboolean cs_save_timeout (gpointer data)
{
  cs_save ();
  return TRUE;
}

static void title_text_changed (ClutterActor *actor)
{
  const gchar *title = clutter_text_get_text (CLUTTER_TEXT (actor));

  cs_save ();

  filename = g_strdup_printf ("%s/%s.json", cs_get_project_root(),
                              title);
  cs_container_remove_children (cluttersmith->property_editors);
  cs_container_remove_children (cluttersmith->scene_graph);
  if (g_file_test (filename, G_FILE_TEST_IS_REGULAR))
    {
      gchar *scriptfilename;
      session_history_add (cs_get_project_root ());
      cluttersmith->fake_stage = cs_replace_content2 (actor, "fake-stage", filename);

      scriptfilename = g_strdup_printf ("%s/%s.js", cs_get_project_root(),
                                  title);
      if (g_file_test (scriptfilename, G_FILE_TEST_IS_REGULAR))
        {
          GError      *error = NULL;
          guchar *js;
          gchar *code = JS_PREAMBLE; 
          gsize len;

          len = strlen (code);

          cluttersmith->priv->page_js_context = gjs_context_new_with_search_path(NULL);
          g_object_set_data_full (G_OBJECT (cluttersmith->fake_stage),
               "js-context", cluttersmith->priv->page_js_context, (void*)g_object_unref);
          gjs_context_eval(cluttersmith->priv->page_js_context, (void*)code, len,
      "<code>", NULL, NULL);
          if (!g_file_get_contents (scriptfilename, (void*)&js, &len, &error))
            {
               g_printerr("failed loading file %s: %s\n", scriptfilename, error->message);
            }
          else
            {
              gint code;

              clutter_text_set_text (CLUTTER_TEXT(cluttersmith->dialog_editor), (gchar*)js);
              g_print ("running js: %i {%s}\n", len, js);

              if (!gjs_context_eval(cluttersmith->priv->page_js_context, (void*)js, len,
                       "<code>", &code, &error))
                {
                   g_printerr("Failed: %s\n", error->message);
                }
              else
                {
                  g_print ("returned: %i\n", code);
                }
              g_free (js);
            }
          g_free (scriptfilename);
        }
    }
  else
    {
      cluttersmith->fake_stage = cs_replace_content2 (actor, "fake-stage", NULL);

    }
  cs_set_current_container (cluttersmith->fake_stage);
  cs_sync_chrome_idle (NULL);
  CS_REVISION = CS_STORED_REVISION = 0;

  cs_selected_clear ();
}

char *
cs_make_config_file (const char *filename)
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

static void dialog_remember (GKeyFile *keyfile,
                            const gchar *name,
                            ClutterActor *actor)
{
  if (!actor)
    return;
 g_key_file_set_boolean (keyfile, name, "visible", CLUTTER_ACTOR_IS_VISIBLE(actor));
 g_key_file_set_integer (keyfile, name, "x", clutter_actor_get_x (actor));
 g_key_file_set_integer (keyfile, name, "y", clutter_actor_get_y (actor));
}


static void dialog_recall (GKeyFile *keyfile,
                           const gchar *name,
                           ClutterActor *actor)
{
  if (!actor)
    return;
  if (g_key_file_get_boolean (keyfile, name, "visible", NULL))
    clutter_actor_show (actor);
  else
    clutter_actor_hide (actor);

  clutter_actor_set_position (actor, g_key_file_get_integer (keyfile, name, "x", NULL),
                                     g_key_file_get_integer (keyfile, name, "y", NULL));
}


void cs_save_dialog_state (void)
{
  gchar *config_path = cs_make_config_file ("dialog-state");
  gchar *config;
  
  GKeyFile *keyfile;
  keyfile = g_key_file_new ();

  dialog_remember (keyfile, "cs-dialog-toolbar", cluttersmith->dialog_toolbar);
  dialog_remember (keyfile, "cs-dialog-tree", cluttersmith->dialog_tree);
  dialog_remember (keyfile, "cs-dialog-property-inspector", cluttersmith->dialog_property_inspector);

  dialog_remember (keyfile, "cs-dialog-templates", cluttersmith->dialog_templates);
  dialog_remember (keyfile, "cs-dialog-editor", cluttersmith->dialog_editor);

  dialog_remember (keyfile, "cs-dialog-scenes", cluttersmith->dialog_scenes);

  dialog_remember (keyfile, "cs-dialog-config", cluttersmith->dialog_config);

  {
    gfloat w,h;
    clutter_actor_get_size (clutter_stage_get_default (), &w, &h);
    g_key_file_set_integer (keyfile, "window", "width", w);
    g_key_file_set_integer (keyfile, "window", "height", h);
  }

  {
    gfloat w,h;
    clutter_actor_get_size (cluttersmith->fake_stage_canvas, &w, &h);
    g_key_file_set_integer (keyfile, "canvas", "width", w);
    g_key_file_set_integer (keyfile, "canvas", "height", h);
  }

  {
    gfloat zoom, x, y;
    g_object_get (G_OBJECT (cluttersmith), "origin-x", &x, "origin-y", &y, "zoom", &zoom, NULL);

    g_key_file_set_double (keyfile, "canvas", "origin-x", x);
    g_key_file_set_double (keyfile, "canvas", "origin-y", y);
    g_key_file_set_double (keyfile, "canvas", "zoom", zoom);
  }

  config = g_key_file_to_data (keyfile, NULL, NULL);
  g_file_set_contents (config_path, config, -1, NULL);
  g_key_file_free (keyfile);
  g_free (config);
  g_free (config_path);
}


void cs_load_dialog_state (void)
{
  gchar *config_path = cs_make_config_file ("dialog-state");
  
  GKeyFile *keyfile;
  keyfile = g_key_file_new ();
  g_key_file_load_from_file (keyfile, config_path, 0, NULL);

  dialog_recall (keyfile, "cs-dialog-toolbar", cluttersmith->dialog_toolbar);
  dialog_recall (keyfile, "cs-dialog-tree", cluttersmith->dialog_tree);
  dialog_recall (keyfile, "cs-dialog-property-inspector", cluttersmith->dialog_property_inspector);

  dialog_recall (keyfile, "cs-dialog-templates", cluttersmith->dialog_templates);
  dialog_recall (keyfile, "cs-dialog-editor", cluttersmith->dialog_editor);

  dialog_recall (keyfile, "cs-dialog-scenes", cluttersmith->dialog_scenes);

  dialog_recall (keyfile, "cs-dialog-config", cluttersmith->dialog_config);

  {
    ClutterActor *stage = clutter_actor_get_stage (cluttersmith->parasite_root);
    gint w, h;

    w = g_key_file_get_integer (keyfile, "window", "width", NULL);
    h = g_key_file_get_integer (keyfile, "window", "height", NULL);
    if (w > 10 && h > 10)
      {
        clutter_actor_set_size (stage, w, h);
      }
    w = g_key_file_get_integer (keyfile, "canvas", "width", NULL);
    h = g_key_file_get_integer (keyfile, "canvas", "height", NULL);
    if (w > 10 && h > 10)
      {
        g_object_set (G_OBJECT (cluttersmith), "canvas-width", w, "canvas-height", h, NULL);
      }
  }

  {
    gfloat zoom, x, y;

    x = g_key_file_get_double (keyfile, "canvas", "origin-x", NULL);
    y = g_key_file_get_double (keyfile, "canvas", "origin-y", NULL);
    zoom = g_key_file_get_double (keyfile, "canvas", "zoom", NULL);
    if (zoom > 1.0)
    g_object_set (G_OBJECT (cluttersmith), "origin-x", x, "origin-y", y, "zoom", zoom, NULL);
  }

  g_key_file_free (keyfile);
  g_free (config_path);
}

void session_history_add (const gchar *dir)
{
  gchar *config_path = cs_make_config_file ("session-history");
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



void cs_set_project_root (const gchar *new_root)
{
  if (cluttersmith->project_root &&
      g_str_equal (cluttersmith->project_root, new_root))
    {
      return;
    }

  g_object_set (cs_find_by_id_int (clutter_actor_get_stage (cluttersmith->parasite_root), "project-root"),
                "text", new_root, NULL);
}

gchar *cs_get_project_root (void)
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
      previews_reload (cs_find_by_id_int (clutter_actor_get_stage(actor), "previews-container"));
      cs_open_layout ("index");
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
