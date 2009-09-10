#include <nbtk/nbtk.h>
#include <stdlib.h>
#include <math.h>

extern guint CB_REV;

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
  g_print ("object for %s gone\n", uc->property_name);
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
update_editor_double (GObject       *object,
                      GParamSpec    *arg1,
                      UpdateClosure *uc)
{
  gdouble value;
  gchar *str;
  if (!uc->object || !uc->editor)
    return;
  g_object_get (object, arg1->name, &value, NULL);
  str = g_strdup_printf ("%2.1f", value);
  if (uc->update_object_handler)
    g_signal_handler_block (nbtk_entry_get_clutter_text (NBTK_ENTRY (uc->editor)),
                            uc->update_object_handler);
  nbtk_entry_set_text (NBTK_ENTRY (uc->editor), str);
  if (uc->update_object_handler)
    g_signal_handler_unblock (nbtk_entry_get_clutter_text (NBTK_ENTRY(uc->editor)), uc->update_object_handler);
  g_free (str);
}

static void
update_editor_float (GObject       *object,
                     GParamSpec    *arg1,
                     UpdateClosure *uc)
{
  gfloat value;
  gchar *str;
  if (!uc->object || !uc->editor)
    return;
  g_object_get (object, arg1->name, &value, NULL);
  str = g_strdup_printf ("%2.1f", value);
  if (uc->update_object_handler)
    g_signal_handler_block (nbtk_entry_get_clutter_text (NBTK_ENTRY(uc->editor)), uc->update_object_handler);
  nbtk_entry_set_text (NBTK_ENTRY (uc->editor), str);
  if (uc->update_object_handler)
    g_signal_handler_unblock (nbtk_entry_get_clutter_text (NBTK_ENTRY(uc->editor)), uc->update_object_handler);
  g_free (str);
}

static gboolean
update_object_float (ClutterText *text,
                     gpointer     data)
{
  UpdateClosure *uc = data;
  const gchar *str = clutter_text_get_text (text);
  gdouble value = g_strtod (str, NULL);
  g_object_set (uc->object, uc->property_name, value, NULL);
  CB_REV++;
  return TRUE;
}

static void
update_editor_uchar (GObject       *object,
                     GParamSpec    *arg1,
                     UpdateClosure *uc)
{
  guchar value;
  gchar *str;
  if (!uc->object)
    return;
  g_object_get (object, arg1->name, &value, NULL);
  str = g_strdup_printf ("%i", (gint)value);
  if (uc->update_object_handler)
    g_signal_handler_block (nbtk_entry_get_clutter_text (NBTK_ENTRY(uc->editor)), uc->update_object_handler);
  nbtk_entry_set_text (NBTK_ENTRY (uc->editor), str);
  if (uc->update_object_handler)
    g_signal_handler_unblock (nbtk_entry_get_clutter_text (NBTK_ENTRY(uc->editor)), uc->update_object_handler);
  g_free (str);
}

static void
update_editor_int (GObject       *object,
                   GParamSpec    *arg1,
                   UpdateClosure *uc)
{
  gint value;
  gchar *str;
  if (!uc->object)
    return;
  g_object_get (object, arg1->name, &value, NULL);
  str = g_strdup_printf ("%i", value);
  if (uc->update_object_handler)
    g_signal_handler_block (nbtk_entry_get_clutter_text (NBTK_ENTRY(uc->editor)), uc->update_object_handler);
  nbtk_entry_set_text (NBTK_ENTRY (uc->editor), str);
  if (uc->update_object_handler)
    g_signal_handler_unblock (nbtk_entry_get_clutter_text (NBTK_ENTRY(uc->editor)), uc->update_object_handler);
  g_free (str);
}

static gboolean
update_object_int (ClutterText *text,
                   gpointer     data)
{
  UpdateClosure *uc = data;
  if (!uc->object)
    return TRUE;
  g_object_set (uc->object, uc->property_name, atoi (clutter_text_get_text (text)), NULL);
  CB_REV++;
  return TRUE;
}

static void
update_editor_uint (GObject       *object,
                    GParamSpec    *arg1,
                    UpdateClosure *uc)
{
  guint value;
  gchar *str;
  if (!uc->object)
    return;
  g_object_get (object, arg1->name, &value, NULL);
  str = g_strdup_printf ("%u", value);
  if (uc->update_object_handler)
    g_signal_handler_block (nbtk_entry_get_clutter_text (NBTK_ENTRY(uc->editor)), uc->update_object_handler);
  nbtk_entry_set_text (NBTK_ENTRY (uc->editor), str);
  if (uc->update_object_handler)
    g_signal_handler_unblock (nbtk_entry_get_clutter_text (NBTK_ENTRY(uc->editor)), uc->update_object_handler);
  g_free (str);
}

