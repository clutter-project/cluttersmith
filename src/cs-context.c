/* cluttersmith-context.c */

#include "cluttersmith.h"
#include "cs-context.h"
#include "clutter-states.h"
#include "animator-editor.h"
#include <gjs/gjs.h>
#include <gio/gio.h>
#include <string.h>

  G_DEFINE_TYPE (CSContext, cs_context, G_TYPE_OBJECT)

#define CONTEXT_PRIVATE(o) \
    (G_TYPE_INSTANCE_GET_PRIVATE ((o), CS_TYPE_CONTEXT, CSContextPrivate))

static gint cs_set_keys_freeze = 0;

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
  gchar  *title;
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
  pspec = g_param_spec_float ("origin-x", "origin X", "origin X coordinate", -20000, 20000, 0, G_PARAM_READWRITE|G_PARAM_CONSTRUCT);
  g_object_class_install_property (object_class, PROP_ORIGIN_X, pspec);

  pspec = g_param_spec_float ("origin-y", "origin Y", "origin Y coordinate", -20000, 20000, 0, G_PARAM_READWRITE|G_PARAM_CONSTRUCT);
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



#include <mx/mx.h>
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
                          "MxWidget",
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
  if (!cluttersmith->parasite_ui)
    return;
  clutter_actor_get_transformed_position (cs_find_by_id_int (clutter_actor_get_stage (cluttersmith->parasite_root), "fake-stage-rect"), &x, &y);

  /* move fake_stage and canvas into a single group that is transformed? */
#if 1
  clutter_actor_set_position (cluttersmith->fake_stage, 
    x-cluttersmith->priv->origin_x,
    y-cluttersmith->priv->origin_y);
  clutter_actor_set_position (cluttersmith->fake_stage_canvas, 
    x-cluttersmith->priv->origin_x,
    y-cluttersmith->priv->origin_y);
#else
  clutter_actor_animate (cluttersmith->fake_stage_canvas, CLUTTER_LINEAR, 100,
    "x", x-cluttersmith->priv->origin_x, "y", y-cluttersmith->priv->origin_y, NULL);
  clutter_actor_animate (cluttersmith->fake_stage, CLUTTER_LINEAR, 100,
    "x", x-cluttersmith->priv->origin_x, "y", y-cluttersmith->priv->origin_y, NULL);
#endif


  clutter_actor_show (cluttersmith->parasite_ui);
  clutter_actor_set_scale (cluttersmith->fake_stage, cluttersmith->priv->zoom/100.0,
                                                     cluttersmith->priv->zoom/100.0);

  clutter_actor_set_scale (cluttersmith->fake_stage_canvas, cluttersmith->priv->zoom/100.0,
                                                     cluttersmith->priv->zoom/100.0);

  has_chrome = TRUE;
}

gfloat cluttersmith_get_origin_x (void)
{
  return cluttersmith->priv->origin_x;
}

gfloat cluttersmith_get_origin_y (void)
{
  return cluttersmith->priv->origin_y;
}

