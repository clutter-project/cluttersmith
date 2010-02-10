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


/**
 * SECTION:clutter-states
 * @short_description: Multi state tweener
 *
 * #ClutterStates controls the tweening of properties on multiple
 * actors between a set of named states.
 */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "clutter-states.h"
#include <gobject/gvaluecollector.h>
#include <string.h>

G_DEFINE_TYPE (ClutterStates, clutter_states, G_TYPE_OBJECT);

typedef struct ClutterStateAnimator {
  const gchar     *source_state_name;
  ClutterAnimator *animator;
} ClutterStateAnimator;

typedef struct ClutterState { 
  ClutterStates *states;
  const gchar   *name;
  GList         *keys;
  GArray        *animators;
} ClutterState;

struct _ClutterStatesPrivate
{
  ClutterTimeline *timeline;
  ClutterTimeline *slave_timeline;
  const gchar     *source_state_name; 
  const gchar     *target_state_name; 
  ClutterState    *source_state;
  ClutterState    *target_state;
  GHashTable      *states;
  ClutterAnimator *current_animator;
};

/**
 * ClutterStateKey:
 *
 * An opaque data structure with accessor functions.
 *
 */
typedef struct _ClutterStateKey
{ 
  GObject         *object;
  const gchar     *property_name;
  gulong           mode;
  GValue           value;
  gdouble          pre_delay;
  gdouble          post_delay;

  /* The following fields are fully internal and not publicly accesible */

  ClutterState    *source_state; 
  ClutterState    *target_state; 
  gboolean         inert; 
  ClutterAlpha    *alpha;
  ClutterInterval *interval;
} _ClutterStateKey;


#define CLUTTER_STATES_GET_PRIVATE(obj)    \
              (G_TYPE_INSTANCE_GET_PRIVATE ((obj),    \
               CLUTTER_TYPE_STATES,        \
               ClutterStatesPrivate))

/**
 * clutter_states_new:
 *
 * Create a new #ClutterStates instance.
 *
 * Returns: a new #ClutterStates.
 */
ClutterStates *
clutter_states_new (void)
{
  return g_object_new (CLUTTER_TYPE_STATES, NULL);
}

static ClutterState * state_new (ClutterStates *states,
                                 const gchar   *name);

static gint sort_props_func (gconstpointer a,
                             gconstpointer b)
{
  const ClutterStateKey *pa = a;
  const ClutterStateKey *pb = b;

  if (pa->object == pb->object)
    {
      gint propnamediff = pa->property_name-pb->property_name;
      if (propnamediff == 0)
        {
         return pb->source_state - pa->source_state;
        }
      return propnamediff;
    }
  return pa->object - pb->object;
}

static void object_disappeared (gpointer data,
                                GObject *where_the_object_was)
{
  clutter_states_remove (data, NULL, NULL, (void*)where_the_object_was, NULL);
}

static ClutterStateKey *
clutter_state_key_new (ClutterState  *state,
                       GObject       *object,
                       const gchar   *property_name,
                       guint          mode)
{
  ClutterStateKey *state_key;
  GParamSpec      *pspec;
  GObjectClass *klass = G_OBJECT_GET_CLASS (object);
  GValue value = {0, };

  state_key = g_slice_new0 (ClutterStateKey);
  state_key->target_state = state;
  state_key->object = object;
  state_key->property_name = g_intern_string (property_name);
  state_key->mode = mode;
  state_key->alpha = clutter_alpha_new ();
  clutter_alpha_set_mode (state_key->alpha, mode);
  clutter_alpha_set_timeline (state_key->alpha,
                              state->states->priv->slave_timeline);

  pspec = g_object_class_find_property (klass, property_name);
          
  state_key->interval =
      g_object_new (CLUTTER_TYPE_INTERVAL,
                    "value-type", G_PARAM_SPEC_VALUE_TYPE (pspec),
                    NULL);
  g_value_init (&value, G_PARAM_SPEC_VALUE_TYPE (pspec));
  clutter_interval_set_initial_value (state_key->interval, &value);
  clutter_interval_set_final_value (state_key->interval, &value);
  g_value_unset (&value);

  g_object_ref_sink (state_key->alpha);
  g_object_ref_sink (state_key->interval);

  g_object_weak_ref (object, object_disappeared,
                     state_key->target_state->states);

  return (void*)state_key;
}

