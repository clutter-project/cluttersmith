#include <clutter/clutter.h>
#include <mx/mx.h>
#include <stdlib.h>
#include <string.h>
#include "cluttersmith.h"

/*
 * Shared state used by all the separate states the event handling can
 * the substates are exclusive (can be extended to have a couple of stages
 * that allow falling through to each other, but a fixed event pipeline
 * is an simpler initial base).
 */
gint cs_last_x = 0;
gint cs_last_y = 0;
static ClutterActor *edited_actor = NULL;

gboolean manipulate_pan_start (ClutterEvent  *event);
ClutterActor        *cs_siblings_pick_next (ClutterActor *sibling,
                                            gfloat        x,
                                            gfloat        y);
static ClutterActor * cs_selection_pick    (gfloat        x,
                                            gfloat        y);
ClutterActor        *cs_children_pick      (ClutterActor *actor,
                                            gfloat        x,
                                            gfloat        y);

static void each_add_to_list (ClutterActor *actor,
                               gpointer      string)
{
  g_string_append_printf (string, "$(\"%s\"),", cs_get_id (actor));
}

gboolean
cs_stage_capture (ClutterActor *actor,
                  ClutterEvent *event,
                  gpointer      data /* unused */)
{
  /* pass events through to actor being edited, this
   * is perhaps not what is desired in the general
   * case, and the text editing should be reimplemented
   * to use an additional proxy. */
  if (edited_actor)
    { 
      if (event->any.type == CLUTTER_KEY_PRESS &&
          event->key.keyval == CLUTTER_Escape)
        {
          cs_edit_actor_end ();
          return TRUE;
        }

      /* the passthrough of events, to perhaps
       * another var than edited_actor, needs
       * to be wider to support other than
       * text editing.
       */
      if (event->any.source == edited_actor)
        {
          return FALSE;
        }
      else
        {
          /* break out when presses occur outside the edited actor,
           * this currently means that proper mouse movement and
           * selection is not possible for MxLabels and Entries
           * when edited
           */
          switch (event->any.type)
            {
              case CLUTTER_BUTTON_PRESS:
                cs_edit_actor_end ();
              default:
                return TRUE;
            }
        }
    }

   if (event->any.type == CLUTTER_KEY_PRESS
       && manipulator_key_pressed_global (actor,
                                           clutter_event_get_state(event),
                                           event->key.keyval))
     return TRUE;

   if (event->any.type == CLUTTER_KEY_PRESS &&
       !cs_actor_has_ancestor (event->any.source, cluttersmith->parasite_root)) /* If the source is in the parasite ui,
                                                                 pass in on as normal*/
     {
        if ((cluttersmith->ui_mode & CS_UI_MODE_EDIT) &&
            manipulator_key_pressed (actor, clutter_event_get_state(event), event->key.keyval))
          return TRUE;
     }

  if (!(cluttersmith->ui_mode & CS_UI_MODE_EDIT))
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
  
  if ((clutter_get_motion_events_enabled()==FALSE) ||
      cs_actor_has_ancestor (event->any.source, cluttersmith->parasite_root))
    {
      if (event->any.type == CLUTTER_MOTION)
        {
          cs_last_x = event->motion.x;
          cs_last_y = event->motion.y;
        }
      return FALSE;
    }
  switch (event->any.type)
    {
      case CLUTTER_SCROLL:
          cs_zoom (event->scroll.direction == CLUTTER_SCROLL_UP,
                   event->scroll.x, event->scroll.y);
        return TRUE;
        break;
      case CLUTTER_MOTION:
        {
          cs_last_x = event->motion.x;
          cs_last_y = event->motion.y;
        }
        break;
      case CLUTTER_BUTTON_PRESS:
        {
          ClutterActor *hit;
          gfloat x = event->button.x;
          gfloat y = event->button.y;

          cs_last_x = x;
          cs_last_y = y;

          if (clutter_event_get_button (event) == 2)
            {
              manipulate_pan_start (event);
              return TRUE;
            }

          hit = cs_selection_pick (x, y);

          if (hit) /* pressed part of initial selection */
            {
              if (clutter_event_get_button (event) == 3)
                {
                  if (cs_selected_count ()>1)
                    {
                      selection_menu (x,y);
                      return FALSE;
                    }
                  else
                    {
                      object_menu (cs_selected_get_any (), x,y);
                      return FALSE;
                    }
                }


              if (clutter_event_get_click_count (event)>1)
                {
                  const gchar *name = clutter_actor_get_name (hit);
                  if (name && g_str_has_prefix (name, "link="))
                    {
                      cluttersmith_load_scene (name+5);
                      cs_selected_clear ();
                      return TRUE;
                    }

                  cs_edit_actor_start (hit);
                  return TRUE;
                }
              else
                {
                   if (event->button.modifier_state & CLUTTER_CONTROL_MASK)
                     {
                       if (cs_selected_has_actor (hit))
                         {
                           SELECT_ACTION_PRE();
                           cs_selected_remove (hit);
                           SELECT_ACTION_POST("select-remove");
                         }
                     }
                   if (event->button.modifier_state & CLUTTER_MOD1_MASK)
                     {
                       if (cs_selected_has_actor (hit))
                         {
                           ClutterActor *next = cs_siblings_pick_next (hit, x, y);
                           SELECT_ACTION_PRE();
                           cs_selected_remove (hit);
                           cs_selected_add (next);
                           SELECT_ACTION_POST ("select foo");
                         }
                     }
                   cs_move_start (cluttersmith->parasite_root, event);
                }
            }
          else 
            { 
              gboolean stage_child = FALSE;
              if (clutter_event_get_button (event) == 3)
                {
                  root_menu (x,y);
                  return FALSE;
                }

              {
#ifdef COMPILEMODULE
           hit = clutter_stage_get_actor_at_pos (CLUTTER_STAGE (clutter_actor_get_stage (actor)),
                                                 CLUTTER_PICK_ALL,
                                                 x, y);
#else
              hit = cs_children_pick (cs_get_current_container (), x, y);

              if (!hit)
                {
                  hit = cs_children_pick (cluttersmith->fake_stage, x, y);
                  if (hit == cs_get_current_container ())
                    hit = NULL;
                  stage_child = TRUE;
                }
#endif
              }

              if (hit)
                {
                  SELECT_ACTION_PRE();
                  if (!((clutter_event_get_state (event) & CLUTTER_CONTROL_MASK) ||
                        (clutter_event_get_state (event) & CLUTTER_SHIFT_MASK)))
                    {
                      const gchar *name = clutter_actor_get_name (hit);
                      if (name &&  /* */
                         (g_str_equal (name, "cluttersmith-is-interactive"))) /* needs to happen earlier as
                                                                               * recent working dirs is in a container
                                                                               */
                        {
                          return FALSE;
                        }
                      cs_selected_clear ();
                      if (stage_child)
                        {
                          cs_set_current_container (cluttersmith->fake_stage);
                        }
                    }
                  else
                    {
                      if (stage_child) /* don't allow multiple select of stage children when in group */
                        {
                          return TRUE;
                        }
                    }

                  if (event->button.modifier_state & CLUTTER_CONTROL_MASK)
                    {
                      if (cs_selected_has_actor (hit))
                        {
                          cs_selected_remove (hit);
                        }
                      else
                        {
                          cs_selected_add (hit);
                        }
                    }
                  else
                    {
                      cs_selected_add (hit);
                    }
                  cs_move_start (cluttersmith->parasite_root, event);
                  SELECT_ACTION_POST("select");
                }
              else
                {
                  cs_selected_lasso_start (cluttersmith->parasite_root, event);
                }
            }
        }
        break;
      case CLUTTER_BUTTON_RELEASE:
          cs_last_x = event->button.x;
          cs_last_y = event->button.y;
        return TRUE;
      default:
        break;
    }
  return TRUE;
}