static void cluttsmith_hide_chrome (void)
{
  gfloat x, y;
  if (!cluttersmith->parasite_ui)
    return;
  clutter_actor_hide (cluttersmith->parasite_ui);
  clutter_actor_get_transformed_position (cs_find_by_id_int (clutter_actor_get_stage (cluttersmith->parasite_root), "fake-stage-rect"), &x, &y);

#if 1
  clutter_actor_set_position (cluttersmith->fake_stage, 
    -cluttersmith->priv->origin_x,
    -cluttersmith->priv->origin_y);
  clutter_actor_set_position (cluttersmith->fake_stage_canvas, 
    -cluttersmith->priv->origin_x,
    -cluttersmith->priv->origin_y);
#else
  clutter_actor_animate (cluttersmith->fake_stage, CLUTTER_LINEAR, 100,
    "x", -cluttersmith->priv->origin_x,
    "y", -cluttersmith->priv->origin_y, NULL);
  clutter_actor_animate (cluttersmith->fake_stage_canvas, CLUTTER_LINEAR, 100,
    "x", -cluttersmith->priv->origin_x,
    "y", -cluttersmith->priv->origin_y, NULL);
#endif

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
  if (!cluttersmith->fake_stage_canvas)
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

static gboolean return_to_ui (gpointer ignored)
{
  cs_set_ui_mode (CS_UI_MODE_CHROME);
  return FALSE;
}

static void save_annotation (void)
{
  const gchar *annotation= clutter_text_get_text (CLUTTER_TEXT (cluttersmith->dialog_editor_annotation));
  gchar *annotationfilename;
  gint len;
  annotationfilename = g_strdup_printf ("%s/%s.txt", cs_get_project_root(),
                              cluttersmith->priv->title);
  len = strlen (annotation);
  if (len >= 6)
    {
      g_file_set_contents (annotationfilename, annotation, len, NULL);
      g_print ("set %s %s\n", annotationfilename, annotation);
    }
  g_free (annotationfilename);
}

static void page_run_start (void)
{
  gchar *scriptfilename;
  gchar *annotationfilename;
  g_print ("entering browse mode\n");
  scriptfilename = g_strdup_printf ("%s/%s.js", cs_get_project_root(),
                              cluttersmith->priv->title);
  annotationfilename = g_strdup_printf ("%s/%s.txt", cs_get_project_root(),
                              cluttersmith->priv->title);



  save_annotation ();

  if (cluttersmith->dialog_editor_text)
  {
    const gchar *text = clutter_text_get_text (CLUTTER_TEXT (cluttersmith->dialog_editor_text));
    GString *new = g_string_new ("");

    gint len = strlen (text);
    if (len >= 6 || 1)
      {
        GList *actors, *a;
        actors = cs_container_get_children_recursive (cluttersmith_get_stage ());
      
        for (a = actors; a; a = a->next)
          {
              GHashTable *ht;
              const gchar *id = clutter_scriptable_get_id (a->data);
              ht = g_object_get_data (G_OBJECT (a->data), "callbacks");
              if (ht)
                {
                  GHashTableIter iter;
                  gpointer key, value;
                  g_hash_table_iter_init (&iter, ht);
                  while (g_hash_table_iter_next (&iter, &key, &value))
                    {
                      GList *cbs;
                      for (cbs = value; cbs; cbs=cbs->next)
                        {
                            /* the protoype for the callback isnt correct, and
                             * should be generated based on the signal_query
                             */
                            g_string_append_printf (new, "/*[CS] %s:%s */  $('%s').connect('%s', function(o,event) {\n", id, (gchar*)key, id, (gchar*)key);
                            g_string_append_printf (new, "%s});/*[CS]*/\n", (gchar*)cbs->data);
                          }
                      }
                  }
            }

          g_list_free (actors);
          g_string_append_printf (new, "%s", text);
          len = strlen (new->str);
          g_file_set_contents (scriptfilename, new->str, len, NULL);
          g_string_free (new, TRUE);
        }
    }

    if (g_file_test (scriptfilename, G_FILE_TEST_IS_REGULAR))
      {
        GError      *error = NULL;
        guchar *js;
        gchar *code = JS_PREAMBLE; 
        gsize len;

        len = strlen (code);

        g_assert (cluttersmith->priv->page_js_context == NULL);
        cluttersmith->priv->page_js_context = gjs_context_new_with_search_path(NULL);
        gjs_context_eval(cluttersmith->priv->page_js_context, (void*)code, len,
    "<code>", NULL, NULL);
        if (!g_file_get_contents (scriptfilename, (void*)&js, &len, &error))
          {
             g_printerr("failed loading file %s: %s\n", scriptfilename, error->message);
          }
        else
          {
            gint code;
            if (!gjs_context_eval(cluttersmith->priv->page_js_context, (void*)js, len,
                     "<code>", &code, &error))
              {
                if (cluttersmith->dialog_editor_error)  
                 clutter_text_set_text (CLUTTER_TEXT(cluttersmith->dialog_editor_error),
                                        error->message);
                else
                  g_print (error->message);
                 g_idle_add (return_to_ui, NULL);
              }
            else
              {
                gchar *str = g_strdup_printf ("returned: %i\n", code);
                if (cluttersmith->dialog_editor_error)  
                  clutter_text_set_text (CLUTTER_TEXT(cluttersmith->dialog_editor_error),
                                       str);
                g_free (str);
              }
            g_free (js);
          }
        g_free (scriptfilename);
      }
  }

  static void page_run_end (void)
  {
    if (cluttersmith->priv->page_js_context)
      {
        g_object_unref (cluttersmith->priv->page_js_context);
        cluttersmith->priv->page_js_context = NULL;
      }

  }


static void browse_start (void)
{
  page_run_end ();
  cs_save (TRUE);

  /* XXX: apply signal handlers */
  page_run_start ();
}

static void cs_load (void);

static void browse_end (void)
{
  page_run_end ();
  g_print ("leaving browse mode\n");
  cs_load (); /* reverting back to saved state */
}

void cs_set_ui_mode (guint ui_mode)
{

  if (!(ui_mode & CS_UI_MODE_EDIT) &&
      (cluttersmith->ui_mode & CS_UI_MODE_EDIT))
    {
      browse_start ();
    }
  else if ((ui_mode & CS_UI_MODE_EDIT) &&
           !(cluttersmith->ui_mode & CS_UI_MODE_EDIT))
    {
      browse_end ();
    }

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

/**
 * cluttersmith_load_scene:
 * @new_title: new scene
 */
void cluttersmith_load_scene (const gchar *new_title)
{
  if (title)
    {
      g_object_set (title, "text", new_title, NULL);
    }
  else
    {
    }
}

static gboolean
runtime_capture (ClutterActor *actor,
                 ClutterEvent *event,
                 gpointer      data /* unused */)
{
    {
      /* check if it is child of a link, if it is then we override anyways...
       *
       * Otherwise this is the case that slips control through to the
       * underlying scene graph.
       */
      switch (event->any.type)
        {
          case CLUTTER_BUTTON_PRESS:
              if (event->any.type == CLUTTER_BUTTON_PRESS &&
                  clutter_event_get_button (event)==1)
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
                           cluttersmith_load_scene (name+5);
                           cs_selected_clear ();
                           return TRUE;
                         }
                       hit = clutter_actor_get_parent (hit);
                     }
                } 
          case CLUTTER_BUTTON_RELEASE:
              cs_last_x = event->button.x;
              cs_last_y = event->button.y;
              break;
          case CLUTTER_MOTION:
              cs_last_x = event->motion.x;
              cs_last_y = event->motion.y;
              break;
          default:
              break;
        }
      return FALSE;
    }
  return FALSE;
}


/**
 * cluttersmith_init:
 */
void cluttersmith_init (void)
{
  ClutterScript *script = clutter_script_new ();
  ClutterActor *actor;
  ClutterActor *stage;
  g_print ("initializing\n");

  cluttersmith = cs_context_new ();
  clutter_script_load_from_data (script, "{'id':'actor','type':'ClutterGroup','children':[{'id':'fake-stage','type':'ClutterGroup'},{'id':'parasite-root','type':'ClutterGroup'},{'id':'title','opacity':1,'type':'MxEntry','signals': [ {'name':'paint', 'handler':'search_entry_init_hack'} ]},{'id':'project-root','type':'MxEntry','opacity':1,'y':50,'signals': [ {'name':'paint', 'handler':'project_root_init_hack'} ]}]}", -1, NULL);

  stage = clutter_stage_get_default();
  actor = CLUTTER_ACTOR (clutter_script_get_object (script, "actor"));
  clutter_container_add_actor (CLUTTER_CONTAINER (stage), actor);

  clutter_script_connect_signals (script, script);
  g_object_set_data_full (G_OBJECT (actor), "clutter-script", script, g_object_unref);
  cluttersmith->parasite_root = CLUTTER_ACTOR (clutter_script_get_object (script, "parasite-root"));
  /* initializing globals */
  title = CLUTTER_ACTOR (clutter_script_get_object (script, "title"));

  g_signal_connect (stage, "captured-event", G_CALLBACK (runtime_capture), NULL);

  cs_set_ui_mode (CS_UI_MODE_BROWSE);
  clutter_actor_show (stage);
  clutter_actor_paint (stage);
}


void mode_browse (void);
void mode_annotate (void);
void mode_sketch (void);
void mode_edit (void);
void mode_animate (void);
void mode_callbacks (void);
void mode_code (void);
void mode_config (void);
void mode_export (void);