static void
clutter_state_key_free (gpointer clutter_state_key)
{
  ClutterStateKey *key = clutter_state_key;

  if (!key->inert)
    g_object_weak_unref (key->object, object_disappeared,
                         key->target_state->states);
  g_object_unref (key->alpha);
  g_object_unref (key->interval);

  g_slice_free (ClutterStateKey, key);
}

static void state_free (gpointer data)
{
  ClutterState *state = data;
  for (; state->keys;
       state->keys = g_list_remove (state->keys, state->keys->data))
    clutter_state_key_free (state->keys->data);
  g_array_unref (state->animators);
  g_free (state);
}

static ClutterState *state_new (ClutterStates *states,
                                const gchar   *name)
{
  ClutterState *state;
  state         = g_new0 (ClutterState, 1);
  state->states = states;
  state->name   = name;
  state->animators = g_array_new (TRUE, TRUE, sizeof (ClutterStateAnimator));
  return state;
}

static void 
clutter_states_finalize (GObject *object)
{
  ClutterStates *states = CLUTTER_STATES (object);
  g_hash_table_destroy (states->priv->states);
  g_object_unref (states->priv->timeline);
  g_object_unref (states->priv->slave_timeline);

  G_OBJECT_CLASS (clutter_states_parent_class)->finalize (object);
}

static void clutter_states_completed (ClutterTimeline *timeline,
                                      ClutterStates   *states)
{
  if (states->priv->current_animator)
    {
      clutter_animator_set_timeline (states->priv->current_animator,
                                     NULL);
      states->priv->current_animator = NULL;
    }
}

static void clutter_states_new_frame (ClutterTimeline *timeline,
                                      gint             msecs,
                                      ClutterStates   *states)
{
  GList       *k;
  gdouble      progress;
  const gchar *curprop = NULL;
  GObject     *curobj = NULL;
  gboolean     found_specific = FALSE;

  if (states->priv->current_animator)
    return;

  progress  = 1.0 * msecs / clutter_timeline_get_duration (timeline);

  for (k = states->priv->target_state->keys; k; k = k->next)
    {
      ClutterStateKey *key = k->data;
      GValue           value = {0,};
      gdouble          sub_progress;

      if ((curprop && !(curprop == key->property_name)) ||
          key->object != curobj)
        {
          curprop = key->property_name;
          curobj = key->object;
          found_specific = FALSE;
        }

      if (!found_specific)
        {
          if (key->source_state &&
              key->source_state->name &&
              g_str_equal (states->priv->source_state_name,
                           key->source_state->name))
            {
              found_specific = TRUE;
            }

          if (found_specific || key->source_state == NULL)
            {
              sub_progress = (progress - key->pre_delay) /
                             (1.0 - (key->pre_delay + key->post_delay));

              if (sub_progress >= 0.0)
                {
                  if (sub_progress >= 1.0)
                    sub_progress = 1.0;
                  clutter_timeline_advance (states->priv->slave_timeline,
                                            sub_progress * 10000);
                  sub_progress = clutter_alpha_get_alpha (key->alpha);

                  g_value_init (&value,
                               clutter_interval_get_value_type (key->interval));
                  clutter_interval_compute_value (key->interval, sub_progress,
                                                  &value);
                  g_object_set_property (key->object, key->property_name,
                                         &value);
                  g_value_unset (&value);
                }
              /* XXX: should the target value of the default destination be
               * used even when found a specific source_state key?
               */
            }
        }
    }
}

/**
 * clutter_states_change:
 * @state_name: a #ClutterStates
 * @duration: how long the state change should take in msecs.
 *
 * Change to @state_name and spend duration msecs when doing so.
 *
 * Return value: the #ClutterTimeline that drives the #ClutterStates instance.
 */
