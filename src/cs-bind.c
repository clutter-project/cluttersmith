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
#include <glib/gprintf.h>

typedef struct Binding
{
  gint          refs;  
  GObject      *objectA;
  GObject      *objectB;
  const gchar  *propertyA;
  const gchar  *propertyB;
  gulong        A_changed_handler;
  gulong        B_changed_handler;

  gpointer      a_pre_changed_userdata;
  gboolean    (*a_pre_changed_callback)(GObject     *objectA,
                                        const gchar *propertyA,
                                        GObject     *objectB,
                                        const gchar *propertyB,
                                        gpointer     userdata);
  gpointer      a_post_changed_userdata;
  void        (*a_post_changed_callback)(GObject     *objectA,
                                         const gchar *propertyA,
                                         GObject     *objectB,
                                         const gchar *propertyB,
                                         gpointer     userdata);
  guint         a_changed_idle;
} Binding;

static void register_transforms (void);

static void object_vanished (gpointer data,
                             GObject *where_the_object_was)
{
  Binding *binding = data;
  binding->refs --;
  if (binding->a_changed_idle)
    {
      g_source_remove (binding->a_changed_idle);
      binding->a_changed_idle = 0;
    }
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

  if (binding->a_pre_changed_callback &&
      binding->a_pre_changed_callback (binding->objectA, binding->propertyA,
                                       binding->objectB, binding->propertyB,
                                       binding->a_pre_changed_userdata))
      return;

  g_value_init (&valueA, pspecA->value_type);
  g_value_init (&valueB, pspecB->value_type);

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

  if (binding->a_post_changed_callback)
    binding->a_post_changed_callback (binding->objectA, binding->propertyA,
                                      binding->objectB, binding->propertyB,
                                      binding->a_post_changed_userdata);
}

typedef struct DelayedUpdateAClosure {
  Binding *binding;
  GValue   valueA;
} DelayedUpdateAClosure;

static gboolean
delayed_update_A (gpointer ptr)
{
  DelayedUpdateAClosure *uac = ptr;
  Binding *binding = uac->binding;

  g_assert (G_IS_OBJECT (binding->objectA));

  g_signal_handler_block (binding->objectA, binding->A_changed_handler);
  g_object_set_property (binding->objectA, binding->propertyA, &uac->valueA);
  g_signal_handler_unblock (binding->objectA, binding->A_changed_handler);
  binding->a_changed_idle = 0;

  g_value_unset (&uac->valueA);
  g_free (ptr);
  return FALSE;
}

static void
cs_binding_B_changed (GObject    *objectB,
                      GParamSpec *arg1,
                      Binding    *binding)
{
  GParamSpec   *pspecA, *pspecB;
  GObjectClass *klassA, *klassB;
  GValue        valueB = {0, };
  DelayedUpdateAClosure *uac;

  if (binding->refs!=2)
    return;

  klassA = G_OBJECT_GET_CLASS (binding->objectA);
  klassB = G_OBJECT_GET_CLASS (binding->objectB);
  pspecA = g_object_class_find_property (klassA, binding->propertyA);
  pspecB = g_object_class_find_property (klassB, binding->propertyB);

  g_value_init (&valueB, pspecB->value_type);

  uac = g_new0 (DelayedUpdateAClosure, 1);
  uac->binding = binding;
  g_value_init (&uac->valueA, pspecA->value_type);
  g_object_get_property (objectB, binding->propertyB, &valueB);
  if (!g_value_transform (&valueB, &uac->valueA))
    {
      g_warning ("failed to transform value from %s to %s",
                  g_type_name (pspecB->value_type),
                  g_type_name (pspecA->value_type));
      g_value_unset (&uac->valueA);
      g_free (uac);
    }
  else
    {
      if (binding->a_changed_idle)
        g_source_remove (binding->a_changed_idle);
      binding->a_changed_idle = g_timeout_add (0, delayed_update_A, uac);
    }
  g_value_unset (&valueB);
}

static Binding *
cs_make_binding (GObject     *objectA,
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
  return binding;
}

/* This API is far to simple, but allows 1:1 binding of
 * gobject properties to each other.
 *
 * Flags:
 *   initialize_a_from_b
 *   propagate_a_to_b
 *   propagate_b_to_a
 *   propagate_both
 *
 * could optionally take an expression and (at least internally) take
 * an array of object/property pairs to depend on. And re-evaluate this
 * expression on change.
 *
 * Also need a way to replace/unregister such bindings, perhaps
 * return the binding struct/object?
 */

void
cs_bind (GObject     *objectA,
         const gchar *propertyA,
         GObject     *objectB,
         const gchar *propertyB)
{
  Binding *binding;
  binding = cs_make_binding (objectA, propertyA, objectB, propertyB);
}

void
cs_bind_full (GObject     *objectA,
              const gchar *propertyA,
              GObject     *objectB,
              const gchar *propertyB,
              gboolean    (*a_pre_changed_callback)(GObject     *objectA,
                                                    const gchar *propertyA,
                                                    GObject     *objectB,
                                                    const gchar *propertyB,
                                                    gpointer     userdata),
              gpointer     a_pre_changed_userdata,
              void        (*a_post_changed_callback)(GObject    *objectA,
                                                    const gchar *propertyA,
                                                    GObject     *objectB,
                                                    const gchar *propertyB,
                                                    gpointer     userdata),
              gpointer     a_post_changed_userdata)
{
  Binding      *binding;
  binding = cs_make_binding (objectA, propertyA, objectB, propertyB);
  binding->a_pre_changed_userdata = a_pre_changed_userdata;
  binding->a_pre_changed_callback = (void*)a_pre_changed_callback;
  binding->a_post_changed_userdata = a_post_changed_userdata;
  binding->a_post_changed_callback = (void*)a_post_changed_callback;
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

  /* These transforms are used in a last come dictates serving order
   * by glib.
   */

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