void state_position_actors (gdouble        progress);

void cs_animator_progress_changed (GObject    *slider,
                                   GParamSpec *pspec,
                                   gpointer    data)
{
  gdouble progress;
  g_object_get (slider, "progress", &progress, NULL);
  state_position_actors (progress);
}

void mode_switch (MxComboBox *combo_box,
                  GParamSpec *pspec,
                  gpointer    data)
{
  gint index = mx_combo_box_get_index (combo_box);

  switch (index)
    {
      case 0: mode_browse (); break;
      case 1: mode_annotate (); break;
      case 2: mode_sketch (); break;
      case 3: mode_edit (); break;
      case 4: mode_animate (); break;
      case 5: mode_callbacks (); break;
      case 6: mode_code (); break;
      case 7: mode_export (); break;
      case 8: mode_config (); break;
      default:
              break;
    }
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
    mx_style_load_from_file (mx_style_get_default (), PKGDATADIR "cluttersmith.css", NULL);
    script = cs_get_script (actor);

    /* initializing globals */
    title = CLUTTER_ACTOR (clutter_script_get_object (script, "title"));
    name = CLUTTER_ACTOR (clutter_script_get_object (script, "cs-type"));
    name2 = CLUTTER_ACTOR (clutter_script_get_object (script, "cs-type2"));
    parents = CLUTTER_ACTOR (clutter_script_get_object (script, "parents"));
    cluttersmith->property_editors = CLUTTER_ACTOR (clutter_script_get_object (script, "property-editors"));
    cluttersmith->property_editors_core = CLUTTER_ACTOR (clutter_script_get_object (script, "property-editors-core"));
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

#define _A(actorname)  CLUTTER_ACTOR (clutter_script_get_object (script, actorname))

    /* Should perhaps replace all occurances of this with the macro, it
     * makes the code a bit smaller and more maintainable since the mappings
     * from names to struct items wouldn't have to be maintained anymore
     */

    cluttersmith->fake_stage_canvas = _A("fake-stage-canvas");
    cluttersmith->resize_handle = _A("resize-handle");
    cluttersmith->move_handle = _A("move-handle");
    cluttersmith->active_panel = _A("cs-active-panel");
    cluttersmith->active_container = _A("cs-active-container");
    cluttersmith->callbacks_container = _A("cs-callbacks-container");
    cluttersmith->dialog_callbacks = _A("cs-dialog-callbacks");
    cluttersmith->states_container = _A("cs-states-container");
    cluttersmith->dialog_states = _A("cs-dialog-states");

    cluttersmith->dialog_config = _A("cs-dialog-config");
    cluttersmith->dialog_tree = _A("cs-dialog-tree");
    cluttersmith->dialog_toolbar = _A("cs-dialog-toolbar");
    cluttersmith->dialog_scenes = _A("cs-dialog-scenes");
    cluttersmith->dialog_export = _A("cs-dialog-export");
    cluttersmith->dialog_templates = _A("cs-dialog-templates");
    cluttersmith->dialog_animator = _A("cs-dialog-animator");
    cluttersmith->animator_progress = _A("cs-animator-progress");
    cluttersmith->animator_props = _A("cs-animator-props");
    cluttersmith->state_duration = _A("cs-state-duration");
    cluttersmith->dialog_editor = _A("cs-dialog-editor");
    cluttersmith->dialog_type = _A("cs-dialog-type");
    cluttersmith->dialog_annotate = _A("cs-dialog-annotate");
    cluttersmith->dialog_editor_annotation = _A("cs-dialog-editor-annotation");
    cluttersmith->source_state = _A("cs-source-state");
    cluttersmith->dialog_editor_text = _A("cs-dialog-editor-text");
    cluttersmith->dialog_editor_error = _A("cs-dialog-editor-error");

    cluttersmith->dialog_property_inspector = _A("cs-dialog-property-inspector");
    cluttersmith->animator_editor = _A("cs-animator-editor");

    cs_manipulate_init (cluttersmith->parasite_root);
    cs_set_active (clutter_actor_get_stage(cluttersmith->parasite_root));


    props_populate (_A("config-editors"), G_OBJECT (cluttersmith), FALSE);

    {
      /* XXX: all of this should be in the json */
      cluttersmith->cs_mode = CLUTTER_ACTOR (clutter_script_get_object (script, "cs-mode"));

      mx_combo_box_append_text (MX_COMBO_BOX (cluttersmith->cs_mode), "Browse (F1)");
      mx_combo_box_append_text (MX_COMBO_BOX (cluttersmith->cs_mode), "Annotate (F2)");
      mx_combo_box_append_text (MX_COMBO_BOX (cluttersmith->cs_mode), "Sketch (F3)");
      mx_combo_box_append_text (MX_COMBO_BOX (cluttersmith->cs_mode), "Edit (F4)");
      mx_combo_box_append_text (MX_COMBO_BOX (cluttersmith->cs_mode), "Animate (F5)");
      mx_combo_box_append_text (MX_COMBO_BOX (cluttersmith->cs_mode), "Callbacks (F6)");
      mx_combo_box_append_text (MX_COMBO_BOX (cluttersmith->cs_mode), "Code (F7)");
      mx_combo_box_append_text (MX_COMBO_BOX (cluttersmith->cs_mode), "Export (F8)");
      mx_combo_box_append_text (MX_COMBO_BOX (cluttersmith->cs_mode), "Config (F9)");

      mx_combo_box_set_index (MX_COMBO_BOX (cluttersmith->cs_mode), 0);

      g_signal_connect (cluttersmith->cs_mode, "notify::index", G_CALLBACK (mode_switch), NULL);
    }

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
    ClutterActor *hbox;
    ClutterActor *label;
    ClutterActor *editor; 

    /* special casing of x,y,w,h to make it take up less space and always be first */

    hbox = g_object_new (MX_TYPE_BOX_LAYOUT, "spacing", 5, NULL);
    label = clutter_text_new_with_text (CS_EDITOR_LABEL_FONT, "x");
    clutter_text_set_color (CLUTTER_TEXT (label), &white);
    clutter_container_add_actor (CLUTTER_CONTAINER (hbox), label);
    clutter_container_child_set (CLUTTER_CONTAINER (hbox), label, "expand", TRUE, NULL);
    label = clutter_text_new_with_text (CS_EDITOR_LABEL_FONT, "y");
    clutter_text_set_color (CLUTTER_TEXT (label), &white);
    clutter_container_add_actor (CLUTTER_CONTAINER (hbox), label);
    clutter_container_child_set (CLUTTER_CONTAINER (hbox), label, "expand", TRUE, NULL);
    clutter_container_add_actor (CLUTTER_CONTAINER (container), hbox);

    hbox = g_object_new (MX_TYPE_BOX_LAYOUT, "spacing", 5, NULL);
    editor = property_editor_new (G_OBJECT (actor), "x");
    clutter_container_add_actor (CLUTTER_CONTAINER (hbox), editor);
    clutter_container_child_set (CLUTTER_CONTAINER (hbox), editor, "expand", TRUE, NULL);
    editor = property_editor_new (G_OBJECT (actor), "y");
    clutter_container_add_actor (CLUTTER_CONTAINER (hbox), editor);
    clutter_container_child_set (CLUTTER_CONTAINER (hbox), editor, "expand", TRUE, NULL);
    clutter_container_add_actor (CLUTTER_CONTAINER (container), hbox);

    hbox = g_object_new (MX_TYPE_BOX_LAYOUT, "spacing", 5, NULL);
    label = clutter_text_new_with_text (CS_EDITOR_LABEL_FONT, "width");
    clutter_text_set_color (CLUTTER_TEXT (label), &white);
    clutter_container_add_actor (CLUTTER_CONTAINER (hbox), label);
    clutter_container_child_set (CLUTTER_CONTAINER (hbox), label, "expand", TRUE, NULL);
    label = clutter_text_new_with_text (CS_EDITOR_LABEL_FONT, "height");
    clutter_text_set_color (CLUTTER_TEXT (label), &white);
    clutter_container_add_actor (CLUTTER_CONTAINER (hbox), label);
    clutter_container_child_set (CLUTTER_CONTAINER (hbox), label, "expand", TRUE, NULL);
    clutter_container_add_actor (CLUTTER_CONTAINER (container), hbox);

    hbox = g_object_new (MX_TYPE_BOX_LAYOUT, "spacing", 5, NULL);
    editor = property_editor_new (G_OBJECT (actor), "width");
    clutter_container_add_actor (CLUTTER_CONTAINER (hbox), editor);
    clutter_container_child_set (CLUTTER_CONTAINER (hbox), editor, "expand", TRUE, NULL);
    editor = property_editor_new (G_OBJECT (actor), "height");
    clutter_container_add_actor (CLUTTER_CONTAINER (hbox), editor);
    clutter_container_child_set (CLUTTER_CONTAINER (hbox), editor, "expand", TRUE, NULL);

    clutter_container_add_actor (CLUTTER_CONTAINER (container), hbox);

    /* virtual 'id' property */
    label = clutter_text_new_with_text (CS_EDITOR_LABEL_FONT, "id");

    {
      editor = CLUTTER_ACTOR (mx_entry_new (""));
      g_signal_connect (mx_entry_get_clutter_text (
                             MX_ENTRY (editor)), "text-changed",
                             G_CALLBACK (update_id), actor);
      mx_entry_set_text (MX_ENTRY (editor), clutter_scriptable_get_id (CLUTTER_SCRIPTABLE (actor)));
    }

    clutter_container_add_actor (CLUTTER_CONTAINER (container), label);
    clutter_container_add_actor (CLUTTER_CONTAINER (container), editor);
    clutter_text_set_color (CLUTTER_TEXT (label), &white);
}