ClutterTimeline *
clutter_states_change (ClutterStates *states,
                       const gchar   *target_state_name,
                       guint          duration)
{
  ClutterStatesPrivate *priv = states->priv;
  ClutterState *state;

  g_return_val_if_fail (CLUTTER_IS_STATES (states), NULL);
  g_return_val_if_fail (target_state_name, NULL);

  if (target_state_name == NULL)
    target_state_name = "default";
  target_state_name = g_intern_string (target_state_name);
  if (priv->target_state_name == NULL)
    priv->target_state_name = g_intern_static_string ("default");

  if (target_state_name == priv->target_state_name)
    {
      /* Avoiding transitioning if the desired state
       * is already current
       */
      return priv->timeline;
    }

  if (priv->current_animator)
    {
      clutter_animator_set_timeline (priv->current_animator, NULL);
      priv->current_animator = NULL;
    }


  clutter_timeline_set_duration (priv->timeline, duration);

  priv->source_state_name = priv->target_state_name;
  priv->target_state_name = target_state_name;

  state = g_hash_table_lookup (priv->states, target_state_name);

  g_return_val_if_fail (state, NULL);

  {
    ClutterAnimator *animator;
    animator = clutter_states_get_animator (states, priv->source_state_name,
                                                    priv->target_state_name);
    if (animator)
      {
        priv->current_animator = animator;
        clutter_animator_set_timeline (animator, priv->timeline);
        clutter_timeline_stop (priv->timeline);
        clutter_timeline_rewind (priv->timeline);
        clutter_timeline_start (priv->timeline);
        return priv->timeline;
      }
  }

  if (state)
    {
      GList *k;

      for (k = state->keys; k; k = k->next)
        {
          ClutterStateKey *key = k->data;
          GValue initial = {0,};

          g_value_init (&initial,
                        clutter_interval_get_value_type (key->interval));

          g_object_get_property (key->object,
                                 key->property_name, &initial);
          if (clutter_alpha_get_mode (key->alpha) != key->mode)
            clutter_alpha_set_mode (key->alpha, key->mode);
          clutter_interval_set_initial_value (key->interval, &initial);
          clutter_interval_set_final_value (key->interval, &key->value);

          g_value_unset (&initial);
        }

       priv->target_state = state;
       clutter_timeline_rewind (priv->timeline);
       clutter_timeline_start (priv->timeline);
    }
  else
    {
      g_warning ("Anim state '%s' not found\n", target_state_name);
    }

  return priv->timeline;
}


/**
 * clutter_states_set:
 * @states: a #ClutterStates instance.
 * @state_name: the name of the state keys are being added for.
 * @first_object: a #GObject
 * @first_property_name: the property to specify a key for
 * @first_mode: the id of the alpha function to use
 * @...: the value first_property_name should have in state_name, followed by
 * more object,property_name,mode,value,...  or NULL to terminate the varargs.
 *
 * Adds multiple keys to a named state of a #ClutterStates instance, specifying
 * the easing mode and value a given property of an object should have at a
 * given progress of the animation. The mode specified is the mode used when
 * going to this key from the previous key of the property_name, If a given
 * object, state_name, property triple already exist the mode and value will be
 * replaced with the new values for the existing key. If a the property_name is
 * prefixed with "delayed::" two additional arguments are expect per key with a
 * value relative to the full transition time to pause before transitioning and
 * after transitioning within the total transition time.
 *
 * Since: 1.2
 */