static gboolean
update_object_uint (ClutterText *text,
                    gpointer     data)
{
  UpdateClosure *uc = data;
  g_object_set (uc->object, uc->property_name, atoi (clutter_text_get_text (text)), NULL);
  return TRUE;
}

static void
update_editor_string (GObject       *object,
                      GParamSpec    *arg1,
                      UpdateClosure *uc)
{
  gchar *value;
  if (!uc->object)
    return;
  g_object_get (object, arg1->name, &value, NULL);
  if (uc->update_object_handler)
    g_signal_handler_block (nbtk_entry_get_clutter_text (NBTK_ENTRY(uc->editor)), uc->update_object_handler);
  nbtk_entry_set_text (NBTK_ENTRY (uc->editor), value);
  if (uc->update_object_handler)
    g_signal_handler_unblock (nbtk_entry_get_clutter_text (NBTK_ENTRY(uc->editor)), uc->update_object_handler);
  g_free (value);
}

static gboolean
update_object_string (ClutterText *text,
                      gpointer     data)
{
  UpdateClosure *uc = data;
  g_object_set (uc->object, uc->property_name, clutter_text_get_text (text), NULL);
  CB_REV++;
  return TRUE;
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
  nbtk_button_set_checked (NBTK_BUTTON (uc->editor), value);
  nbtk_button_set_label (NBTK_BUTTON (uc->editor), value?"1":"0");
  if (uc->update_object_handler)
    g_signal_handler_unblock (uc->editor, uc->update_object_handler);
}



static gboolean
update_object_boolean (NbtkButton *button,
                       gpointer    data)
{
  UpdateClosure *uc = data;
  g_object_set (uc->object, uc->property_name, nbtk_button_get_checked (button), NULL);
    {
      nbtk_button_set_label (button,  nbtk_button_get_checked (button)?" 1 ":" 0 ");
    }
  CB_REV++;
  return TRUE;
}


static gboolean
update_object_generic (ClutterText *text,
                       gpointer     data)
{
  GParamSpec *pspec;
  GValue value = {0,};
  GValue str_value = {0,};
  UpdateClosure *uc = data;

  pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (uc->object), uc->property_name);

  g_value_init (&str_value, G_TYPE_STRING);
  g_value_set_string (&str_value, clutter_text_get_text (text));

  g_value_init (&value, pspec->value_type);

  if (g_value_transform (&str_value, &value))
    {
      g_object_set_property (G_OBJECT (uc->object), uc->property_name, &value);
    }
  g_value_unset (&value);
  g_value_unset (&str_value);
  CB_REV++;
  return TRUE;
}


