/*
 * Clutter.
 *
 * An OpenGL based 'interactive canvas' library.
 *
 * Authored By Øyvind Kolås <pippin@linux.intel.com>
 *
 * Copyright (C) 2009 Intel Corporation
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __CLUTTER_STATES_H__
#define __CLUTTER_STATES_H__

#include <clutter/clutter.h>

G_BEGIN_DECLS

#define CLUTTER_TYPE_STATES (clutter_states_get_type ())

#define CLUTTER_STATES(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  CLUTTER_TYPE_STATES, ClutterStates))

#define CLUTTER_STATES_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  CLUTTER_TYPE_STATES, ClutterStatesClass))

#define CLUTTER_IS_STATES(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  CLUTTER_TYPE_STATES))

#define CLUTTER_IS_STATES_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  CLUTTER_TYPE_STATES))

#define CLUTTER_STATES_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  CLUTTER_TYPE_STATES, ClutterStatesClass))

typedef struct _ClutterStates        ClutterStates;
typedef struct _ClutterStatesPrivate ClutterStatesPrivate;
typedef struct _ClutterStatesClass   ClutterStatesClass;

/**
 * ClutterStates:
 *
 * The #ClutterStates structure contains only private data and
 * should be accessed using the provided API
 *
 * Since: 1.2
 */
struct _ClutterStates
{
  /*< private >*/
  GObject        parent;
  ClutterStatesPrivate *priv;
};

/**
 * ClutterStatesClass:
 *
 * The #ClutterStatesClass structure contains only private data
 *
 * Since: 1.2
 */
struct _ClutterStatesClass
{
  /*< private >*/
  GObjectClass parent_class;

  /* padding for future expansion */
  gpointer _padding_dummy[16];
};

GType clutter_states_get_type (void) G_GNUC_CONST;
ClutterStates   *clutter_states_new                   (void);

typedef struct _ClutterStateKey ClutterStateKey;
  
gdouble           clutter_state_key_get_pre_delay         (ClutterStateKey *state_key);
gdouble           clutter_state_key_get_post_delay        (ClutterStateKey *state_key);
gulong            clutter_state_key_get_mode              (ClutterStateKey *state_key);
void              clutter_state_key_get_value             (ClutterStateKey *state_key,
                                                           GValue          *value);
GObject         * clutter_state_key_get_object            (ClutterStateKey *state_key);
const gchar     * clutter_state_key_get_property_name     (ClutterStateKey *state_key);
const gchar     * clutter_state_key_get_source_state_name (ClutterStateKey *state_key);
const gchar     * clutter_state_key_get_target_state_name (ClutterStateKey *state_key);

ClutterStates   * clutter_states_new                  (void);
ClutterTimeline * clutter_states_change               (ClutterStates   *states,
                                                       const gchar     *target_state_name);
ClutterTimeline * clutter_states_change_noanim        (ClutterStates   *states,
                                                       const gchar     *target_state_name);
ClutterStates *   clutter_states_set_key              (ClutterStates   *states,
                                                       const gchar     *source_state_name,
                                                       const gchar     *target_state_name,
                                                       GObject         *object,
                                                       const gchar     *property_name,
                                                       gulong           mode,
                                                       const GValue    *value,
                                                       gdouble          pre_delay,
                                                       gdouble          post_delay);

void              clutter_states_set_duration         (ClutterStates   *states,
                                                       const gchar     *source_state_name,
                                                       const gchar     *target_state_name,
                                                       guint            duration);
guint             clutter_states_get_duration         (ClutterStates   *states,
                                                       const gchar     *source_state_name,
                                                       const gchar     *target_state_name);

void              clutter_states_set                  (ClutterStates   *states,
                                                       const gchar     *source_state_name,
                                                       const gchar     *target_state_name,
                                                       gpointer         first_object,
                                                       const gchar     *first_property_name,
                                                       gulong           first_mode,
                                                       ...);
GList           * clutter_states_get_states           (ClutterStates   *states);
GList           * clutter_states_get_keys             (ClutterStates   *states,
                                                       const gchar     *source_state,
                                                       const gchar     *target_state,
                                                       GObject         *object,
                                                       const gchar     *property_name);
void              clutter_states_remove               (ClutterStates   *states,
                                                       const gchar     *source_state,
                                                       const gchar     *target_state,
                                                       GObject         *object,
                                                       const gchar     *property_name);
ClutterTimeline * clutter_states_get_timeline         (ClutterStates   *states);
void              clutter_states_set_animator         (ClutterStates   *states,
                                                       const gchar     *source_state,
                                                       const gchar     *target_state,
                                                       ClutterAnimator *animator);
ClutterAnimator * clutter_states_get_animator         (ClutterStates   *states,
                                                       const gchar     *source_state,
                                                       const gchar     *target_state);


G_END_DECLS

#endif /* __CLUTTER_STATES_H__ */