void
clutter_states_set (ClutterStates *states,
                    const gchar   *source_state_name,
                    const gchar   *target_state_name,
                    gpointer       first_object,
                    const gchar   *first_property_name,
                    gulong         first_mode,
                    ...)
{
  GObjectClass *klass;
  gpointer      object;
  const gchar  *property_name;
  gulong        mode;
  va_list       args;

  object = first_object;

  property_name = first_property_name;
  mode = first_mode;
  va_start (args, first_mode);

  g_return_if_fail (G_IS_OBJECT (first_object));
  g_return_if_fail (first_property_name);

  while (object != NULL)
    {
      GParamSpec *pspec;
      GValue value = { 0, };
      gchar *error = NULL;
      const gchar *real_property_name = property_name;
      klass = G_OBJECT_GET_CLASS (object);

      if (g_str_has_prefix (property_name, "delayed::"))
        {
          real_property_name = strstr (property_name, "::") + 2;
        }

      pspec = g_object_class_find_property (klass, real_property_name);

      if (!pspec)
        {
          g_warning ("Cannot bind property '%s': object of type '%s' "
                     "do not have this property",
                     real_property_name, G_OBJECT_TYPE_NAME (object));
          break;
        }

      g_value_init (&value, G_PARAM_SPEC_VALUE_TYPE (pspec));
      G_VALUE_COLLECT (&value, args, 0, &error);

      if (error)
        {
          g_warning ("%s: %s", G_STRLOC, error);
          g_free (error);
          break;
        }

      if (g_str_has_prefix (property_name, "delayed::"))
        {
          gdouble pre_delay = va_arg (args, gdouble);
          gdouble post_delay = va_arg (args, gdouble);
          clutter_states_set_key (states,
                                  source_state_name,
                                  target_state_name,
                                  object,
                                  real_property_name,
                                  mode,
                                  &value,
                                  pre_delay,
                                  post_delay);
        }
      else
        {
          clutter_states_set_key (states,
                                  source_state_name,
                                  target_state_name,
                                  object,
                                  property_name,
                                  mode,
                                  &value,
                                  0.0,
                                  0.0);
        }

      object = va_arg (args, gpointer);
      if (object)
        {
          property_name = va_arg (args, gchar*);
          mode = va_arg (args, gulong);
        }
    }

  va_end (args);
}

/**
 * clutter_states_set_key:
 * @states: a #ClutterStates instance.
 * @source_state_name: the source state to specify transition for
 * @target_state_name: the name of the state to set a key.
 * @object: the #GObject to set a key for
 * @property_name: the property to set a key for
 * @mode: the id of the alpha function to use
 * @value: the value for property_name of object in state_name
 * @pre_delay: relative time of the transition to be idle in the beginning of the transition
 * @post_delay: relative time of the transition to be idle in the end of the transition
 *
 * Sets one specific end key for a state_name, object, property_name
 * combination. (XXX: should perhaps be possible to even specify a specific
 * source state in addition to the wildcard one?)
 *
 * Returns: the #ClutterStates instance, allowing chaining of multiple calls.
 * Since: 1.2
 */
ClutterStates *
clutter_states_set_key (ClutterStates *states,
                        const gchar   *source_state_name,
                        const gchar   *target_state_name,
                        GObject       *object,
                        const gchar   *property_name,
                        gulong         mode,
                        const GValue  *value,
                        gdouble        pre_delay,
                        gdouble        post_delay)
{
  ClutterStateKey *state_key;
  GList           *old_item;
  ClutterState    *source_state = NULL;
  ClutterState    *target_state;

  g_return_val_if_fail (CLUTTER_IS_STATES (states), NULL);
  g_return_val_if_fail (G_IS_OBJECT (object), NULL);
  g_return_val_if_fail (property_name, NULL);
  g_return_val_if_fail (value, NULL);

  if (target_state_name == NULL)
    target_state_name = "default";

  source_state_name = g_intern_string (source_state_name);
  target_state_name = g_intern_string (target_state_name);
  property_name = g_intern_string (property_name);

  target_state = g_hash_table_lookup (states->priv->states,
                                      target_state_name);

  if (!target_state)
    {
      target_state = state_new (states, target_state_name);
      g_hash_table_insert (states->priv->states,
                           (gpointer)target_state_name, target_state);
    }

  if (source_state_name)
    {
      source_state = g_hash_table_lookup (states->priv->states,
                                          source_state_name);
      if (!source_state)
        {
          source_state = state_new (states, source_state_name);
          g_hash_table_insert (states->priv->states,
                               (gpointer)source_state_name, source_state);
        }
    }

  state_key = clutter_state_key_new (target_state, object, property_name, mode);
  state_key->source_state = source_state;

  state_key->pre_delay = pre_delay;
  state_key->post_delay = post_delay;

  g_value_init (&state_key->value, G_VALUE_TYPE (value));
  g_value_copy (value, &state_key->value);

  if ((old_item = g_list_find_custom (target_state->keys, state_key,
                                      sort_props_func)))
    {
      ClutterStateKey *old_key = old_item->data;
      clutter_state_key_free (old_key);
      target_state->keys = g_list_remove (target_state->keys, old_key);
    }

  target_state->keys = g_list_insert_sorted (target_state->keys, state_key,
                                             sort_props_func);
  return states;
}

