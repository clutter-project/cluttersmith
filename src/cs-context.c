/*
 * ClutterSmith - a visual authoring environment for clutter.
 * Copyright (c) 2009, Intel Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU Lesser General Public License,
 * version 2.1, as published by the Free Software Foundation.
 *
 * Alternatively, you can redistribute it and/or modify it
 * under the terms and conditions of the GNU Lesser General Public License,
 * version 3, with the following additional permissions:
 *
 * 1. Intel grants you an additional permission under Section 7 of the
 * GNU General Public License, version 3, exempting you from the
 * requirement in Section 6 of the GNU General Public License, version 3,
 * to accompany Corresponding Source with Installation Information for
 * the Program or any work based on the Program.  You are still required
 * to comply with all other Section 6 requirements to provide
 * Corresponding Source.
 *
 * 2. Intel grants you an additional permission under Section 7 of the
 * GNU General Public License, version 3, allowing you to convey the
 * Program or a work based on the Program in combination with or linked
 * to any works licensed under the GNU General Public License version 2,
 * with the terms and conditions of the GNU General Public License
 * version 2 applying to the combined or linked work as a whole.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Written by: Øyvind Kolås <oyvind.kolas@intel.com>
 */

#include "cluttersmith.h"
#include "cs-context.h"
#include <gjs/gjs.h>
#include <gio/gio.h>
#include <string.h>

gchar *json_serialize_subtree (ClutterActor *root);

G_DEFINE_TYPE (CSContext, cs_context, G_TYPE_OBJECT)

#define CONTEXT_PRIVATE(o) \
      (G_TYPE_INSTANCE_GET_PRIVATE ((o), CS_TYPE_CONTEXT, CSContextPrivate))

gint cs_set_keys_freeze = 0; /* XXX: global! */

enum
{
  PROP_0,
  PROP_FULLSCREEN,
  PROP_ZOOM,
  PROP_ORIGIN_X,
  PROP_ORIGIN_Y,
  PROP_CANVAS_WIDTH,
  PROP_CANVAS_HEIGHT,
  PROP_SCENE,
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

static void project_root_text_changed (ClutterActor *actor);
static void state_machine_name_changed (ClutterActor *actor);
static void project_title_text_changed (ClutterActor *actor);
static void scene_title_text_changed (ClutterActor *actor);

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

  pspec = g_param_spec_string ("scene", "Scene", "current scene", "", G_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_SCENE, pspec);

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
#include "cluttersmith.h"

CSContext *cs = NULL;

static guint  CS_REVISION;
static guint  CS_STORED_REVISION;

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
  if (!cs->parasite_ui)
    return;
  clutter_actor_get_transformed_position (cs_find_by_id_int (clutter_actor_get_stage (cs->parasite_root), "fake-stage-rect"), &x, &y);

  /* move fake_stage and canvas into a single group that is transformed? */
#if 1
    clutter_actor_set_position (cs->fake_stage, 
      x-cs->priv->origin_x,
      y-cs->priv->origin_y);
    clutter_actor_set_position (cs->fake_stage_canvas, 
      x-cs->priv->origin_x,
      y-cs->priv->origin_y);
#else
    clutter_actor_animate (cs->fake_stage_canvas, CLUTTER_LINEAR, 100,
      "x", x-cs->priv->origin_x, "y", y-cs->priv->origin_y, NULL);
    clutter_actor_animate (cs->fake_stage, CLUTTER_LINEAR, 100,
      "x", x-cs->priv->origin_x, "y", y-cs->priv->origin_y, NULL);
#endif


    clutter_actor_show (cs->parasite_ui);

    /* Workaround for ClutterScript / json not obeying the x-fill
     * specified in cluttersmith.json
     */
      {
        gboolean x_fill;
        g_object_get (cs->parasite_ui, "x-fill", &x_fill, NULL);
        if (!x_fill)
          {
            g_print ("Warning: Forcing x-fill property of #parasite-ui\n");
            g_object_set (cs->parasite_ui, "x-fill", TRUE, NULL);
          }
      }
    clutter_actor_set_scale (cs->fake_stage, cs->priv->zoom/100.0,
                                                       cs->priv->zoom/100.0);

    clutter_actor_set_scale (cs->fake_stage_canvas, cs->priv->zoom/100.0,
                                                       cs->priv->zoom/100.0);

    has_chrome = TRUE;
  }

  gfloat cluttersmith_get_origin_x (void)
  {
    return cs->priv->origin_x;
  }

  gfloat cluttersmith_get_origin_y (void)
  {
    return cs->priv->origin_y;
  }

  static void cluttsmith_hide_chrome (void)
  {
    gfloat x, y;
    if (!cs->parasite_ui)
      return;
    clutter_actor_hide (cs->parasite_ui);
    clutter_actor_get_transformed_position (cs_find_by_id_int (clutter_actor_get_stage (cs->parasite_root), "fake-stage-rect"), &x, &y);

#if 1
  clutter_actor_set_position (cs->fake_stage, 
    -cs->priv->origin_x,
    -cs->priv->origin_y);
  clutter_actor_set_position (cs->fake_stage_canvas, 
    -cs->priv->origin_x,
    -cs->priv->origin_y);
#else
  clutter_actor_animate (cs->fake_stage, CLUTTER_LINEAR, 100,
    "x", -cs->priv->origin_x,
    "y", -cs->priv->origin_y, NULL);
  clutter_actor_animate (cs->fake_stage_canvas, CLUTTER_LINEAR, 100,
    "x", -cs->priv->origin_x,
    "y", -cs->priv->origin_y, NULL);
#endif

  clutter_actor_set_scale (cs->fake_stage, cs->priv->zoom/100.0,
                                                     cs->priv->zoom/100.0);

  clutter_actor_set_scale (cs->fake_stage_canvas, cs->priv->zoom/100.0,
                                                     cs->priv->zoom/100.0);

  has_chrome = FALSE;
}

guint cs_get_ui_mode (void)
{
  return cs->ui_mode;
}