ClutterActor *property_editor_new (GObject *object,
                                   const gchar *property_name)
{
  GParamSpec *pspec;
  ClutterActor *editor = NULL;
  gchar *detailed_signal;
  UpdateClosure *uc;

  pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (object), property_name);
  if (!pspec)
    return clutter_rectangle_new ();
 
  uc = g_new0 (UpdateClosure, 1);
  uc->property_name = g_strdup (pspec->name);
  uc->object = object;

  detailed_signal = g_strdup_printf ("notify::%s", pspec->name);
  g_object_weak_ref (object, object_vanished, uc);

  if (pspec->value_type == G_TYPE_FLOAT)
    {
      editor = CLUTTER_ACTOR (nbtk_entry_new (""));
      uc->editor = editor;
        uc->update_editor_handler = g_signal_connect (object, detailed_signal,
                                    G_CALLBACK (update_editor_float), uc);
      if ((pspec->flags & G_PARAM_WRITABLE))
      uc->update_object_handler =
        g_signal_connect_data (nbtk_entry_get_clutter_text (
                             NBTK_ENTRY (editor)), "text-changed",
                             G_CALLBACK (update_object_float), uc,
                             update_closure_free, 0);


      update_editor_float (object, pspec, uc);
    }
  else if (pspec->value_type == G_TYPE_DOUBLE)
    {
      editor = CLUTTER_ACTOR (nbtk_entry_new (""));
      uc->editor = editor;
        uc->update_editor_handler = g_signal_connect (object, detailed_signal,
                                    G_CALLBACK (update_editor_double), uc);
      if ((pspec->flags & G_PARAM_WRITABLE))
      uc->update_object_handler =
        g_signal_connect_data (nbtk_entry_get_clutter_text (
                             NBTK_ENTRY (editor)), "text-changed",
                             G_CALLBACK (update_object_float), uc,
                             update_closure_free, 0);

      update_editor_double (object, pspec, uc);
    }
  else if (pspec->value_type == G_TYPE_UCHAR)
    {
      editor = CLUTTER_ACTOR (nbtk_entry_new (""));
      uc->editor = editor;
        uc->update_editor_handler = g_signal_connect (object, detailed_signal,
                                    G_CALLBACK (update_editor_uchar), uc);
      if ((pspec->flags & G_PARAM_WRITABLE))
      uc->update_object_handler =
        g_signal_connect_data (nbtk_entry_get_clutter_text (
                             NBTK_ENTRY (editor)), "text-changed",
                             G_CALLBACK (update_object_int), uc,
                             update_closure_free, 0);

      update_editor_uchar (object, pspec, uc);
    }
  else if (pspec->value_type == G_TYPE_INT)
    {
      editor = CLUTTER_ACTOR (nbtk_entry_new (""));
      uc->editor = editor;
        uc->update_editor_handler = g_signal_connect (object, detailed_signal,
                                    G_CALLBACK (update_editor_int), uc);
      if ((pspec->flags & G_PARAM_WRITABLE))
      uc->update_object_handler =
        g_signal_connect_data (nbtk_entry_get_clutter_text (
                             NBTK_ENTRY (editor)), "text-changed",
                             G_CALLBACK (update_object_int), uc,
                             update_closure_free, 0);

      update_editor_int (object, pspec, uc);
    }
  else if (pspec->value_type == G_TYPE_UINT)
    {
      editor = CLUTTER_ACTOR (nbtk_entry_new (""));
      uc->editor = editor;
        uc->update_editor_handler = g_signal_connect (object, detailed_signal,
                                    G_CALLBACK (update_editor_uint), uc);
      if ((pspec->flags & G_PARAM_WRITABLE))
      uc->update_object_handler =
        g_signal_connect_data (nbtk_entry_get_clutter_text (
                             NBTK_ENTRY (editor)), "text-changed",
                             G_CALLBACK (update_object_uint), uc,
                             update_closure_free, 0);

      update_editor_uint (object, pspec, uc);
    }
  else if (pspec->value_type == G_TYPE_STRING)
    {
      editor = CLUTTER_ACTOR (nbtk_entry_new (""));
      uc->editor = editor;
        uc->update_editor_handler = g_signal_connect (object, detailed_signal,
                                    G_CALLBACK (update_editor_string), uc);
      if ((pspec->flags & G_PARAM_WRITABLE))
      uc->update_object_handler =
        g_signal_connect_data (nbtk_entry_get_clutter_text (
                             NBTK_ENTRY (editor)), "text-changed",
                             G_CALLBACK (update_object_string), uc,
                             update_closure_free, 0);
      update_editor_string (object, pspec, uc);
    }
  else if (pspec->value_type == G_TYPE_BOOLEAN)
    {
      editor = CLUTTER_ACTOR (nbtk_button_new_with_label ("-"));
      nbtk_button_set_toggle_mode (NBTK_BUTTON (editor), TRUE);
      uc->editor = editor;
        uc->update_editor_handler = g_signal_connect (object, detailed_signal,
                                    G_CALLBACK (update_editor_boolean), uc);
      if ((pspec->flags & G_PARAM_WRITABLE))
      uc->update_object_handler =
        g_signal_connect_data (NBTK_BUTTON (editor), "clicked",
                             G_CALLBACK (update_object_boolean), uc,
                             update_closure_free, 0);
      update_editor_boolean (object, pspec, uc);
    }
  else
    {
      GValue value = {0,};
      GValue str_value = {0,};
      gchar *initial;
      g_value_init (&value, pspec->value_type);
      g_value_init (&str_value, G_TYPE_STRING);
      g_object_get_property (object, pspec->name, &value);
      if (g_value_transform (&value, &str_value))
        {
          initial = g_strdup_printf ("%s", g_value_get_string (&str_value));
          editor = CLUTTER_ACTOR (nbtk_entry_new (initial));
          uc->editor = editor;
          if ((pspec->flags & G_PARAM_WRITABLE))
            g_signal_connect_data (nbtk_entry_get_clutter_text (NBTK_ENTRY (editor)), "text-changed",
                                   G_CALLBACK (update_object_generic), uc, update_closure_free, 0);
        }
      else
        {
          initial = g_strdup_printf ("%s", g_type_name (pspec->value_type));
          editor = clutter_text_new_with_text ("Sans 12px", initial);
          uc->editor = editor;
          update_closure_free (uc, NULL);
        }
      g_value_unset (&value);
      g_value_unset (&str_value);
      g_free (initial);
    }



  g_free (detailed_signal);
  return editor;
}