/**
 * clutter_states_get_states:
 * @states: a #ClutterStates instance.
 *
 * Get a list of all the state names managed by this #ClutterStates.
 *
 * Returns: a GList of const gchar * containing state names.
 * Since: 1.2
 */
GList *
clutter_states_get_states (ClutterStates *states)
{
  return g_hash_table_get_keys (states->priv->states);
}

/**
 * clutter_states_get_keys:
 * @states: a #ClutterStates instance.
 * @source_state_name: the source state name to query for, or NULL for all
 * source states.
 * @target_state_name: the target state name to query for, or NULL for all
 * target states.
 * @object: the specific object instance to list keys for, or NULL for all
 * managed objects.
 * @property_name: the property name to search for or NULL for all properties.
 *
 * Returns a list of pointers to opaque structures with accessor functions
 * that describe the keys added to an animator.
 *
 * Return value: a GList of #ClutterAnimtorKey<!-- -->s.
 * Since: 1.2
 */
GList *
clutter_states_get_keys (ClutterStates *states,
                         const gchar   *source_state_name,
                         const gchar   *target_state_name,
                         GObject       *object,
                         const gchar   *property_name)
{
  GList *s, *state_list;
  GList *targets = NULL;
  ClutterState *source_state = NULL;

  g_return_val_if_fail (CLUTTER_IS_STATES (states), NULL);

  source_state_name = g_intern_string (source_state_name);
  target_state_name = g_intern_string (target_state_name);
  property_name = g_intern_string (property_name);

  if (target_state_name != NULL)
    {
      state_list = g_list_append (NULL, (void*)target_state_name);
    }
  else
    {
      state_list = clutter_states_get_states (states);
    }

  if (source_state_name)
    source_state = g_hash_table_lookup (states->priv->states,
                                        source_state_name);

  for (s = state_list; s; s=s->next)
    {
      ClutterState *target_state;
      target_state = g_hash_table_lookup (states->priv->states, s->data);

      if (target_state)
        {
          GList *k;

          for (k = target_state->keys; k; k = k->next)
            {
              ClutterStateKey *key = k->data;
              if ((object == NULL || (object == key->object)) &&
                   (source_state_name == NULL || source_state == key->source_state) &&
                   (property_name == NULL || 
                    ((property_name == key->property_name))))
                {
                  targets = g_list_append (targets, key);
                }
            }
        }
    }
  g_list_free (state_list);
  return targets;
}

/**
 * clutter_states_remove:
 * @states: a #ClutterStates instance.
 * @source_state_name: the source state name to query for, or NULL for all
 * source states.
 * @target_state_name: the target state name to query for, or NULL for all
 * target states.
 * @object: the specific object instance to list keys for, or NULL for all
 * managed objects.
 * @property_name: the property name to search for or NULL for all properties.
 *
 * Removes all keys matching the search criteria passed in arguments.
 *
 * Since: 1.2
 */
void
clutter_states_remove (ClutterStates *states,
                       const gchar   *source_state_name,
                       const gchar   *target_state_name,
                       GObject       *object, 
                       const gchar   *property_name)
{
  GList *s, *state_list;

  ClutterState *source_state = NULL;

  g_return_if_fail (CLUTTER_IS_STATES (states));

  source_state_name = g_intern_string (source_state_name);
  target_state_name = g_intern_string (target_state_name);
  property_name = g_intern_string (property_name);

  if (source_state_name)
    source_state = g_hash_table_lookup (states->priv->states,
                                        source_state_name);

  if (target_state_name != NULL)
    {
      state_list = g_list_append (NULL, (void*)target_state_name);
    }
  else
    {
      state_list = clutter_states_get_states (states);
    }

  for (s = state_list; s; s=s->next)
    {
      ClutterState *target_state;
      target_state = g_hash_table_lookup (states->priv->states, s->data);

      if (target_state)
        {
          GList *k;

          for (k = target_state->keys; k; k = k->next)
            {
              ClutterStateKey        *key = k->data;

              if ((object == NULL || (object == key->object)) &&
                  (source_state == NULL ||
                   (source_state == key->source_state)) &&
                  (property_name == NULL ||
                   ((property_name == key->property_name))))
                {
                  key->inert = TRUE;
                  clutter_state_key_free (key);
                  k = target_state->keys = g_list_remove (target_state->keys,
                                                          key);
                }
            }
        }
    }
  g_list_free (state_list);
}