static gboolean cs_sync_chrome_idle (gpointer data)
{
  if (!cs) /*the singleton might not be made yet */
    return FALSE;
  if (!cs->fake_stage_canvas)
    return FALSE;
  clutter_actor_set_size (cs->fake_stage_canvas,
                          cs->priv->canvas_width,
                          cs->priv->canvas_height);
  if (cs->ui_mode & CS_UI_MODE_UI)
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
  if (cs->ui_mode & CS_UI_MODE_UI)
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
  const gchar *annotation;
  gchar *annotationfilename;
  gint len;

  if (!cs->dialog_editor_annotation)
    return;

  annotation = clutter_text_get_text (CLUTTER_TEXT (cs->dialog_editor_annotation));
  annotationfilename = g_strdup_printf ("%s/%s.txt", cs_get_project_root(),
                              cs->priv->title);
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
                              cs->priv->title);
  annotationfilename = g_strdup_printf ("%s/%s.txt", cs_get_project_root(),
                              cs->priv->title);



  save_annotation ();

  if (cs->dialog_editor_text)
  {
    const gchar *text = clutter_text_get_text (CLUTTER_TEXT (cs->dialog_editor_text));
    GString *new = g_string_new ("");

    gint len = strlen (text);
    if (len >= 6 || 1)
      {
        GList *actors, *a;
        actors = cs_container_get_children_recursive (CLUTTER_CONTAINER (cluttersmith_get_stage ()));
      
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

      g_assert (cs->priv->page_js_context == NULL);
      cs->priv->page_js_context = gjs_context_new_with_search_path(NULL);
      gjs_context_eval(cs->priv->page_js_context, (void*)code, len,
  "<code>", NULL, NULL);
      if (!g_file_get_contents (scriptfilename, (void*)&js, &len, &error))
        {
           g_printerr("failed loading file %s: %s\n", scriptfilename, error->message);
        }
      else
        {
          gint code;
          if (!gjs_context_eval(cs->priv->page_js_context, (void*)js, len,
                   "<code>", &code, &error))
            {
              if (cs->dialog_editor_error)  
               clutter_text_set_text (CLUTTER_TEXT(cs->dialog_editor_error),
                                      error->message);
              else
                g_print (error->message);
               g_idle_add (return_to_ui, NULL);
            }
          else
            {
              gchar *str = g_strdup_printf ("returned: %i\n", code);
              if (cs->dialog_editor_error)  
                clutter_text_set_text (CLUTTER_TEXT(cs->dialog_editor_error),
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
  if (cs->priv->page_js_context)
    {
      g_object_unref (cs->priv->page_js_context);
      cs->priv->page_js_context = NULL;
    }

}

static void browse_start (void)
{
  page_run_end ();
  cs_save (TRUE);
  clutter_actor_hide (cs->fake_stage_canvas);

  /* XXX: apply signal handlers */
  page_run_start ();
}

static void cs_load (void);

static void browse_end (void)
{
  page_run_end ();
  g_print ("leaving browse mode\n");
  clutter_actor_show (cs->fake_stage_canvas);
  cs_load (); /* reverting back to saved state */
}

void cs_set_ui_mode (guint ui_mode)
{

  if (!(ui_mode & CS_UI_MODE_EDIT) &&
      (cs->ui_mode & CS_UI_MODE_EDIT))
    {
      browse_start ();
    }
  else if ((ui_mode & CS_UI_MODE_EDIT) &&
           !(cs->ui_mode & CS_UI_MODE_EDIT))
    {
      browse_end ();
    }

  cs->ui_mode = ui_mode;
  cs_selected_clear ();
  cs_sync_chrome ();
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
      case PROP_SCENE:
        g_value_set_string (value, context->priv->title);
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
      case PROP_SCENE:
        cs_context_set_scene (context, g_value_get_string (value));
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

/* XXX: evil global */
static gchar *filename = NULL;

const gchar *cs_context_get_scene (CSContext *context)
{
  return context->priv->title;
}

void cs_context_set_scene (CSContext   *context,
                           const gchar *new_title)
{
  g_print ("%s\n", G_STRLOC);

  if (context->scene_title)
    {
      g_object_set (context->scene_title, "text", new_title, NULL);
    }
  else
    {
    }

  cs_save (FALSE);
  if (context->priv->title)
    g_free (context->priv->title);

  context->priv->title = g_strdup (new_title);
  if (filename)
    g_free (filename);
  filename = g_strdup_printf ("%s/%s.json", cs_get_project_root(),
                              new_title);

  if (!(context->ui_mode & CS_UI_MODE_EDIT))
    page_run_end ();

  cs_load ();
  cs_sync_chrome_idle (NULL);
  cs_selected_clear ();

  if (!(context->ui_mode & CS_UI_MODE_EDIT))
    page_run_start ();

  g_object_notify (G_OBJECT (context), "scene");
}

/**
 * cluttersmith_load_scene:
 * @new_title: new scene
 */
void cluttersmith_load_scene (const gchar *new_title)
{
  gchar *undo, *redo;
  redo = g_strdup_printf ("CS.context().scene='%s'", new_title);
  if (cs->priv->title != NULL)
    undo = g_strdup_printf ("CS.context().scene='%s'", cs->priv->title);
  else
    undo = g_strdup (redo);
  cs_history_do ("change scene", redo, undo);
  g_free (undo);
  g_free (redo);
}

/**
 * cluttersmith_context:
 * Return value: (transfer none): the clurresmith context.
 */
CSContext *cluttersmith_context (void)
{
  return cs;
}

/**
 * cluttersmith_get_scene:
 */
const gchar *cluttersmith_current_scene (void)
{
  return cs_context_get_scene (cs);
}

/**
 * cluttersmith_animator_start:
 * @new_title: new scene
 *
 * Return value: (transfer none): the timeline involved.
 */
ClutterTimeline *cluttersmith_animator_start (const gchar *animator)
{
  GList *a;
  g_print ("trying to start %s\n", animator);
  for (a = cs->animators; a; a = a->next)
    {
      const gchar *name = clutter_scriptable_get_id (a->data);
      if (name && g_str_equal (name, animator))
        {
          return clutter_animator_start (a->data);
        }
    }
  g_print ("failed\n");
  return NULL;
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

  cs = cs_context_new ();
  clutter_script_load_from_data (script, "{'id':'actor','type':'ClutterGroup','children':[{'id':'fake-stage','type':'ClutterGroup'},{'id':'parasite-root','type':'ClutterGroup'},{'id':'cs-project-title','opacity':1,'type':'MxEntry'},{'id':'cs-scene-title','type':'MxEntry','opacity':1},{'id':'project-root','type':'MxEntry','opacity':1,'y':50 }]}", -1, NULL);

  stage = clutter_stage_get_default();
  actor = CLUTTER_ACTOR (clutter_script_get_object (script, "actor"));
  clutter_container_add_actor (CLUTTER_CONTAINER (stage), actor);

#define _A(actorname)  CLUTTER_ACTOR (clutter_script_get_object (script, actorname))
  cs->project_root_entry = _A("project-root");
  cs->project_title = _A("cs-project-title");
  cs->scene_title = _A("cs-scene-title");

  g_object_set_data_full (G_OBJECT (actor), "clutter-script", script, g_object_unref);

  g_signal_connect (mx_entry_get_clutter_text (MX_ENTRY (cs->project_root_entry)),
   "text-changed", G_CALLBACK (project_root_text_changed), NULL);
  g_signal_connect (mx_entry_get_clutter_text (MX_ENTRY (cs->project_title)),
   "text-changed", G_CALLBACK (project_title_text_changed), NULL);
  g_signal_connect (mx_entry_get_clutter_text (MX_ENTRY (cs->scene_title)),
   "text-changed", G_CALLBACK (scene_title_text_changed), NULL);

  cs_set_active (clutter_actor_get_stage(cs->parasite_root));

  cs->parasite_root = CLUTTER_ACTOR (clutter_script_get_object (script, "parasite-root"));
  /* initializing globals */

  /* this is the main initialization */
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

void animator_editor_update_handles (void);

static gint max_x = 0;
static gint max_y = 0;
static gint min_x = 0;
static gint min_y = 0;

static void find_extent (ClutterActor *actor,
                         gpointer      data)
{
  ClutterVertex verts[4];
  clutter_actor_get_abs_allocation_vertices (actor,
                                             verts);

  {
    gint i;
    for (i=0;i<4;i++)
      {
        if (verts[i].x > max_x)
          max_x = verts[i].x;
        if (verts[i].x < min_x)
          min_x = verts[i].x;
        if (verts[i].y > max_y)
          max_y = verts[i].y;
        if (verts[i].y < min_y)
          min_y = verts[i].y;
      }
  }
}


gboolean update_overlay_positions (gpointer data)
{
  ClutterActor *actor;

  if (cs->current_animator)
    animator_editor_update_handles ();

  if (cs_selected_count ()==0 && lasso == NULL)
    {
      clutter_actor_hide (cs->move_handle);
      clutter_actor_hide (cs->depth_handle);
      clutter_actor_hide (cs->resize_handle);
      clutter_actor_hide (cs->anchor_handle);
      clutter_actor_hide (cs->rotate_x_handle);
      clutter_actor_hide (cs->rotate_y_handle);
      clutter_actor_hide (cs->rotate_z_handle);
      return TRUE;
    }

  clutter_actor_show (cs->move_handle);
  clutter_actor_show (cs->depth_handle);
  clutter_actor_show (cs->resize_handle);
  clutter_actor_show (cs->anchor_handle);
  clutter_actor_show (cs->rotate_x_handle);
  clutter_actor_show (cs->rotate_y_handle);
  clutter_actor_show (cs->rotate_z_handle);

  actor = cs_selected_get_any ();


  if (actor)
    {
      ClutterVertex pos = {0,0,0};
      clutter_actor_get_anchor_point (actor, &pos.x, &pos.y);
      clutter_actor_apply_transform_to_point (actor, &pos, &pos);
      
      clutter_actor_set_position (cs->anchor_handle, pos.x, pos.y);

      pos.x = pos.y = pos.z = 0;
      clutter_actor_get_size (actor, &pos.x, &pos.y);
      clutter_actor_apply_transform_to_point (actor, &pos, &pos);
      clutter_actor_set_position (cs->resize_handle, pos.x, pos.y);

      pos.x = pos.y = pos.z = 0;
      clutter_actor_get_size (actor, &pos.x, &pos.y);
      pos.x = 0.0;
      clutter_actor_apply_transform_to_point (actor, &pos, &pos);
      clutter_actor_set_position (cs->rotate_z_handle, pos.x, pos.y);
      clutter_actor_get_position (cs->rotate_z_handle, &pos.x, &pos.y);
      clutter_actor_set_position (cs->rotate_y_handle, pos.x - clutter_actor_get_width (cs->rotate_z_handle), pos.y);
      clutter_actor_get_position (cs->rotate_z_handle, &pos.x, &pos.y);
      clutter_actor_set_position (cs->rotate_x_handle, pos.x, pos.y + clutter_actor_get_height (cs->rotate_z_handle));
    }

  min_x = 65536;
  min_y = 65536;
  max_x = 0;
  max_y = 0;
  cs_selected_foreach (G_CALLBACK (find_extent), data);
  clutter_actor_set_position (cs->move_handle, (max_x+min_x)/2, (max_y+min_y)/2);
  clutter_actor_set_position (cs->depth_handle, (max_x+min_x)/2 + 20, (max_y+min_y)/2);

  return TRUE;
}


static void
cs_overlay_paint (ClutterActor *stage,
                  gpointer      user_data)
{
  cs_move_snap_paint ();
  cs_selected_paint ();
  cs_animator_editor_stage_paint ();
}

void init_multi_select (void);

/* The handler that might trigger the playback mode context menu */
static gboolean playback_context (ClutterActor *actor,
                                  ClutterEvent *event)
{
  if (!(cs->ui_mode & CS_UI_MODE_EDIT) &&
      clutter_event_get_button (event)==3)
    {
      playback_menu (event->button.x, event->button.y);
      return TRUE;
    }
  return FALSE;
}


/* This function is the entry point that initializes
 * ClutterSmith event handlers for a stage.
 *
 * Cluttersmith is designed to sit around an ClutterStage's
 * scene graph without impacting the exisitng behavior of
 * the actors in the graph.
 */
gboolean idle_add_stage (gpointer stage)
{
  ClutterActor *actor;
  ClutterScript *script;

  cs = cs_context_new ();
  mx_style_load_from_file (mx_style_get_default (), PKGDATADIR "cluttersmith.css", NULL);

/*#ifdef COMPILEMODULE
  actor = cs_load_json (PKGDATADIR "cluttersmith-assistant.json");
#else*/
  actor = cs_load_json (PKGDATADIR "cluttersmith.json");
 /*#endif*/

  g_object_set_data (G_OBJECT (actor), "clutter-smith", (void*)TRUE);
  clutter_container_add_actor (CLUTTER_CONTAINER (stage), actor);

   //g_print ("%s", json_serialize_subtree (actor));
   //exit (1);

  g_signal_connect_after (stage, "paint", G_CALLBACK (cs_overlay_paint), NULL);
  clutter_threads_add_repaint_func (update_overlay_positions, stage, NULL);
  g_signal_connect_after (clutter_actor_get_stage (actor), "captured-event",
                          G_CALLBACK (cs_stage_capture), NULL);
  g_signal_connect (clutter_actor_get_stage (actor), "button-press-event",
                    G_CALLBACK (playback_context), NULL);
  init_multi_select ();
  cs_edit_text_init ();


  script = cs_get_script (actor);

  /* initializing globals */
  cs->type = CLUTTER_ACTOR (clutter_script_get_object (script, "cs-type"));
  cs->ancestors = CLUTTER_ACTOR (clutter_script_get_object (script, "parents"));

  cs->property_editors = CLUTTER_ACTOR (clutter_script_get_object (script, "property-editors"));
  cs->property_editors_core = CLUTTER_ACTOR (clutter_script_get_object (script, "property-editors-core"));
  cs->scene_graph = CLUTTER_ACTOR (clutter_script_get_object (script, "scene-graph"));
  cs->parasite_ui = CLUTTER_ACTOR (clutter_script_get_object (script, "parasite-ui"));
  cs->parasite_root = CLUTTER_ACTOR (clutter_script_get_object (script, "parasite-root"));

  {
    if (cs->parasite_ui)
      {
        g_signal_connect (stage, "notify::width", G_CALLBACK (stage_size_changed), cs->parasite_ui);
        g_signal_connect (stage, "notify::height", G_CALLBACK (stage_size_changed), cs->parasite_ui);
        /* do an initial sync of the ui-size */
        stage_size_changed (stage, NULL, cs->parasite_ui);
      }
  }


  cs->fake_stage_canvas = _A("fake-stage-canvas");
  cs->project_root_entry = _A("project-root");
  cs->resize_handle = _A("resize-handle");
  cs->anchor_handle = _A("anchor-handle");
  cs->rotate_x_handle = _A("rotate-x-handle");
  cs->rotate_y_handle = _A("rotate-y-handle");
  cs->rotate_z_handle = _A("rotate-z-handle");
  cs->animation_name = _A("cs-animation-name");
  cs->move_handle = _A("move-handle");
  cs->depth_handle = _A("depth-handle");
  cs->active_container = _A("cs-active-container");
  cs->callbacks_container = _A("cs-callbacks-container");
  cs->dialog_callbacks = _A("cs-dialog-callbacks");
  cs->states_container = _A("cs-states-container");
  cs->dialog_project = _A("cs-dialog-project");
  cs->dialog_selected = _A("cs-dialog-selected");
  cs->dialog_states = _A("cs-dialog-states");
  cs->project_title = _A("cs-project-title");
  cs->scene_title = _A("cs-scene-title");
  cs->dialog_config = _A("cs-dialog-config");
  cs->dialog_tree = _A("cs-dialog-tree");
  cs->dialog_toolbar = _A("cs-dialog-toolbar");
  cs->dialog_scenes = _A("cs-dialog-scenes");
  cs->dialog_export = _A("cs-dialog-export");
  cs->dialog_templates = _A("cs-dialog-templates");
  cs->dialog_animator = _A("cs-dialog-animator");
  cs->animator_props = _A("cs-animator-props");
  cs->state_duration = _A("cs-state-duration");
  cs->dialog_editor = _A("cs-dialog-editor");
  cs->dialog_type = _A("cs-dialog-type");
  cs->dialog_annotate = _A("cs-dialog-annotate");
  cs->dialog_editor_annotation = _A("cs-dialog-editor-annotation");
  cs->source_state = _A("cs-source-state");
  cs->state_machine_name = _A("cs-state-machine-name");
  cs->state_name = _A("cs-state-name");
  cs->dialog_editor_text = _A("cs-dialog-editor-text");
  cs->dialog_editor_error = _A("cs-dialog-editor-error");
  cs->dialog_property_inspector = _A("cs-dialog-property-inspector");
  cs->animator_editor = _A("cs-animator-editor");

  cs_animation_edit_init ();
  g_signal_connect (mx_entry_get_clutter_text (MX_ENTRY (cs->project_root_entry)),
   "text-changed", G_CALLBACK (project_root_text_changed), NULL);
  g_signal_connect (mx_entry_get_clutter_text (MX_ENTRY (cs->state_machine_name)),
   "text-changed", G_CALLBACK (state_machine_name_changed), NULL);
  g_signal_connect (mx_entry_get_clutter_text (MX_ENTRY (cs->project_title)),
   "text-changed", G_CALLBACK (project_title_text_changed), NULL);
  g_signal_connect (mx_entry_get_clutter_text (MX_ENTRY (cs->scene_title)),
   "text-changed", G_CALLBACK (scene_title_text_changed), NULL);

  cs_set_active (clutter_actor_get_stage(cs->parasite_root));

  props_populate (_A("config-editors"), G_OBJECT (cs), FALSE);

#ifdef COMPILEMODULE
  clutter_actor_hide (cs->fake_stage_canvas);
#endif

  /* XXX: all of this should be in the json */
  cs->cs_mode = CLUTTER_ACTOR (clutter_script_get_object (script, "cs-mode"));

  mx_combo_box_append_text (MX_COMBO_BOX (cs->cs_mode), "Browse (F1)");
  mx_combo_box_append_text (MX_COMBO_BOX (cs->cs_mode), "Annotate (F2)");
  mx_combo_box_append_text (MX_COMBO_BOX (cs->cs_mode), "Sketch (F3)");
  mx_combo_box_append_text (MX_COMBO_BOX (cs->cs_mode), "Edit (F4)");
  mx_combo_box_append_text (MX_COMBO_BOX (cs->cs_mode), "Animate (F5)");
  mx_combo_box_append_text (MX_COMBO_BOX (cs->cs_mode), "Callbacks (F6)");
  mx_combo_box_append_text (MX_COMBO_BOX (cs->cs_mode), "Code (F7)");
  mx_combo_box_append_text (MX_COMBO_BOX (cs->cs_mode), "Export (F8)");
  mx_combo_box_append_text (MX_COMBO_BOX (cs->cs_mode), "Config (F9)");

  mx_combo_box_set_index (MX_COMBO_BOX (cs->cs_mode), 0);

  g_signal_connect (cs->cs_mode, "notify::index", G_CALLBACK (mode_switch), NULL);

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
  //clutter_scriptable_set_id (CLUTTER_SCRIPTABLE (data), clutter_text_get_text (text));
  const gchar *set;
  const gchar *stem;
  stem = clutter_text_get_text (text);
  clutter_scriptable_set_id (CLUTTER_SCRIPTABLE (data), stem);
  cs_actor_make_id_unique (data, stem);
  set = clutter_scriptable_get_id (data);
  if (!g_str_equal (stem, set))
    {
      clutter_text_set_text (CLUTTER_TEXT (text), set);
    }
  cs_dirtied ();
  return TRUE;
}


void actor_defaults_populate (ClutterActor *container,
                              ClutterActor *actor)
{
  ClutterColor  white = {0xff,0xff,0xff,0xff};  /* XXX: should be in CSS */
  ClutterActor *hbox;
  ClutterActor *label;
  ClutterActor *editor; 

  /* position */
  hbox = g_object_new (MX_TYPE_BOX_LAYOUT, "spacing", 5, NULL);
  label = clutter_text_new_with_text (CS_EDITOR_LABEL_FONT, "x, y, z");
  clutter_text_set_color (CLUTTER_TEXT (label), &white);
  clutter_container_add_actor (CLUTTER_CONTAINER (hbox), label);
  clutter_actor_set_width (label, CS_PROPEDITOR_LABEL_WIDTH);

  editor = property_editor_new (G_OBJECT (actor), "x");
  clutter_container_add_actor (CLUTTER_CONTAINER (hbox), editor);
  clutter_container_child_set (CLUTTER_CONTAINER (hbox), editor,
                               "expand", TRUE, NULL);
  editor = property_editor_new (G_OBJECT (actor), "y");
  clutter_container_add_actor (CLUTTER_CONTAINER (hbox), editor);
  clutter_container_child_set (CLUTTER_CONTAINER (hbox), editor,
                               "expand", TRUE, NULL);
  editor = property_editor_new (G_OBJECT (actor), "depth");
  clutter_container_add_actor (CLUTTER_CONTAINER (hbox), editor);
  clutter_container_child_set (CLUTTER_CONTAINER (hbox), editor,
                               "expand", TRUE, NULL);

  clutter_container_add_actor (CLUTTER_CONTAINER (container), hbox);

  /* dimensions */
  hbox = g_object_new (MX_TYPE_BOX_LAYOUT, "spacing", 5, NULL);
  label = clutter_text_new_with_text (CS_EDITOR_LABEL_FONT, "width, height");
  clutter_text_set_color (CLUTTER_TEXT (label), &white);
  clutter_container_add_actor (CLUTTER_CONTAINER (hbox), label);
  clutter_actor_set_width (label, CS_PROPEDITOR_LABEL_WIDTH);

  editor = property_editor_new (G_OBJECT (actor), "width");
  clutter_container_add_actor (CLUTTER_CONTAINER (hbox), editor);
  clutter_container_child_set (CLUTTER_CONTAINER (hbox), editor,
                               "expand", TRUE, NULL);
  editor = property_editor_new (G_OBJECT (actor), "height");
  clutter_container_add_actor (CLUTTER_CONTAINER (hbox), editor);
  clutter_container_child_set (CLUTTER_CONTAINER (hbox), editor,
                               "expand", TRUE, NULL);

  clutter_container_add_actor (CLUTTER_CONTAINER (container), hbox);

  /* rotation */
  hbox = g_object_new (MX_TYPE_BOX_LAYOUT, "spacing", 5, NULL);
  label = clutter_text_new_with_text (CS_EDITOR_LABEL_FONT, "rotation");
  clutter_text_set_color (CLUTTER_TEXT (label), &white);
  clutter_container_add_actor (CLUTTER_CONTAINER (hbox), label);
  clutter_actor_set_width (label, CS_PROPEDITOR_LABEL_WIDTH);

  editor = property_editor_new (G_OBJECT (actor), "rotation-angle-x");
  clutter_container_add_actor (CLUTTER_CONTAINER (hbox), editor);
  clutter_container_child_set (CLUTTER_CONTAINER (hbox), editor,
                               "expand", TRUE, NULL);
  editor = property_editor_new (G_OBJECT (actor), "rotation-angle-y");
  clutter_container_add_actor (CLUTTER_CONTAINER (hbox), editor);
  clutter_container_child_set (CLUTTER_CONTAINER (hbox), editor,
                               "expand", TRUE, NULL);
  editor = property_editor_new (G_OBJECT (actor), "rotation-angle-z");
  clutter_container_add_actor (CLUTTER_CONTAINER (hbox), editor);
  clutter_container_child_set (CLUTTER_CONTAINER (hbox), editor,
                               "expand", TRUE, NULL);
  clutter_container_add_actor (CLUTTER_CONTAINER (container), hbox);

  /* scale */
  hbox = g_object_new (MX_TYPE_BOX_LAYOUT, "spacing", 5, NULL);
  label = clutter_text_new_with_text (CS_EDITOR_LABEL_FONT, "scale");
  clutter_text_set_color (CLUTTER_TEXT (label), &white);
  clutter_container_add_actor (CLUTTER_CONTAINER (hbox), label);
  clutter_actor_set_width (label, CS_PROPEDITOR_LABEL_WIDTH);

  editor = property_editor_new (G_OBJECT (actor), "scale-x");
  clutter_container_add_actor (CLUTTER_CONTAINER (hbox), editor);
  clutter_container_child_set (CLUTTER_CONTAINER (hbox), editor,
                               "expand", TRUE, NULL);
  editor = property_editor_new (G_OBJECT (actor), "scale-y");
  clutter_container_add_actor (CLUTTER_CONTAINER (hbox), editor);
  clutter_container_child_set (CLUTTER_CONTAINER (hbox), editor,
                               "expand", TRUE, NULL);
  clutter_container_add_actor (CLUTTER_CONTAINER (container), hbox);

  /* anchor */
  hbox = g_object_new (MX_TYPE_BOX_LAYOUT, "spacing", 5, NULL);
  label = clutter_text_new_with_text (CS_EDITOR_LABEL_FONT, "anchor/pivot");
  clutter_text_set_color (CLUTTER_TEXT (label), &white);
  clutter_container_add_actor (CLUTTER_CONTAINER (hbox), label);
  clutter_actor_set_width (label, CS_PROPEDITOR_LABEL_WIDTH);

  editor = property_editor_new (G_OBJECT (actor), "anchor-x");
  clutter_container_add_actor (CLUTTER_CONTAINER (hbox), editor);
  clutter_container_child_set (CLUTTER_CONTAINER (hbox), editor,
                               "expand", TRUE, NULL);
  editor = property_editor_new (G_OBJECT (actor), "anchor-y");
  clutter_container_add_actor (CLUTTER_CONTAINER (hbox), editor);
  clutter_container_child_set (CLUTTER_CONTAINER (hbox), editor,
                               "expand", TRUE, NULL);
  clutter_container_add_actor (CLUTTER_CONTAINER (container), hbox);

  /* virtual 'id' property */
  hbox = g_object_new (MX_TYPE_BOX_LAYOUT, "spacing", 5, NULL);
  label = clutter_text_new_with_text (CS_EDITOR_LABEL_FONT, "id");

  editor = CLUTTER_ACTOR (mx_entry_new ());
  g_signal_connect (mx_entry_get_clutter_text (
                    MX_ENTRY (editor)), "text-changed",
                    G_CALLBACK (update_id), actor);
  mx_entry_set_text (MX_ENTRY (editor), clutter_scriptable_get_id (CLUTTER_SCRIPTABLE (actor)));

  clutter_container_add_actor (CLUTTER_CONTAINER (hbox), label);
  clutter_actor_set_width (label, CS_PROPEDITOR_LABEL_WIDTH);
  clutter_container_add_actor (CLUTTER_CONTAINER (hbox), editor);
  clutter_container_child_set (CLUTTER_CONTAINER (hbox), editor, "expand", TRUE, NULL);
  clutter_container_add_actor (CLUTTER_CONTAINER (container), hbox);
  clutter_text_set_color (CLUTTER_TEXT (label), &white);
}


void cs_set_active (ClutterActor *item)
{
  if (!cs->type)
    return;
  cs_container_remove_children (cs->ancestors);
  cs_container_remove_children (cs->property_editors);
  cs_container_remove_children (cs->property_editors_core);
  cs_container_remove_children (cs->scene_graph);

  if (!item)
    {
      clutter_text_set_text (CLUTTER_TEXT (cs->type), "");
      /* XXX: could use the property inspector for editing properties of 
       * the current scene when no (or many) things are selected.
       */
      return;
    }

  if (item)
      clutter_text_set_text (CLUTTER_TEXT (cs->type), G_OBJECT_TYPE_NAME (item));


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
              clutter_container_add_actor (CLUTTER_CONTAINER (cs->ancestors), new);

              if (iter == cs->fake_stage)
                iter = NULL;
              else
                iter = clutter_actor_get_parent (iter);
            }

          g_object_weak_ref (G_OBJECT (item), selected_vanished, NULL);

          actor_defaults_populate (cs->property_editors_core, active_actor);
          props_populate (cs->property_editors, G_OBJECT (active_actor), FALSE);
          cs_tree_populate (cs->scene_graph, active_actor);

          if(0)props_populate (cs->active_container, G_OBJECT (active_actor), FALSE);

          cs_callbacks_populate (active_actor);
        }
    }
}


static void
ensure_unique_ids (ClutterActor *start)
{
  GList *children = cs_container_get_children_recursive (CLUTTER_CONTAINER (start));
  GList *c;
  for (c = children; c; c = c->next)
    {
      cs_actor_make_id_unique (c->data, NULL);
    }

  g_list_free (children);
}

void cs_save (gboolean force)
{
  /* Save if we've changed */
  if (filename)
    {
      GFile *gf = g_file_new_for_path (filename);
      GFile *gf_parent;

      gf_parent = g_file_get_parent (gf);
      g_file_make_directory_with_parents (gf_parent, NULL, NULL);
      g_object_unref (gf_parent);
      g_object_unref (gf);
      save_annotation ();
    }

  if (CS_REVISION != CS_STORED_REVISION || force)
    {
      GString *str = g_string_new ("[");
      gchar *tmp;
      gfloat x, y;

      g_print ("saving\n");

      clutter_actor_get_position (cs->fake_stage, &x, &y);
      clutter_actor_set_position (cs->fake_stage, 0.0, 0.0);

      /* make sure all ids are unique, ideally this would
       * already be the case, but we ensure it here as a
       * last resort, needed to make the resulting files
       * loadable
       */
      ensure_unique_ids (cs->fake_stage);

      tmp = json_serialize_subtree (cs->fake_stage);
      g_string_append (str, tmp);
      g_free (tmp);

      if (cs->animators)
        {
          GList *a;
          g_string_append (str, "\n");
          for (a = cs->animators; a; a=a->next)
            {
              tmp = json_serialize_animator (a->data);
              g_string_append (str, ",");
              g_string_append (str, tmp);
              g_free (tmp);
            }
        }
      if (cs->state_machines)
        {
          GList *a;
          g_string_append (str, "\n");
          for (a = cs->state_machines; a; a=a->next)
            {
              tmp = json_serialize_state (a->data);
              g_string_append (str, ",");
              g_string_append (str, tmp);
              g_free (tmp);
            }
        }
      g_string_append (str, "]");

      clutter_actor_set_position (cs->fake_stage, x, y);
      if (filename)
        {
          g_file_set_contents (filename, str->str, -1, NULL);
        }
      g_string_free (str, TRUE);
      CS_STORED_REVISION = CS_REVISION;
    }
}

gboolean cs_save_timeout (gpointer data)
{
  if (cs->ui_mode & CS_UI_MODE_EDIT)
    cs_save (FALSE);
  return TRUE;
}

void cs_freecode (gpointer ht);

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
      g_object_set_data_full (G_OBJECT (actor), "callbacks", ht, cs_freecode);
    }
  callbacks = g_hash_table_lookup (ht, signal);
  callbacks = g_list_append (callbacks, g_strdup (code));
  g_hash_table_insert (ht, g_strdup (signal), callbacks);
}


void cs_prop_tweaked (GObject     *object,
                      const gchar *property_name)
{
  if (cs->current_animator)
    {
       gdouble progress = 0.0;
       GValue value = {0, };
       GParamSpec      *pspec;
       pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (object),
                                             property_name);
       g_value_init (&value, pspec->value_type);
       g_object_get_property (object, property_name, &value);
       
       progress = cs_animator_editor_get_progress (CS_ANIMATOR_EDITOR (cs->animator_editor));

       if (cs_set_keys_freeze == 0)
         clutter_animator_set_key (cs->current_animator,
                                   object,
                                   property_name,
                                   CLUTTER_LINEAR,
                                   progress,
                                   &value);
       g_value_unset (&value);

       cs_animator_editor_set_animator (CS_ANIMATOR_EDITOR (cs->animator_editor), cs->current_animator);

       cs_update_animator_editor (cs->current_state_machine,
                                  NULL,
                                  cs->current_state);
    }
  else if (cs->current_state_machine &&
      (cs->current_state != NULL))
    {
       const gchar *source_state = NULL;
       GValue value = {0, };
       GParamSpec      *pspec;
       pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (object),
                                             property_name);
       g_value_init (&value, pspec->value_type);
       g_object_get_property (object, property_name, &value);

       source_state = mx_label_get_text (MX_LABEL(cs->source_state));
       if (g_str_equal (source_state, ""))
         {
           source_state = NULL;
         }

       if (cs_set_keys_freeze == 0)
        {
          GList *states, *s;

          /* get list of states */
          states = clutter_state_get_states (cs->current_state_machine);
          for (s = states; s; s = s->next)
            {
              GList *list;
              GList *list2;

              list2 = clutter_state_get_keys (cs->current_state_machine,
                                              s->data,
                                              cs->current_state,
                                              NULL,
                                              NULL);
              if (list2)
                {

              list = clutter_state_get_keys (cs->current_state_machine,
                                             s->data,
                                             cs->current_state,
                                             object,
                                             property_name);
              if (list) /* does a transition key exist for this state */
                {
                  gulong mode;
                  gdouble pre_delay, post_delay;
                  g_assert (g_list_length (list)==1);

                  /* fetch its current state */
                  mode = clutter_state_key_get_mode (list->data);
                  pre_delay  = clutter_state_key_get_pre_delay (list->data);
                  post_delay  = clutter_state_key_get_post_delay (list->data);

                  /* and update with new value */
                  clutter_state_set_key (cs->current_state_machine,
                                         s->data,
                                         cs->current_state,
                                         object,
                                         property_name,
                                         mode,
                                         &value,
                                         pre_delay,
                                         post_delay);

                  g_list_free (list);
                }
              else if (s->data != cs->current_state)
                {
                  /* otherwise, update with new value */
                  clutter_state_set_key (cs->current_state_machine,
                                         s->data,
                                         cs->current_state,
                                         object,
                                         property_name,
                                         CLUTTER_LINEAR,
                                         &value,
                                         0.0,
                                         0.0);
                }
                g_list_free (list2);
              }
            }
          g_list_free (states);
          /* otherwise, update with new value */
          clutter_state_set_key (cs->current_state_machine,
                                 source_state,
                                 cs->current_state,
                                 object,
                                 property_name,
                                 CLUTTER_LINEAR,
                                 &value,
                                 0.0,
                                 0.0);
        }
       g_value_unset (&value);

       cs_update_animator_editor (cs->current_state_machine,
                                  source_state, cs->current_state);
    }
}

