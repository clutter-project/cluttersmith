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
static gfloat manipulate_x;
static gfloat manipulate_y;

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

static gpointer is_in_actor (ClutterActor *actor, gfloat *args)
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
cs_selection_pick (gfloat x, gfloat y)
{
  gfloat data[2]={x,y}; 
  return cs_selected_match (G_CALLBACK (is_in_actor), data);
}

ClutterActor *cs_pick (gfloat x, gfloat y)
{
  GList *actors = cs_container_get_children_recursive (
      CLUTTER_CONTAINER (clutter_actor_get_stage(cluttersmith->parasite_root)));
  ClutterActor *ret;
  gfloat data[2]={x,y}; 
  ret = cs_list_match (actors, G_CALLBACK (is_in_actor), data);
  g_list_free (actors);
  return ret;
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

ClutterActor *cs_children_pick (ClutterActor *actor, gfloat x, gfloat y)
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

gchar *json_serialize_subtree (ClutterActor *root);

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

static void
cs_overlay_paint (ClutterActor *stage,
                  gpointer      user_data)
{
  cs_move_snap_paint ();
  cs_selected_paint ();
  cs_animator_editor_stage_paint ();
}

void animator_editor_update_handles (void);

gboolean update_overlay_positions (gpointer data)
{

  if (cluttersmith->current_animator)
    animator_editor_update_handles ();

  if (cs_selected_count ()==0 && lasso == NULL)
    {
      clutter_actor_hide (cluttersmith->move_handle);
      clutter_actor_hide (cluttersmith->resize_handle);
      return TRUE;
    }
  clutter_actor_show (cluttersmith->move_handle);
  clutter_actor_show (cluttersmith->resize_handle);
      
  /*XXX: */ clutter_actor_hide (cluttersmith->move_handle);

  min_x = 65536;
  min_y = 65536;
  max_x = 0;
  max_y = 0;
  cs_selected_foreach (G_CALLBACK (find_extent), data);

  clutter_actor_set_position (cluttersmith->resize_handle, max_x, max_y);
  clutter_actor_set_position (cluttersmith->move_handle, (max_x+min_x)/2, (max_y+min_y)/2);

  return TRUE;
}

void init_multi_select (void);

void cs_actor_editing_init (gpointer stage)
{
  clutter_threads_add_repaint_func (update_overlay_positions, stage, NULL);
  g_signal_connect_after (stage, "paint", G_CALLBACK (cs_overlay_paint), NULL);
  init_multi_select ();
}


static gfloat manipulate_pan_start_x = 0;
static gfloat manipulate_pan_start_y = 0;

static void do_zoom (gboolean in,
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

static gboolean
manipulate_pan_capture (ClutterActor *stage,
                        ClutterEvent *event,
                        gpointer      data)
{
  switch (event->any.type)
    {
      case CLUTTER_MOTION:
        {
          gfloat ex = event->motion.x, ey = event->motion.y;
          gfloat originx, originy;

          g_object_get (cluttersmith, "origin-x", &originx,
                                      "origin-y", &originy,
                                      NULL);
          originx += manipulate_x-ex;
          originy += manipulate_y-ey;
          g_object_set (cluttersmith, "origin-x", originx,
                                      "origin-y", originy,
                                      NULL);

          manipulate_x=ex;
          manipulate_y=ey;
        }
        break;
      case CLUTTER_BUTTON_RELEASE:
        clutter_actor_queue_redraw (stage);
        g_signal_handlers_disconnect_by_func (stage, manipulate_pan_capture, data);
        if (manipulate_x == manipulate_pan_start_x &&
            manipulate_y == manipulate_pan_start_y)
          {
            do_zoom (!(event->button.modifier_state & CLUTTER_SHIFT_MASK), manipulate_x, manipulate_y);
          }
      default:
        break;
    }
  return TRUE;
}

gboolean manipulate_pan_start (ClutterEvent  *event)
{
  manipulate_x = event->button.x;
  manipulate_y = event->button.y;

  manipulate_pan_start_x = manipulate_x;
  manipulate_pan_start_y = manipulate_y;

  g_signal_connect (clutter_actor_get_stage (event->any.source), "captured-event",
                    G_CALLBACK (manipulate_pan_capture), NULL);
  return TRUE;
}




gint cs_last_x = 0;
gint cs_last_y = 0;

static ClutterActor *edited_actor = NULL;

static gboolean
manipulate_capture (ClutterActor *actor,
                    ClutterEvent *event,
                    gpointer      data /* unused */)
{

  /* pass events through to text being edited */
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

    if (event->any.type == CLUTTER_KEY_PRESS)
      {
          if(manipulator_key_pressed_global (actor, clutter_event_get_state(event), event->key.keyval))
            return TRUE;
      }

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
          do_zoom (event->scroll.direction == CLUTTER_SCROLL_UP,
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
                           cs_selected_remove (hit);
                         }
                     }
                   if (event->button.modifier_state & CLUTTER_MOD1_MASK)
                     {
                       if (cs_selected_has_actor (hit))
                         {
                           ClutterActor *next = cs_siblings_pick_next (hit, x, y);
                           cs_selected_remove (hit);
                           cs_selected_add (next);
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

static gboolean playback_context (ClutterActor *actor,
                                  ClutterEvent *event)
{
  if (!(cluttersmith->ui_mode & CS_UI_MODE_EDIT) &&
      clutter_event_get_button (event)==3)
    {
      playback_menu (event->button.x, event->button.y);
      return TRUE;
    }
  return FALSE;
}

void cs_manipulate_init (ClutterActor *actor)
{
  g_signal_connect_after (clutter_actor_get_stage (actor), "captured-event",
                          G_CALLBACK (manipulate_capture), NULL);
  g_signal_connect (clutter_actor_get_stage (actor), "button-press-event",
                    G_CALLBACK (playback_context), NULL);
  cs_edit_text_init ();
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