/**
 * clutter_states_get_timeline:
 * @states: a #ClutterStates
 *
 * Get the timeline driving the #ClutterStates
 *
 * Return value: the #ClutterTimeline that drives the state change animations.
 */
ClutterTimeline *
clutter_states_get_timeline (ClutterStates *states)
{
  g_return_val_if_fail (CLUTTER_IS_STATES (states), NULL);
  return states->priv->timeline;
}



static void
clutter_states_class_init (ClutterStatesClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->finalize = clutter_states_finalize;

  g_type_class_add_private (klass, sizeof (ClutterStatesPrivate));
}

static void
clutter_states_init (ClutterStates *self)
{
  ClutterStatesPrivate *priv;
  priv = self->priv = CLUTTER_STATES_GET_PRIVATE (self);
  priv->states = g_hash_table_new_full (g_direct_hash, g_direct_equal,
                                        NULL, state_free);
  self->priv->source_state_name = NULL;
  self->priv->target_state_name = NULL;
  priv->timeline = clutter_timeline_new (1000);
  priv->slave_timeline = clutter_timeline_new (10000);
  g_signal_connect (priv->timeline, "new-frame",
                    G_CALLBACK (clutter_states_new_frame), self);
  g_signal_connect (priv->timeline, "completed",
                    G_CALLBACK (clutter_states_completed), self);
}


/**
 * clutter_states_get_animator:
 * @states: a #ClutterStates instance.
 * @source_state_name: the name of a source state
 * @target_state_name: the name of a target state
 *
 * Retrieve a ClutterAnimation that is being used for transitioning between two
 * states if any.
 *
 * Returns: a #ClutterAnimator instance or NULL
 */
ClutterAnimator *
clutter_states_get_animator (ClutterStates *states,
                             const gchar   *source_state_name,
                             const gchar   *target_state_name)
{
  ClutterState         *target_state;
  ClutterStateAnimator *animators;
  gint i;

  g_return_val_if_fail (CLUTTER_IS_STATES (states), NULL);

  source_state_name = g_intern_string (source_state_name);
  target_state_name = g_intern_string (target_state_name);

  target_state = g_hash_table_lookup (states->priv->states,
                                      target_state_name);
  if (!target_state)
    return NULL;
  
  animators = (ClutterStateAnimator*)target_state->animators->data;

  for (i=0; animators[i].animator; i++)
    {
      if (animators[i].source_state_name == source_state_name)
        return animators[i].animator;
    }
  return NULL;
}

/**
 * clutter_states_set_animator:
 * @states: a #ClutterStates instance.
 * @source_state_name: the name of a source state
 * @target_state_name: the name of a target state
 * @animator: a #ClutterAnimator instance.
 *
 * Specify a ClutterAnimation to be used when transitioning between the two
 * named states, this allows specifying a transition between the states that is
 * more elaborate than the basic transitions other allowed by the simple
 * tweening of #ClutterStates. Pass NULL to unset an existing animator.
 * ClutterStates takes a reference on the passed in animator if any.
 */
void
clutter_states_set_animator (ClutterStates   *states,
                             const gchar     *source_state_name,
                             const gchar     *target_state_name,
                             ClutterAnimator *animator)
{
  ClutterState         *target_state;
  ClutterStateAnimator *animators;
  gint i;

  g_return_if_fail (CLUTTER_IS_STATES (states));

  source_state_name = g_intern_string (source_state_name);
  target_state_name = g_intern_string (target_state_name);

  target_state = g_hash_table_lookup (states->priv->states,
                                      target_state_name);
  if (!target_state)
    return;
  
  animators = (ClutterStateAnimator*)target_state->animators->data;
  for (i=0; animators[i].animator; i++)
    {
      if (animators[i].source_state_name == source_state_name)
        {
          g_object_unref (animators[i].animator);
          if (animator)
            animators[i].animator = g_object_ref (animator);
          else /* remove the matched animator if passed NULL */
            g_array_remove_index (target_state->animators, i);
          return;
        }
    }

  if (animator)
    {
      ClutterStateAnimator state_animator =
                              {source_state_name, g_object_ref (animator)};
      g_array_append_val (target_state->animators, state_animator);
    }
}


