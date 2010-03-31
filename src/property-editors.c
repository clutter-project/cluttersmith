#include <mx/mx.h>
#include <stdlib.h>
#include <math.h>
#include "cluttersmith.h"

typedef struct UpdateClosure
{
  GObject      *object;
  ClutterActor *editor;
  gchar        *property_name;
  gulong        update_editor_handler;
  gulong        update_object_handler;
} UpdateClosure;

static void object_vanished (gpointer data,
                             GObject *where_the_object_was)
{
  UpdateClosure *uc = data;
  uc->object = NULL;
}

static void update_closure_free (gpointer data, GClosure *closure)
{
  UpdateClosure *uc = data;
  if (uc->object)
    {
      if (uc->update_editor_handler)
        g_signal_handler_disconnect (uc->object, uc->update_editor_handler);
      g_object_weak_unref (uc->object, object_vanished, uc);
      uc->object = NULL;
    }
  g_free (uc->property_name);
  g_free (uc);
}


static void
update_editor_boolean (GObject       *object,
                       GParamSpec    *arg1,
                       UpdateClosure *uc)
{
  gboolean value;
  if (!uc->object)
    return;
  g_object_get (object, arg1->name, &value, NULL);
  if (uc->update_object_handler)
    g_signal_handler_block (uc->editor, uc->update_object_handler);
  mx_button_set_toggled (MX_BUTTON (uc->editor), value);
  mx_button_set_label (MX_BUTTON (uc->editor), value?"1":"0");
  if (uc->update_object_handler)
    g_signal_handler_unblock (uc->editor, uc->update_object_handler);
}

static gboolean
update_object_boolean (MxButton *button,
                       gpointer    data)
{
  UpdateClosure *uc = data;
  g_object_set (uc->object, uc->property_name, mx_button_get_toggled (button), NULL);
    {
      mx_button_set_label (button,  mx_button_get_toggled (button)?" 1 ":" 0 ");
    }
  cs_dirtied ();
  cs_prop_tweaked (uc->object, uc->property_name);
  return TRUE;
}




ClutterActor *property_editor_new (GObject *object,
                                   const gchar *property_name)
{
  GParamSpec *pspec;
  ClutterActor *editor = NULL;
  gchar *detailed_signal;

  pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (object), property_name);
  if (!pspec)
    {
      ClutterColor red={0xff,0,0,0xff};
      editor = clutter_rectangle_new ();
      clutter_rectangle_set_color (CLUTTER_RECTANGLE (editor), &red);
      return editor;
    }
 

  detailed_signal = g_strdup_printf ("notify::%s", pspec->name);

  /* Boolean values are special cased and get their own editor,
   * for all other types we rely on binding an MxEntry to the
   * respective value.
   *
   * (cs_bind increments the dirty count for the scene
   *  when the values bound to are changed.)
   */
  if (pspec->value_type == G_TYPE_BOOLEAN)
    {
      UpdateClosure *uc;

      uc = g_new0 (UpdateClosure, 1);
      uc->property_name = g_strdup (pspec->name);
      uc->object = object;

      editor = CLUTTER_ACTOR (mx_button_new_with_label ("-"));
      mx_button_set_is_toggle (MX_BUTTON (editor), TRUE);
      uc->editor = editor;
        uc->update_editor_handler = g_signal_connect (object, detailed_signal,
                                    G_CALLBACK (update_editor_boolean), uc);
      if ((pspec->flags & G_PARAM_WRITABLE))
      uc->update_object_handler =
        g_signal_connect_data (MX_BUTTON (editor), "clicked",
                             G_CALLBACK (update_object_boolean), uc,
                             update_closure_free, 0);
      update_editor_boolean (object, pspec, uc);
      
      g_object_weak_ref (object, object_vanished, uc);
    }
  else
    {
      GValue value = {0,};
      GValue str_value = {0,};
      g_value_init (&value, pspec->value_type);
      g_value_init (&str_value, G_TYPE_STRING);
      g_object_get_property (object, pspec->name, &value);
      if (g_value_transform (&value, &str_value))
        {
          editor = CLUTTER_ACTOR (mx_entry_new ());
          cs_bind (G_OBJECT (editor), "text", object, property_name);
        }
      else
        {
          g_print ("skipping %s", pspec->name);
        }
      g_value_unset (&value);
      g_value_unset (&str_value);
    }

  g_free (detailed_signal);
  return editor;
}