static GList *cs_actor_get_js_callbacks (ClutterActor *actor,
                                         const gchar  *signal)
{
  GHashTable *ht;
  ht = g_object_get_data (G_OBJECT (actor), "callbacks");
  if (!ht)
    return NULL;
  return g_hash_table_lookup (ht, signal);
}

static void
callback_text_changed (ClutterText  *text,
                       ClutterActor *actor)
{
  const gchar *new_text = clutter_text_get_text (text);
  GList *c, *callbacks;
  GHashTable *ht;
  gint i;
  const gchar *signal = g_object_get_data (G_OBJECT (text), "signal");
  gint no = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (text), "no"));

  ht = g_object_get_data (G_OBJECT (actor), "callbacks");
  callbacks = g_hash_table_lookup (ht, signal);
  for (i=0, c=callbacks; c; c=c->next, i++)
    {
      if (i==no)
        {
          g_free (c->data);
          c->data = g_strdup (new_text);
        }
    }
}


static void freecode (gpointer ht)
{
  GHashTableIter iter;
  gpointer key, value;

  g_hash_table_iter_init (&iter, ht);
  while (g_hash_table_iter_next (&iter, &key, &value)) 
    {
      g_list_foreach (value, (GFunc)g_free, NULL);
      g_list_free (value);
    }
  g_hash_table_destroy (ht);
}

static void
callbacks_populate (ClutterActor *actor);

static void
callback_add (MxWidget *button,
              ClutterActor *actor)
{
  GList *callbacks;
  GHashTable *ht;
  gchar *signal = g_object_get_data (G_OBJECT (button), "signal");

  ht = g_object_get_data (G_OBJECT (actor), "callbacks");
  if (!ht)
    {
      ht = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
      g_object_set_data_full (G_OBJECT (actor), "callbacks", ht, freecode);
    }

  callbacks = g_hash_table_lookup (ht, signal);
  callbacks = g_list_append (callbacks, g_strdup ("hoi"));
  g_hash_table_insert (ht, g_strdup (signal), callbacks);
  callbacks_populate (actor);
}

static void
callbacks_add_cb (ClutterActor *actor,
                  const gchar  *signal,
                  const gchar  *code,
                  gint          no)
{
  ClutterActor *cb = clutter_text_new_with_text ("Mono 10", code);

  ClutterActor *hbox   = g_object_new (MX_TYPE_BOX_LAYOUT, NULL);
  ClutterActor *remove = mx_button_new_with_label ("-");
  clutter_container_add (CLUTTER_CONTAINER (hbox), CLUTTER_ACTOR (remove),
                                                                   CLUTTER_ACTOR (cb), NULL);
  clutter_container_add_actor (CLUTTER_CONTAINER (cluttersmith->callbacks_container), hbox);
  g_object_set (G_OBJECT (cb), "editable", TRUE, "selectable", TRUE, "reactive", TRUE, NULL);
  g_object_set_data (G_OBJECT (cb), "no", GINT_TO_POINTER (no));
  g_object_set_data_full (G_OBJECT (cb), "signal", g_strdup (signal), g_free);
  g_signal_connect (cb, "text-changed", G_CALLBACK (callback_text_changed), actor);

  clutter_container_add_actor (CLUTTER_CONTAINER (cluttersmith->callbacks_container), hbox);
}

