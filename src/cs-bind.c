#include "cluttersmith.h"
#include <glib/gprintf.h>

typedef struct Binding
{
  gint         refs;  
  GObject     *objectA;
  GObject     *objectB;
  const gchar *propertyA;
  const gchar *propertyB;
  gulong       A_changed_handler;
  gulong       B_changed_handler;
} Binding;

static void register_transforms (void);

static void object_vanished (gpointer data,
                             GObject *where_the_object_was)
{
  Binding *binding = data;
  binding->refs --;
  if (binding->refs == 0)
    g_free (binding);

  /* With this memory management, we will leak inert bindings
   * for all generated properties when the selection changes
   * in cluttersmith, these leaks are cleared on scene changes
   */
}

static void
cs_binding_A_changed (GObject    *objectA,
                      GParamSpec *arg1,
                      Binding    *binding)
{
  GParamSpec   *pspecA, *pspecB;
  GObjectClass *klassA, *klassB;
  GValue        valueA = {0, };
  GValue        valueB = {0, };

  if (binding->refs != 2)
    return;

  klassA = G_OBJECT_GET_CLASS (binding->objectA);
  pspecA = g_object_class_find_property (klassA, binding->propertyA);
  klassB = G_OBJECT_GET_CLASS (binding->objectB);
  pspecB = g_object_class_find_property (klassB, binding->propertyB);

  g_value_init (&valueB, pspecB->value_type);
  g_value_init (&valueA, pspecA->value_type);

  g_object_get_property (objectA, binding->propertyA, &valueA);
  if (!g_value_transform (&valueA, &valueB))
     g_warning ("failed to transform value from %s to %s",
                 g_type_name (pspecA->value_type),
                 g_type_name (pspecB->value_type));
  else
    {
      g_signal_handler_block (binding->objectB, binding->B_changed_handler);
      g_object_set_property (binding->objectB, binding->propertyB, &valueB);
      g_signal_handler_unblock (binding->objectB, binding->B_changed_handler);
    }

  g_value_unset (&valueA);
  g_value_unset (&valueB);

  cs_dirtied ();
  cs_prop_tweaked (binding->objectB, binding->propertyB);
}

static void
cs_binding_B_changed (GObject    *objectB,
                      GParamSpec *arg1,
                      Binding    *binding)
{
  GParamSpec   *pspecA, *pspecB;
  GObjectClass *klassA, *klassB;
  GValue        valueA = {0, };
  GValue        valueB = {0, };

  if (binding->refs!=2)
    return;

  klassA = G_OBJECT_GET_CLASS (binding->objectA);
  klassB = G_OBJECT_GET_CLASS (binding->objectB);
  pspecA = g_object_class_find_property (klassA, binding->propertyA);
  pspecB = g_object_class_find_property (klassB, binding->propertyB);

  g_value_init (&valueB, pspecB->value_type);
  g_value_init (&valueA, pspecA->value_type);

  g_object_get_property (objectB, binding->propertyB, &valueB);
  if (!g_value_transform (&valueB, &valueA))
     g_warning ("failed to transform value from %s to %s",
                 g_type_name (pspecB->value_type),
                 g_type_name (pspecA->value_type));
  else
    {
      g_signal_handler_block (binding->objectA, binding->A_changed_handler);
      g_object_set_property (binding->objectA, binding->propertyA, &valueA);
      g_signal_handler_unblock (binding->objectA, binding->A_changed_handler);
    }
  g_value_unset (&valueA);
  g_value_unset (&valueB);
}

void
cs_bind (GObject     *objectA,
         const gchar *propertyA,
         GObject     *objectB,
         const gchar *propertyB)
{
  Binding      *binding;
  gchar        *detailed_signalA;
  gchar        *detailed_signalB;

  register_transforms ();

  binding = g_new0 (Binding, 1);
  binding->refs = 2;
  binding->objectA = objectA;
  binding->objectB = objectB;
  binding->propertyA = g_intern_string (propertyA);
  binding->propertyB = g_intern_string (propertyB);

  detailed_signalA = g_strdup_printf ("notify::%s", propertyA);
  detailed_signalB = g_strdup_printf ("notify::%s", propertyB);

  binding->A_changed_handler = g_signal_connect (objectA, detailed_signalA,
                                             G_CALLBACK (cs_binding_A_changed),
                                             binding);
  binding->B_changed_handler = g_signal_connect (objectB, detailed_signalB,
                                             G_CALLBACK (cs_binding_B_changed),
                                             binding);
  g_free (detailed_signalA);
  g_free (detailed_signalB);

  cs_binding_B_changed (objectB, NULL, binding);

  g_object_weak_ref (objectA, object_vanished, binding);
  g_object_weak_ref (objectB, object_vanished, binding);
}

static void cs_string_to_double (const GValue *src_value,
                                 GValue       *dest_value)
{
  g_value_set_double (dest_value,
                      g_strtod (g_value_get_string (src_value), NULL));
}

static void cs_string_to_float (const GValue *src_value,
                                GValue       *dest_value)
{
  g_value_set_float (dest_value,
                     g_strtod (g_value_get_string (src_value), NULL));
}

static void cs_float_to_string (const GValue *src_value,
                                GValue       *dest_value)
{
  gchar buf[256];
  g_sprintf (buf, "%.3f", g_value_get_float (src_value));
  g_value_set_string (dest_value, buf);
}

static void cs_double_to_string (const GValue *src_value,
                                 GValue       *dest_value)
{
  gchar buf[256];
  g_sprintf (buf, "%.3f", g_value_get_double (src_value));
  g_value_set_string (dest_value, buf);
}

static void cs_string_to_int (const GValue *src_value,
                              GValue       *dest_value)
{
  g_value_set_int (dest_value,
                   g_strtod (g_value_get_string (src_value), NULL));
}

static void cs_string_to_uint (const GValue *src_value,
                               GValue       *dest_value)
{
  g_value_set_uint (dest_value,
                    g_strtod (g_value_get_string (src_value), NULL));
}

static void
register_transforms (void)
{
  static gboolean done = FALSE;
  if (done)
    return;
  done = TRUE;

  g_value_register_transform_func (G_TYPE_FLOAT, G_TYPE_STRING,
                                   cs_float_to_string);
  g_value_register_transform_func (G_TYPE_DOUBLE, G_TYPE_STRING,
                                   cs_double_to_string);
  g_value_register_transform_func (G_TYPE_STRING, G_TYPE_FLOAT,
                                   cs_string_to_float);
  g_value_register_transform_func (G_TYPE_STRING, G_TYPE_DOUBLE,
                                   cs_string_to_double);
  g_value_register_transform_func (G_TYPE_STRING, G_TYPE_INT,
                                   cs_string_to_int);
  g_value_register_transform_func (G_TYPE_STRING, G_TYPE_UINT,
                                   cs_string_to_uint);
}