/**
 * clutter_state_key_get_pre_delay:
 * @state_key: a #ClutterStateKey
 *
 * Retrieve the pause before transitioning starts as a fraction of total
 * transition time.
 *
 * Returns: the pre delay used before starting the transition.
 * Since: 1.2
 */
gdouble
clutter_state_key_get_pre_delay (ClutterStateKey *state_key)
{
  g_return_val_if_fail (state_key, 0.0);
  return state_key->pre_delay;
}

/**
 * clutter_state_key_get_post_delay:
 * @state_key: a #ClutterStateKey
 *
 * Retrieve the duration of the pause after transitioning is complete as
 * a fraction of total transition time.
 *
 * Returns: the post delay, used after doing the transition.
 * Since: 1.2
 */
gdouble
clutter_state_key_get_post_delay (ClutterStateKey *state_key)
{
  g_return_val_if_fail (state_key, 0.0);
  return state_key->post_delay;
}

/**
 * clutter_state_key_get_mode:
 * @state_key: a #ClutterStateKey
 *
 * Retrieve the easing mode used for a clutter_state_key.
 *
 * Returns: the mode of a ClutterStateKey
 * Since: 1.2
 */
gulong
clutter_state_key_get_mode (ClutterStateKey *state_key)
{
  g_return_val_if_fail (state_key, 0);
  return state_key->mode;
}

/**
 * clutter_state_key_get_value:
 * @state_key: a #ClutterStateKey
 * @value: a #GValue initialized with the correct type for the state_key.
 *
 * Get a copy of the value for a ClutterStateKey, the passed in GValue needs
 * to be already initialized for the value type.
 *
 * Since: 1.2
 */
void
clutter_state_key_get_value (ClutterStateKey *state_key,
                             GValue          *value)
{
  g_return_if_fail (state_key);
  g_value_copy (&state_key->value, value);
}


/**
 * clutter_state_key_get_object:
 * @state_key: a #ClutterStateKey
 *
 * Get the object instance this #ClutterStateKey applies to.
 *
 * Returns: the object this state key applies to.
 * Since: 1.2
 */
GObject *
clutter_state_key_get_object (ClutterStateKey *state_key)
{
  g_return_val_if_fail (state_key, NULL);
  return state_key->object;
}

/**
 * clutter_state_key_get_property_name:
 * @state_key: a #ClutterStateKey
 *
 * Get the name of the property this #ClutterStateKey applies to.
 *
 * Returns: the name of the property the key applies to.
 * Since: 1.2
 */
const gchar *
clutter_state_key_get_property_name (ClutterStateKey *state_key)
{
  g_return_val_if_fail (state_key, NULL);
  return state_key->property_name;
}

/**
 * clutter_state_key_get_source_state_name:
 * @state_key: a #ClutterStateKey
 *
 * Get the name of the source state this #ClutterStateKey contains,
 * or NULL if this is the generic state key for the given property
 * when transitioning to the target state.
 *
 * Returns: the name of the source state for this key, or NULL if
 * the key is generic.
 * Since: 1.2
 */
const gchar *
clutter_state_key_get_source_state_name (ClutterStateKey *state_key)
{
  g_return_val_if_fail (state_key, NULL);
  if (state_key->source_state)
    {
      return state_key->source_state->name;
    }
  return NULL;
}

/**
 * clutter_state_key_get_target_state_name:
 * @state_key: a #ClutterStateKey
 *
 * Get the name of the source state this #ClutterStateKey contains,
 * or NULL if this is the generic state key for the given property
 * when transitioning to the target state.
 *
 * Returns: the name of the source state for this key, or NULL if
 * the key is generic.
 * Since: 1.2
 */
const gchar *
clutter_state_key_get_target_state_name (ClutterStateKey *state_key)
{
  g_return_val_if_fail (state_key, NULL);
  return state_key->target_state->name;
}