static void
callbacks_populate (ClutterActor *actor)
{
  GType type = G_OBJECT_TYPE (actor);

  cs_container_remove_children (cluttersmith->callbacks_container);

  while (type)
  {
    guint *list;
    guint count;
    guint i;


    list = g_signal_list_ids (type, &count);
    
    for (i=0; i<count;i++)
      {
        GSignalQuery query;
        g_signal_query (list[i], &query);

        if (type == G_TYPE_OBJECT)
          continue;
        if (type == CLUTTER_TYPE_ACTOR)
          {
            gchar *white_list[]={"show", "motion-event", "enter-event", "leave-event", "button-press-event", "button-release-event", NULL};
            gint i;
            gboolean skip = TRUE;
            for (i=0;white_list[i];i++)
              if (g_str_equal (white_list[i], query.signal_name))
                skip = FALSE;
            if (skip)
              continue;
          }


          {
            ClutterActor *hbox  = g_object_new (MX_TYPE_BOX_LAYOUT, NULL);
            ClutterActor *title = mx_label_new (query.signal_name);
            ClutterActor *add   = mx_button_new_with_label ("+");

            g_object_set_data_full (G_OBJECT (add), "signal", g_strdup (query.signal_name), g_free);
            g_signal_connect (add, "clicked", G_CALLBACK (callback_add), actor);

            clutter_container_add (CLUTTER_CONTAINER (hbox), CLUTTER_ACTOR (add),
                                                             CLUTTER_ACTOR (title), NULL);
            clutter_container_add_actor (CLUTTER_CONTAINER (cluttersmith->callbacks_container), hbox);
            clutter_container_child_set (CLUTTER_CONTAINER (cluttersmith->callbacks_container),
                                         CLUTTER_ACTOR (title),
                                         "expand", TRUE,
                                         NULL);

            {
              gint no = 0;
              GList *l, *list = cs_actor_get_js_callbacks (actor, query.signal_name);
              for (l=list;l;l=l->next, no++)
                {
                  callbacks_add_cb (actor, query.signal_name, l->data, no);
                }
            }
          }
      }
    type = g_type_parent (type);
    g_free (list);
  }
}

void cs_set_active (ClutterActor *item)
{
  if (!name || !name2)
    return;
  if (item == NULL)
    item = clutter_stage_get_default ();
  if (item)
    {
      clutter_text_set_text (CLUTTER_TEXT (name), G_OBJECT_TYPE_NAME (item));
      clutter_text_set_text (CLUTTER_TEXT (name2), G_OBJECT_TYPE_NAME (item));
    }

  cs_container_remove_children (parents);
  cs_container_remove_children (cluttersmith->property_editors);
  cs_container_remove_children (cluttersmith->property_editors_core);
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
              new = CLUTTER_ACTOR (mx_button_new_with_label (G_OBJECT_TYPE_NAME (iter)));
              g_signal_connect (new, "clicked", G_CALLBACK (cs_set_active_event), iter);
              clutter_container_add_actor (CLUTTER_CONTAINER (parents), new);

              if (iter == cluttersmith->fake_stage)
                iter = NULL;
              else
                iter = clutter_actor_get_parent (iter);
            }

          g_object_weak_ref (G_OBJECT (item), selected_vanished, NULL);

          actor_defaults_populate (cluttersmith->property_editors_core, active_actor);
          props_populate (cluttersmith->property_editors, G_OBJECT (active_actor), FALSE);
          cs_tree_populate (cluttersmith->scene_graph, active_actor);

          if(0)props_populate (cluttersmith->active_container, G_OBJECT (active_actor), FALSE);

          callbacks_populate (active_actor);

        }
    }
}

gchar *json_serialize_subtree (ClutterActor *root);

void session_history_add (const gchar *dir);

static gchar *filename = NULL;