/**********************************************************************/

static ClutterColor  white = {0xff,0xff,0xff,0xff};  /* XXX: should be in CSS */

/* whitelist of properties for ClutterActor */
gchar *actor_property_whitelist[]={"depth",
                                   "opacity",
                                   "scale-x",
                                   "scale-y",
                                   "anchor-x",
                                   "color",
                                   "anchor-y",
                                   "rotation-angle-x",
                                   "rotation-angle-y",
                                   "rotation-angle-z",
                                   "name",
                                   "reactive",
                                   NULL};




void
props_populate (ClutterActor *container,
                GObject      *object,
                gboolean      easing_button)
{
  GParamSpec **properties;
  GParamSpec **actor_properties = NULL;
  guint        n_properties;
  guint        n_actor_properties = 0;
  gint         i;

  properties = g_object_class_list_properties (
                     G_OBJECT_GET_CLASS (object),
                     &n_properties);

  if (CLUTTER_IS_ACTOR (object))
    {
      actor_properties = g_object_class_list_properties (
                G_OBJECT_CLASS (g_type_class_ref (CLUTTER_TYPE_ACTOR)),
                &n_actor_properties);
    }


  for (i = 0; i < n_properties; i++)
    {
      gint j;
      gboolean skip = FALSE;

      for (j=0;j<n_actor_properties;j++)
        {
          /* ClutterActor contains so many properties that we restrict our view a bit */
          if (actor_properties[j]==properties[i])
            {
              gint k;
              skip = TRUE;
              for (k=0;actor_property_whitelist[k];k++)
                if (g_str_equal (properties[i]->name, actor_property_whitelist[k]))
                  skip = FALSE;
            }
        }

      if (!(properties[i]->flags & G_PARAM_READABLE))
        skip = TRUE;

      if (skip)
        continue;

      {
        ClutterActor *hbox = g_object_new (MX_TYPE_BOX_LAYOUT, NULL);
        ClutterActor *label = mx_label_new ();
        ClutterActor *editor = property_editor_new (object, properties[i]->name);

        mx_label_set_text (MX_LABEL (label), properties[i]->name);

        clutter_container_add_actor (CLUTTER_CONTAINER (hbox), label);
        clutter_actor_set_width (label, CS_PROPEDITOR_LABEL_WIDTH);
        clutter_container_add_actor (CLUTTER_CONTAINER (hbox), editor);
        clutter_container_child_set (CLUTTER_CONTAINER (hbox), editor, "expand", TRUE, NULL);

        if (easing_button)
          {
            ClutterActor *ease = mx_button_new ();
            clutter_container_add_actor (CLUTTER_CONTAINER (hbox), ease);
          }
        clutter_container_add_actor (CLUTTER_CONTAINER (container), hbox);
      }
    }

  /* should be split inot its own function */
  if (CLUTTER_IS_ACTOR (object))
  {
    ClutterActor *actor = CLUTTER_ACTOR (object);
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
                    ClutterActor *hbox = g_object_new (MX_TYPE_BOX_LAYOUT, NULL);
                    ClutterActor *label = clutter_text_new_with_text (CS_EDITOR_LABEL_FONT, child_properties[i]->name);
                    ClutterActor *editor = property_editor_new (G_OBJECT (child_meta), child_properties[i]->name);
                    clutter_text_set_color (CLUTTER_TEXT (label), &white);
                    clutter_text_set_ellipsize (CLUTTER_TEXT (label), PANGO_ELLIPSIZE_MIDDLE);
                    clutter_actor_set_size (label, CS_PROPEDITOR_LABEL_WIDTH, EDITOR_LINE_HEIGHT);
                    clutter_actor_set_size (editor, CS_PROPEDITOR_EDITOR_WIDTH, EDITOR_LINE_HEIGHT);
                    clutter_container_add_actor (CLUTTER_CONTAINER (container), label);
                    clutter_container_add_actor (CLUTTER_CONTAINER (hbox), editor);
                    clutter_container_child_set (CLUTTER_CONTAINER (hbox), editor, "expand", TRUE, NULL);
                    clutter_container_add_actor (CLUTTER_CONTAINER (container), hbox);
                  }
              }
            g_free (child_properties);
          }
      }
  }

  g_free (properties);
}