static void remove_state_machines (void)
{
  GList *i;
  for (i = cs->state_machines; i; i = i->next)
    {
      g_object_unref (i->data);
    }
  g_list_free (cs->state_machines);
  cs->state_machines = NULL;
  for (i = cs->animators; i; i = i->next)
    {
      g_object_unref (i->data);
    }
  g_list_free (cs->animators);
  cs->animators = NULL;
}

/* filename has already been set when cs_load is called */
static void cs_load (void)
{
  cs_container_remove_children (cs->property_editors);
  remove_state_machines ();
  cs_container_remove_children (cs->scene_graph);
  cs_container_remove_children (cs->fake_stage);


  if (g_file_test (filename, G_FILE_TEST_IS_REGULAR))
    {
      gchar *scriptfilename;
      gchar *annotationfilename;
      cs_session_history_add (cs_get_project_root ());
      cs->fake_stage = NULL; /* forcing cs->fake_stage to NULL */
      cs->fake_stage = cs_replace_content2 (cs->parasite_root, "fake-stage", filename);

      {
        ClutterScript *script = cs_get_script (cs->fake_stage);
        GList *o, *objects;
        g_assert (script);
        objects = clutter_script_list_objects (script);

        for (o = objects; o; o = o->next)
          {
            if (CLUTTER_IS_ANIMATOR (o->data))
             {
                ClutterAnimator *animator = o->data;
                cs->animators = g_list_append (cs->animators, animator);
             }
            else if (CLUTTER_IS_STATE (o->data))
             {
                ClutterState *state = o->data;
                cs->state_machines = g_list_append (cs->state_machines, state);
                g_object_ref (state);
             }
          }
        /* Add a fake state machine at first, to bootstrap things.. */

        g_list_free (objects);
      }

      scriptfilename = g_strdup_printf ("%s/%s.js", cs_get_project_root(),
                                  cs->priv->title);
      annotationfilename = g_strdup_printf ("%s/%s.txt", cs_get_project_root(),
                                  cs->priv->title);
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
              if (cs->dialog_editor_annotation)
                {
                  g_print ("LOADING %s\n", txt);
                  clutter_text_set_text (CLUTTER_TEXT(cs->dialog_editor_annotation), txt);
                }
              g_free (txt);
            }
        }
      else
        {
          if (cs->dialog_editor_text)
            clutter_text_set_text (CLUTTER_TEXT(cs->dialog_editor_annotation), "...");
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
              
              if (cs->dialog_editor_text)
                clutter_text_set_text (CLUTTER_TEXT(cs->dialog_editor_text), (gchar*)end);
              g_free (js);
            }
        }
      else
        {
          if (cs->dialog_editor_text)
            clutter_text_set_text (CLUTTER_TEXT(cs->dialog_editor_text), "/* */");
        }
      g_free (scriptfilename);
      g_free (annotationfilename);
    }
  else
    {
      cs->fake_stage = NULL; /* forcing cs->fake_stage to NULL */
      cs->fake_stage = cs_replace_content2 (cs->parasite_root, "fake-stage", NULL);
    }
  cs_set_current_container (cs->fake_stage);
  CS_REVISION = CS_STORED_REVISION = 0;

  clutter_actor_set_position (cs->fake_stage, 
     -cs->priv->origin_x,
     -cs->priv->origin_y);
}