static gboolean is_point_in_actor (ClutterActor *actor, gfloat x, gfloat y)
{
  ClutterVertex verts[4];

  clutter_actor_get_abs_allocation_vertices (actor,
                                             verts);
  /* XXX: use cairo to check with the outline of the verts? */
  if (x>verts[2].x && x<verts[1].x &&
      y>verts[1].y && y<verts[2].y)
    {
      return TRUE;
    }
  return FALSE;
}

static gpointer
is_in_actor (ClutterActor *actor,
             gfloat       *args)
{
  gfloat x=args[0]; /* convert pointed to argument list into variables */
  gfloat y=args[1];

  if (actor == clutter_actor_get_stage (actor))
    return NULL;

  if (is_point_in_actor (actor, x, y))
    {
      return actor;
    }
  return NULL;
}

static ClutterActor *
cs_selection_pick (gfloat x,
                   gfloat y)
{
  gfloat data[2]={x,y}; 
  return cs_selected_match (G_CALLBACK (is_in_actor), data);
}

ClutterActor *cs_siblings_pick (ClutterActor *actor, gfloat x, gfloat y)
{
  GList *siblings = cs_actor_get_siblings (actor);
  ClutterActor *ret;
  gfloat data[2]={x,y}; 
  ret = cs_list_match (siblings, G_CALLBACK (is_in_actor), data);
  g_list_free (siblings);
  return ret;
}

ClutterActor *cs_children_pick (ClutterActor *actor,
                                gfloat        x,
                                gfloat        y)
{
  GList *children = clutter_container_get_children (CLUTTER_CONTAINER (actor));
  ClutterActor *ret;
  gfloat data[2]={x,y}; 
  ret = cs_list_match (children, G_CALLBACK (is_in_actor), data);
  g_list_free (children);
  return ret;
}


typedef struct SiblingPickNextData {
  gfloat x;
  gfloat y;
  ClutterActor *sibling;
  gboolean found;
  ClutterActor *first;
} SiblingPickNextData;


static gpointer is_in_actor_sibling_next (ClutterActor *actor, SiblingPickNextData *data)
{
  if (actor == clutter_actor_get_stage (actor))
    return NULL;

  if (is_point_in_actor (actor, data->x, data->y))
    {
      if (!data->first)
        data->first = actor;
      if (data->found)
        return actor;
      if (data->sibling == actor)
        data->found = TRUE;
    }
  return NULL;
}