void cs_save (gboolean force)
{
  /* Save if we've changed */
  if (filename)
    {
      GFile *gf = g_file_new_for_path (filename);
      GFile *gf_parent = g_file_new_for_path (filename);

      gf_parent = g_file_get_parent (gf);
      g_file_make_directory_with_parents (gf_parent, NULL, NULL);
      g_object_unref (gf_parent);
      g_object_unref (gf);
      save_annotation ();
    }

  if (CS_REVISION != CS_STORED_REVISION || force)
    {
      gchar *content;
      gfloat x, y;

      g_print ("saving\n");

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
  if (cluttersmith->ui_mode & CS_UI_MODE_EDIT)
    cs_save (FALSE);
  return TRUE;
}

static void parsed_callback (const gchar *id,
                             const gchar *signal,
                             const gchar *code)
{
  GHashTable *ht;
  GList *callbacks;
  ClutterActor *actor;

  actor = cluttersmith_get_actor (id);
  if (!actor)
    {
      g_warning ("didnt find actor %s", id);
      return;
    }

  ht = g_object_get_data (G_OBJECT (actor), "callbacks");
  if (!ht)
    {
      ht = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
      g_object_set_data_full (G_OBJECT (actor), "callbacks", ht, freecode);
    }
  callbacks = g_hash_table_lookup (ht, signal);
  callbacks = g_list_append (callbacks, g_strdup (code));
  g_hash_table_insert (ht, g_strdup (signal), callbacks);
}

static void
update_animator_editor (ClutterStates *states,
                 const gchar   *source_state,
                 const gchar   *target_state)
{
  GList *k, *keys;
  cs_container_remove_children (cluttersmith->animator_props);

  keys = clutter_states_get_keys (states,
                                  source_state,
                                  target_state,
                                  NULL,
                                  NULL);
                                  
  for (k = keys; k; k = k->next)
    {
      ClutterStateKey *key = k->data;
      ClutterColor white = {0xff,0xff,0xff,0xff};
      ClutterActor *text;
      gchar *str;
     
      /* if we ask for NULL we get everything, but we really
       * only want NULL
       */
      if (source_state == NULL &&
          clutter_state_key_get_source_state_name (key))
        continue;

      str = g_strdup_printf ("%p %s %li", clutter_state_key_get_object (key), 
                          clutter_state_key_get_property_name (key),
                          clutter_state_key_get_mode (key)
                          );
      text = clutter_text_new_full ("Sans 10px", str, &white);
      g_free (str);
      clutter_container_add_actor (CLUTTER_CONTAINER (cluttersmith->animator_props),
                                   text);
    }
}

void cs_prop_tweaked (GObject     *object,
                      const gchar *property_name)
{
  /* XXX: if we are editing a state machine, update the
   * state being adited with the new state for this
   * property
   */

  if (cluttersmith->current_animator)
    {
       gdouble progress = 0.0;
       GValue value = {0, };
       GParamSpec      *pspec;
       pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (object),
                                             property_name);
       g_value_init (&value, pspec->value_type);
       g_object_get_property (object, property_name, &value);
       
       progress = mx_slider_get_progress (MX_SLIDER (cluttersmith->animator_progress));

       if (cs_set_keys_freeze == 0)
         clutter_animator_set_key (cluttersmith->current_animator,
                                   object,
                                   property_name,
                                   CLUTTER_LINEAR,
                                   progress,
                                   &value);
       g_print ("%p %p %s %f\n", cluttersmith->current_animator,
                                 object, property_name, progress);
       g_value_unset (&value);

       cs_animator_editor_set_animator (CS_ANIMATOR_EDITOR (cluttersmith->animator_editor), cluttersmith->current_animator);

       /* XXX: we should update with the real animator in this case */
       update_animator_editor (cluttersmith->current_state_machine,
                        NULL,
                        cluttersmith->current_state);

       {
         GList *k, *keys;

         keys = clutter_animator_get_keys (cluttersmith->current_animator,
                                           NULL, NULL, -1);
         for (k = keys; k; k = k->next)
           {
              ClutterAnimatorKey *key = k->data;

              g_print ("%p.%s.%f \n", 
                       clutter_animator_key_get_object (key),
                       clutter_animator_key_get_property_name (key),
                       clutter_animator_key_get_progress (key));
           }
         g_print ("\n");

         g_list_free (keys);
       }
    }
  else if (cluttersmith->current_state_machine &&
      (cluttersmith->current_state != NULL &&
       cluttersmith->current_state != g_intern_static_string ("default")))
    {
       const gchar *source_state = NULL;
       GValue value = {0, };
       GParamSpec      *pspec;
       pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (object),
                                             property_name);
       g_value_init (&value, pspec->value_type);
       g_object_get_property (object, property_name, &value);

       source_state = mx_entry_get_text (MX_ENTRY (cluttersmith->source_state));
       if (g_str_equal (source_state, "*") ||
           g_str_equal (source_state, ""))
         {
           source_state = NULL;
         }

       if (cs_set_keys_freeze == 0)
       clutter_states_set_key (cluttersmith->current_state_machine,
                               source_state,
                               cluttersmith->current_state,
                               object,
                               property_name,
                               CLUTTER_LINEAR,
                               &value,
                               0.0,
                               0.0);
       g_value_unset (&value);

       update_animator_editor (cluttersmith->current_state_machine,
                        source_state, cluttersmith->current_state);
    }
}

static void update_animator_editor2 (void)
{
  const gchar *source_state = NULL;

  source_state = mx_entry_get_text (MX_ENTRY (cluttersmith->source_state));
  if (g_str_equal (source_state, "*") ||
      g_str_equal (source_state, ""))
    {
      source_state = NULL;
    }
  update_animator_editor (cluttersmith->current_state_machine,
                   source_state, cluttersmith->current_state);
}

static void remove_state_machines (void)
{
  GList *i;
  for (i = cluttersmith->state_machines; i; i = i->next)
    {
      g_object_unref (i->data);
    }
  g_list_free (cluttersmith->state_machines);
  cluttersmith->state_machines = NULL;
}