static gboolean title_frozen = FALSE;

/**
 * cluttersmith_set_project_root:
 * @new_root: new root
 */
void cluttersmith_set_project_root (const gchar *new_root)
{
  ClutterActor *project_root;
  if (cs->project_root &&
      g_str_equal (cs->project_root, new_root))
    {
      return;
    }

  if ((project_root = cs_find_by_id_int (clutter_actor_get_stage (cs->parasite_root), "project-root")))
    g_object_set (G_OBJECT (project_root), "text", new_root, NULL);

  if (filename)
    g_free (filename);
  filename = g_strdup_printf ("%s/%s.json", cs_get_project_root(),
                              "index");
}

gchar *cs_get_project_root (void)
{
  return cs->project_root;
}

void cs_set_current_state_machine (ClutterState *state_machine)
{
  if (cs->current_state_machine)
    {
      GList *keys = clutter_state_get_keys (cs->current_state_machine, NULL, NULL, NULL, NULL);
      if (!keys) /* if no keys are set, just destroy the machine */
        {
          g_object_unref (cs->current_state_machine);
                          cs->state_machines = g_list_remove (cs->state_machines, cs->current_state_machine);
        }
      g_list_free (keys);
    }
  cs->current_state_machine = state_machine;
  //cs_state_machine_editor_set_state_machine (CS_ANIMATOR_EDITOR (cs->state_machine_editor), state_machine);
}