ClutterActor *cs_siblings_pick_next (ClutterActor *sibling, gfloat x, gfloat y)
{
  GList *actors = clutter_container_get_children (CLUTTER_CONTAINER (clutter_actor_get_parent (sibling)));
  ClutterActor *ret;
  SiblingPickNextData data = {x, y, sibling, FALSE, NULL};
  ret = cs_list_match (actors, G_CALLBACK (is_in_actor_sibling_next), &data);
  if (!ret)
    ret = data.first;
  g_list_free (actors);
  return ret;
}


/* Used for resize and similar handles on the active
 * actor
 */
gboolean cs_canvas_handler_enter (ClutterActor *actor)
{
  clutter_actor_set_opacity (actor, 255);
  return TRUE;
}

/* Used for resize and similar handles on the active
 * actor
 */
gboolean cs_canvas_handler_leave (ClutterActor *actor)
{
  clutter_actor_set_opacity (actor, 100);
  return TRUE;
}

void cs_set_current_container (ClutterActor *actor)
{
  if (actor && CLUTTER_IS_CONTAINER (actor))
    cluttersmith->current_container = actor;
}

/**
 * cs_get_current_container:
 * Return value: (transfer none): the current "working dir".
 */
ClutterActor *cs_get_current_container (void)
{
  if (!cluttersmith->current_container)
    return cluttersmith->fake_stage;
  return cluttersmith->current_container;
}




#if 0

/* independently for the axes for axis aligned boxes */

static gboolean
intersects (gint min, gint max, gint minb, gint maxb)
{
  if (minb <= max && minb >= min)
    return TRUE;
  if (min <= maxb && min >= minb)
    return TRUE;
  return FALSE;
}
#endif

static GHashTable *actor_editors_start = NULL;
static GHashTable *actor_editors_end = NULL;

void cs_actor_editors_add (GType type,
                           void (*editing_start) (ClutterActor *edited_actor),
                           void (*editing_end) (ClutterActor *edited_actor))
{
  if (!actor_editors_start)
    {
      actor_editors_start = g_hash_table_new_full (g_direct_hash, g_direct_equal, NULL, NULL);
      actor_editors_end = g_hash_table_new_full (g_direct_hash, g_direct_equal, NULL, NULL);
    }
  g_hash_table_insert (actor_editors_start, GUINT_TO_POINTER (type), editing_start);
  g_hash_table_insert (actor_editors_end, GUINT_TO_POINTER (type), editing_end);
}

static void (*editing_ended) (ClutterActor *edited_actor) = NULL;

gboolean cs_edit_actor_start (ClutterActor *actor)
{
  const gchar *name;
  if (!actor)
    return FALSE;

  if ((editing_ended = g_hash_table_lookup (actor_editors_end, GUINT_TO_POINTER (G_OBJECT_TYPE (actor)))))
    {
      void (*editing_start) (ClutterActor *edited_actor) = NULL;
      editing_start = g_hash_table_lookup (actor_editors_start, GUINT_TO_POINTER (G_OBJECT_TYPE (actor)));
      edited_actor = actor;
      editing_start (actor);
      return TRUE;
    }

  name = clutter_actor_get_name (actor);
  if (name && g_str_has_prefix (name, "link="))
    {
      cluttersmith_load_scene (name+5);
      cs_selected_clear ();
      return TRUE;
    }
  if (CLUTTER_IS_CONTAINER (actor))
     {
       cs_selected_clear ();
       cs_set_current_container (actor);
       return TRUE;
     }
  g_print ("unhandled enter edit for %s\n", G_OBJECT_TYPE_NAME (actor));

  return FALSE;
}


gboolean cs_edit_actor_end (void)
{
  if (!edited_actor)
    return FALSE;

  if (editing_ended)
    {
      editing_ended (edited_actor);
      editing_ended = NULL;
      edited_actor = NULL;
    }
  return FALSE;
}


void cs_zoom (gboolean in,
              gfloat   x,
              gfloat   y)
{
  gfloat zoom;
  gfloat origin_x;
  gfloat origin_y;
  gfloat target_x;
  gfloat target_y;

  gfloat offset_x;
  gfloat offset_y;

  clutter_actor_get_transformed_position (cs_find_by_id_int (clutter_actor_get_stage (cluttersmith->parasite_root), "fake-stage-rect"),
                                          &offset_x, &offset_y);

  clutter_actor_transform_stage_point (cluttersmith->fake_stage,
                                       x, y, &target_x, &target_y);

  g_object_get (cluttersmith,
                "zoom", &zoom,
                NULL);

  if (in)
    {
      zoom *= 1.412135;
    }
  else
    {
      zoom /= 1.412135;
    }

  origin_x = target_x * (zoom/100) - x + offset_x;
  origin_y = target_y * (zoom/100) - y + offset_y;
  
  g_object_set (cluttersmith,
                "zoom", zoom,
                "origin-x", origin_x,
                "origin-y", origin_y,
                NULL);
}