static void cs_load (void)
{
  cs_container_remove_children (cluttersmith->property_editors);
  cs_container_remove_children (cluttersmith->scene_graph);
  cs_container_remove_children (cluttersmith->fake_stage);

  remove_state_machines ();

  if (g_file_test (filename, G_FILE_TEST_IS_REGULAR))
    {
      gchar *scriptfilename;
      gchar *annotationfilename;
      session_history_add (cs_get_project_root ());
      cluttersmith->fake_stage = cs_replace_content2 (cluttersmith->parasite_root, "fake-stage", filename);

      scriptfilename = g_strdup_printf ("%s/%s.js", cs_get_project_root(),
                                  cluttersmith->priv->title);
      annotationfilename = g_strdup_printf ("%s/%s.txt", cs_get_project_root(),
                                  cluttersmith->priv->title);
      if (g_file_test (annotationfilename, G_FILE_TEST_IS_REGULAR))
        {
          GError      *error = NULL;
          gchar *txt;
          gsize len;


          if (!g_file_get_contents (annotationfilename, (void*)&txt, &len, &error))
            {
               g_printerr("failed loading file %s: %s\n", annotationfilename, error->message);
            }
          else
            {
              if (cluttersmith->dialog_editor_annotation)
                {
                  g_print ("LOADING %s\n", txt);
                  clutter_text_set_text (CLUTTER_TEXT(cluttersmith->dialog_editor_annotation), txt);
                }
              g_free (txt);
            }
        }
      else
        {
          if (cluttersmith->dialog_editor_text)
            clutter_text_set_text (CLUTTER_TEXT(cluttersmith->dialog_editor_annotation), "...");
        }


      if (g_file_test (scriptfilename, G_FILE_TEST_IS_REGULAR))
        {
          GError      *error = NULL;
          guchar *js;
          gchar *code = JS_PREAMBLE; 
          gsize len;

          len = strlen (code);

          if (!g_file_get_contents (scriptfilename, (void*)&js, &len, &error))
            {
               g_printerr("failed loading file %s: %s\n", scriptfilename, error->message);
            }
          else
            {
              gchar *p = (void*)js;
              gchar *end = p;
              g_print ("loaded: %s\n", scriptfilename);
              gint lineno=0;
              while (*p)
                {
                  if (g_str_has_prefix (p, "/*[CS] "))
                    {
                      GString *id     = g_string_new ("");
                      GString *signal = g_string_new ("");
                      GString *code   = g_string_new ("");
                      gint item=0;
                      gint endlineno = lineno;
                      gchar *q=p+7;
                      
                      while (q && *q)
                        {
                          if (g_str_has_prefix (q, "});/*[CS]*/"))
                            {
                              end = q + 12;
                              q=NULL;
                              continue;
                            }

                           switch (item)
                             {
                               case 0:
                                 if (*q == ':')
                                   {
                                    item++;
                                   }
                                 else
                                   {
                                     g_string_append_c (id, *q);
                                   }
                                 break;
                               case 1:
                                 if (*q == ':' ||
                                     *q == ' ' ||
                                     *q == '*')
                                   {
                                    item++;
                                   }
                                 else
                                   {
                                     g_string_append_c (signal, *q);
                                   }
                                 break;
                               case 2:
                                 if (*q == '{')
                                   {
                                    if (*(q+1) == '\n')
                                      q++;
                                    item++;
                                   }
                                 break;
                               case 3:
                                 g_string_append_c (code, *q);
                                 break;
                               default:
                                 q=NULL;
                                 break;
                             }

                           if (*q=='\n')
                             endlineno++;
                          q++;
                        }


                      parsed_callback (id->str, signal->str, code->str);

                      g_string_free (id, TRUE);
                      g_string_free (signal, TRUE);
                      g_string_free (code, TRUE);
                    }
                  if (*p=='\n')
                    lineno++;
                  p++;
                }

              g_print ("end:{%s}\n", end);
              
              if (cluttersmith->dialog_editor_text)
                clutter_text_set_text (CLUTTER_TEXT(cluttersmith->dialog_editor_text), (gchar*)end);
              g_free (js);
            }
        }
      else
        {
          if (cluttersmith->dialog_editor_text)
            clutter_text_set_text (CLUTTER_TEXT(cluttersmith->dialog_editor_text), "/* */");
        }
      g_free (scriptfilename);
      g_free (annotationfilename);

      /* create a fake set of state machines,. */
      if (g_str_equal (cluttersmith->priv->title, "index"))
        {
          ClutterStates *states;
          g_print ("loaded scene [%s]\n", cluttersmith->priv->title);
          states = clutter_states_new ();
          /* XXX: add actors and states */
          cluttersmith->state_machines = g_list_append (cluttersmith->state_machines,
                                                        states);
          cluttersmith->current_state_machine = states;
        }
      else
        {
          cluttersmith->current_state_machine = NULL;
        }
    }
  else
    {
      cluttersmith->fake_stage = cs_replace_content2 (cluttersmith->parasite_root, "fake-stage", NULL);

    }
  cs_set_current_container (cluttersmith->fake_stage);
  CS_REVISION = CS_STORED_REVISION = 0;

  clutter_actor_set_position (cluttersmith->fake_stage, 
     -cluttersmith->priv->origin_x,
     -cluttersmith->priv->origin_y);

}

static void title_text_changed (ClutterActor *actor)
{
  const gchar *title = clutter_text_get_text (CLUTTER_TEXT (actor));
  
  cs_save (FALSE);
  if (cluttersmith->priv->title)
    g_free (cluttersmith->priv->title);
  cluttersmith->priv->title = g_strdup (title);


  filename = g_strdup_printf ("%s/%s.json", cs_get_project_root(),
                              title);

  if (!(cluttersmith->ui_mode & CS_UI_MODE_EDIT))
    page_run_end ();

  cs_load ();
  cs_sync_chrome_idle (NULL);
  cs_selected_clear ();

  if (!(cluttersmith->ui_mode & CS_UI_MODE_EDIT))
    page_run_start ();
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


/**
 * cluttersmith_foobar:
 * @stringA: new root
 * @stringB: new root
 */
void cluttersmith_foobar (const gchar *stringA,
                          const gchar *stringB)
{
  g_print ("::::::::::::%s %s\n", stringA, stringB);
}


/**
 * cluttersmith_set_project_root:
 * @new_root: new root
 */
void cluttersmith_set_project_root (const gchar *new_root)
{
  ClutterActor *project_root;
  if (cluttersmith->project_root &&
      g_str_equal (cluttersmith->project_root, new_root))
    {
      return;
    }

  project_root = cs_find_by_id_int (clutter_actor_get_stage (cluttersmith->parasite_root), "project-root");
  if (project_root)
    {
      g_object_set (G_OBJECT (project_root), "text", new_root, NULL);
    }
  else
    {
      g_print ("set root %s\n", new_root);
    }
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
      cluttersmith_load_scene ("index");
    }
}


static void project_title_text_changed (ClutterActor *actor)
{
  const gchar *new_title = clutter_text_get_text (CLUTTER_TEXT (actor));
  gchar *path;

  path = g_strdup_printf ("%s/cluttersmith/%s",
                          g_get_user_special_dir (G_USER_DIRECTORY_DOCUMENTS),
                          new_title);
  cluttersmith_set_project_root (path);
  g_free (path);
  /*
  if (cluttersmith->project_root)
    g_free (cluttersmith->project_root);
  cluttersmith->project_root = g_strdup (new_text);

  if (g_file_test (cluttersmith->project_root, G_FILE_TEST_IS_DIR))
    {
      previews_reload (cs_find_by_id_int (clutter_actor_get_stage(actor), "previews-container"));
      cluttersmith_load_scene ("index");
    }
    */
}

void project_title_init_hack (ClutterActor  *actor)
{
  /* we hook this up to the first paint, since no other signal seems to
   * be available to hook up for some additional initialization
   */
  static gboolean done = FALSE; 
  if (done)
    return;
  done = TRUE;

  g_signal_connect (mx_entry_get_clutter_text (MX_ENTRY (actor)), "text-changed",
                    G_CALLBACK (project_title_text_changed), NULL);
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

  g_signal_connect (mx_entry_get_clutter_text (MX_ENTRY (actor)), "text-changed",
                    G_CALLBACK (project_root_text_changed), NULL);
}