static void state_machine_name_changed (ClutterActor *actor)
{
  ClutterText *text = CLUTTER_TEXT (actor);
  const gchar *name = clutter_text_get_text (text);
  ClutterState *state;
  GList *a;

  cs_properties_restore_defaults ();
  if (!name || name[0]=='\0')
    {
      /* if it is the empty string we drop any held state machine */
      if (cs->current_state_machine)
        {
          cs_properties_restore_defaults ();
        }
      cs->current_state_machine = NULL;
      return;
    }

  for (a=cs->state_machines; a; a=a->next)
    {
      const gchar *id = clutter_scriptable_get_id (a->data);
      if (id && g_str_equal (id, name))
        {
          cs_set_current_state_machine (a->data);
          return;
        }
    }
  state = clutter_state_new ();
  clutter_scriptable_set_id (CLUTTER_SCRIPTABLE (state), name);
  cs->state_machines = g_list_append (cs->state_machines, state);
  cs_set_current_state_machine (state);
}

static void project_root_text_changed (ClutterActor *actor)
{
  const gchar *new_text = clutter_text_get_text (CLUTTER_TEXT (actor));
  const gchar *project_title;
  title_frozen = TRUE;
  if (cs->project_root)
    g_free (cs->project_root);
  cs->project_root = g_strdup (new_text);

  if (!new_text)
    return;

  project_title = strrchr (new_text, '/');
  if (project_title)
    {
      mx_entry_set_text (MX_ENTRY (cs->project_title),
                                   project_title + 1);
    }
  else
    {
      mx_entry_set_text (MX_ENTRY (cs->project_title),
                                   "-");
    }
  title_frozen = FALSE;
}

static void project_title_text_changed (ClutterActor *actor)
{
  const gchar *new_title = clutter_text_get_text (CLUTTER_TEXT (actor));
  gchar *path;

  if (title_frozen)
    return;

  path = g_strdup_printf ("%s/cluttersmith/%s",
                          g_get_user_special_dir (G_USER_DIRECTORY_DOCUMENTS),
                          new_title);
  cluttersmith_set_project_root (path);
  g_free (path);
}

static void scene_title_text_changed (ClutterActor *actor)
{
  const gchar *new_title = clutter_text_get_text (CLUTTER_TEXT (actor));

  if (title_frozen)
    return;
  title_frozen = TRUE;
  cs_context_set_scene (cs, new_title);
  title_frozen = FALSE;
}