void title_entry_init_hack (ClutterActor  *actor)
{
  /* we hook this up to the first paint, since no other signal seems to
   * be available to hook up for some additional initialization
   */
  static gboolean done = FALSE; 
  if (done)
    return;
  done = TRUE;

  g_signal_connect (mx_entry_get_clutter_text (MX_ENTRY (actor)), "text-changed",
                    G_CALLBACK (title_text_changed), NULL);
}

static void update_duration (void)
{
  gchar *str;
  gint   duration;
  const gchar *source_state = NULL;

  source_state = mx_entry_get_text (MX_ENTRY (cluttersmith->source_state));
  if (g_str_equal (source_state, "*") ||
      g_str_equal (source_state, ""))
    {
      source_state = NULL;
    }

  duration = clutter_states_get_duration (cluttersmith->current_state_machine,
                                          source_state,
                                          cluttersmith->current_state);
  str = g_strdup_printf ("%i", duration);
  g_object_set (cluttersmith->state_duration, "text", str, NULL);
  g_free (str);
}

static void state_source_name_text_changed (ClutterActor *actor)
{
  update_duration ();
  update_animator_editor2 ();
}

static void state_name_text_changed (ClutterActor *actor)
{
  const gchar *state = clutter_text_get_text (CLUTTER_TEXT (actor));
  const gchar *default_state =  g_intern_static_string ("default");
  
  if (cluttersmith->current_state == NULL)
    cluttersmith->current_state = default_state;

  if (cluttersmith->current_state == default_state)
    {
      cs_properties_store_defaults ();
    }
  else if (g_intern_string (state) == default_state)
    {
      cs_properties_restore_defaults ();
    }
  else
    {
      /* for each cached key, that is different from current value,
       * and not in blacklist, store the values in state machine
       */

      /* update storage of this state */
      clutter_states_change (cluttersmith->current_state_machine, state);
    }

  cluttersmith->current_state = g_intern_string (state);

  update_duration ();


  update_animator_editor2 ();
}

void state_source_name_init_hack (ClutterActor  *actor)
{
  /* we hook this up to the first paint, since no other signal seems to
   * be available to hook up for some additional initialization
   */
  static gboolean done = FALSE; 
  if (done)
    return;
  done = TRUE;

  g_signal_connect (mx_entry_get_clutter_text (MX_ENTRY (actor)), "text-changed",
                    G_CALLBACK (state_source_name_text_changed), NULL);
}

void state_name_init_hack (ClutterActor  *actor)
{
  /* we hook this up to the first paint, since no other signal seems to
   * be available to hook up for some additional initialization
   */
  static gboolean done = FALSE; 
  if (done)
    return;
  done = TRUE;

  g_signal_connect (mx_entry_get_clutter_text (MX_ENTRY (actor)), "text-changed",
                    G_CALLBACK (state_name_text_changed), NULL);

}


static void state_duration_text_changed (ClutterActor *actor)
{
  const gchar *text = clutter_text_get_text (CLUTTER_TEXT (actor));
  const gchar *source_state = NULL;

  source_state = mx_entry_get_text (MX_ENTRY (cluttersmith->source_state));
  if (g_str_equal (source_state, "*") ||
      g_str_equal (source_state, ""))
    {
      source_state = NULL;
    }

  clutter_states_set_duration (cluttersmith->current_state_machine,
                               source_state,
                               cluttersmith->current_state,
                               atoi (text));
}

void state_duration_init_hack (ClutterActor  *actor)
{
  /* we hook this up to the first paint, since no other signal seems to
   * be available to hook up for some additional initialization
   */
  static gboolean done = FALSE; 
  if (done)
    return;
  done = TRUE;

  g_signal_connect (mx_entry_get_clutter_text (MX_ENTRY (actor)), "text-changed",
                    G_CALLBACK (state_duration_text_changed), NULL);
}


void state_elaborate_clicked (ClutterActor  *actor)
{
  ClutterAnimator *animator;
  const gchar *source_state;
  source_state = mx_entry_get_text (MX_ENTRY (cluttersmith->source_state));
  if (g_str_equal (source_state, "*") ||
      g_str_equal (source_state, ""))
    source_state = NULL;

  animator = cs_states_make_animator (cluttersmith->current_state_machine,
                                      source_state,
                                      cluttersmith->current_state);
  clutter_states_set_animator (cluttersmith->current_state_machine,
                               source_state,
                               cluttersmith->current_state,
                               animator);
  cluttersmith->current_animator = animator;

  g_print ("elaborate\n");
}

void state_test_clicked (ClutterActor  *actor)
{
  const gchar *source_state;
  source_state = mx_entry_get_text (MX_ENTRY (cluttersmith->source_state));
  if (g_str_equal (source_state, "*") ||
      g_str_equal (source_state, ""))
    source_state = NULL;

  /* Reset to the source of the animation */
  if (!source_state)
    {
      /* if no source state specified, restore the source state */
      clutter_states_change_noanim (cluttersmith->current_state_machine,
                                    "default");
      cs_properties_restore_defaults ();
    }
  else
    {
      clutter_states_change_noanim (cluttersmith->current_state_machine,
                                    source_state);
    }

  clutter_states_change (cluttersmith->current_state_machine,
                         cluttersmith->current_state);
}

void state_position_actors (gdouble progress)
{
  const gchar *source_state;
  ClutterTimeline *timeline;
  source_state = mx_entry_get_text (MX_ENTRY (cluttersmith->source_state));
  if (g_str_equal (source_state, "*") ||
      g_str_equal (source_state, ""))
    source_state = NULL;

  /* Reset to the source of the animation */
  if (!source_state)
    {
      /* if no source state specified, restore the source state */
      clutter_states_change_noanim (cluttersmith->current_state_machine,
                                    "default");
      cs_properties_restore_defaults ();
    }
  else
    {
      clutter_states_change_noanim (cluttersmith->current_state_machine,
                                    source_state);
    }

  timeline = clutter_states_change (cluttersmith->current_state_machine,
                                    cluttersmith->current_state);
  clutter_timeline_pause (timeline);

  cs_set_keys_freeze ++;
    {
      gint frame = clutter_timeline_get_duration (timeline) * progress;
      clutter_timeline_advance (timeline, frame);
      g_signal_emit_by_name (timeline, "new-frame", frame, NULL);
    }
  cs_set_keys_freeze --;
}
