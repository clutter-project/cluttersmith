#include <math.h>
#include <stdlib.h>
#include "cluttersmith.h"

/* snap positions, in relation to actor */
static gint ver_pos = 0; /* 1 = start 2 = mid 3 = end */
static gint hor_pos = 0; /* 1 = start 2 = mid 3 = end */
/*
 * Shared state used by all the separate states the event handling can
 * the substates are exclusive (can be extended to have a couple of stages
 * that allow falling through to each other, but a fixed event pipeline
 * is an simpler initial base).
 */
static gfloat manipulate_x;
static gfloat manipulate_y;
static gfloat start_x;
static gfloat start_y;

static GString *undo = NULL;
static GString *redo = NULL;

/* snap the provided position of an object according
 * to its siblings.
 */
static void snap_position (ClutterActor *actor,
                           gfloat        in_x,
                           gfloat        in_y,
                           gfloat       *out_x,
                           gfloat       *out_y)
{
  *out_x = in_x;
  *out_y = in_y;

  ClutterActor *parent;


  parent = clutter_actor_get_parent (actor);

  if (CLUTTER_IS_CONTAINER (parent))
    {
      gfloat in_width = clutter_actor_get_width (actor);
      gfloat in_height = clutter_actor_get_height (actor);
      gfloat in_mid_x = in_x + in_width/2;
      gfloat in_mid_y = in_y + in_height/2;
      gfloat in_end_x = in_x + in_width;
      gfloat in_end_y = in_y + in_height;


      gfloat best_x = 0;
      gfloat best_x_diff = 4096;
      gfloat best_y = 0;
      gfloat best_y_diff = 4096;
      GList *children, *c;
      children = clutter_container_get_children (CLUTTER_CONTAINER (parent));

      hor_pos = 0;
      ver_pos = 0;

      /* We only search our siblings for snapping...
       * perhaps we should search more.
       */

      for (c=children; c; c = c->next)
        {
          gfloat this_x = clutter_actor_get_x (c->data);
          gfloat this_width = clutter_actor_get_width (c->data);
          gfloat this_height = clutter_actor_get_height (c->data);
          gfloat this_end_x = this_x + this_width;
          gfloat this_mid_x = this_x + this_width/2;
          gfloat this_y = clutter_actor_get_y (c->data);
          gfloat this_end_y = this_y + this_height;
          gfloat this_mid_y = this_y + this_height/2;

          /* skip self */
          if (c->data == actor)
            continue;

/* starts aligned
                    this_x     this_mid_x     this_end_x
                    in_x    in_mid_x    in_end_x
*/
          if (abs (this_x - in_x) < best_x_diff)
            {
              best_x_diff = abs (this_x - in_x);
              best_x = this_x;
              hor_pos=1;
            }
          if (abs (this_y - in_y) < best_y_diff)
            {
              best_y_diff = abs (this_y - in_y);
              best_y = this_y;
              ver_pos=1;
            }
/*
 end aligned with start
                    this_x     this_mid_x     this_end_x
in_x    in_mid_x    in_end_x
*/
          if (abs (this_x - in_end_x) < best_x_diff)
            {
              best_x_diff = abs (this_x - in_end_x);
              best_x = this_x - in_width;
              hor_pos=3;
            }
          if (abs (this_y - in_end_y) < best_y_diff)
            {
              best_y_diff = abs (this_y - in_end_y);
              best_y = this_y - in_height;
              ver_pos=3;
            }
/*
 ends aligned
                    this_x     this_mid_x     this_end_x
                          in_x    in_mid_x    in_end_x
*/
          if (abs (this_end_x - in_end_x) < best_x_diff)
            {
              best_x_diff = abs (this_end_x - in_end_x);
              best_x = this_end_x - in_width;
              hor_pos=3;
            }
          if (abs (this_end_y - in_end_y) < best_y_diff)
            {
              best_y_diff = abs (this_end_y - in_end_y);
              best_y = this_end_y - in_height;
              ver_pos=3;
            }
/*
 end aligned with start
                    this_x     this_mid_x     this_end_x
                                              in_x    in_mid_x    in_end_x
                    x=this_end
*/
          if (abs (this_end_x - in_x) < best_x_diff)
            {
              best_x_diff = abs (this_end_x - in_x);
              best_x = this_end_x;
              hor_pos=1;
            }
          if (abs (this_end_y - in_y) < best_y_diff)
            {
              best_y_diff = abs (this_end_y - in_y);
              best_y = this_end_y;
              ver_pos=1;
            }
/*
 middles aligned
                    this_x     this_mid_x     this_end_x
                       in_x    in_mid_x    in_end_x
                    x=this_mid_x-in_width/2
*/
          if (abs (this_mid_x - in_mid_x) < best_x_diff)
            {
              best_x_diff = abs (this_mid_x - in_mid_x);
              best_x = this_mid_x - in_width/2;
              hor_pos=2;
            }
          if (abs (this_mid_y - in_mid_y) < best_y_diff)
            {
              best_y_diff = abs (this_mid_y - in_mid_y);
              best_y = this_mid_y - in_height/2;
              ver_pos=2;
            }
        }

        {
          if (best_x_diff < SNAP_THRESHOLD)
            {
              *out_x = best_x;
            }
          else
            {
              hor_pos = 0;
            }
          if (best_y_diff < SNAP_THRESHOLD)
            {
              *out_y = best_y;
            }
          else
            {
              ver_pos = 0;
            }
        }
    }
}


static void each_move (ClutterActor *actor,
                       gpointer      data)
{
  gfloat *delta = data;
  gfloat x, y;
  clutter_actor_get_position (actor, &x, &y);
  x-= delta[0];
  y-= delta[1];
  clutter_actor_set_position (actor, x, y);
}


static void
selection_to_position_commands (GString *string)
{
  GList *s, *selected;

  selected = cs_selected_get_list ();
  for (s = selected; s; s = s->next)
    {
      ClutterActor *actor = s->data;
      gfloat x, y;
      clutter_actor_get_position (actor, &x, &y);
      g_string_append_printf (string, "$('%s').x = %f; $('%s').y = %f;\n",
                              cs_get_id (actor), x,
                              cs_get_id (actor), y);
    }
  g_list_free (selected);
}

static gboolean
manipulate_move_capture (ClutterActor *stage,
                         ClutterEvent *event,
                         gpointer      data)
{
  switch (event->any.type)
    {
      case CLUTTER_MOTION:
        {
          gfloat delta[2];
          delta[0]=manipulate_x-event->motion.x;
          delta[1]=manipulate_y-event->motion.y;
          {
            if (cs_selected_count ()==1)
              {
                /* we only snap when there is only one selected item */

                GList *selected = cs_selected_get_list ();
                ClutterActor *actor = selected->data;
                gfloat x, y;
                g_assert (actor);
                clutter_actor_get_position (actor, &x, &y);
                x-= delta[0];
                y-= delta[1];
                snap_position (actor, x, y, &x, &y);
                clutter_actor_set_position (actor, x, y);
                cs_prop_tweaked (G_OBJECT (actor), "x");
                cs_prop_tweaked (G_OBJECT (actor), "y");
                g_list_free (selected);
              }
            else
              {
                cs_selected_foreach (G_CALLBACK (each_move), &delta[0]);
              }
          }

          manipulate_x=event->motion.x;
          manipulate_y=event->motion.y;

        }
        break;
      case CLUTTER_BUTTON_RELEASE:
        g_signal_handlers_disconnect_by_func (stage, manipulate_move_capture, data);
        hor_pos = 0;
        ver_pos = 0;

        selection_to_position_commands (redo);
        if (start_x != manipulate_x
            || start_y != manipulate_y)
          cs_history_add ("move actors", redo->str, undo->str);
        g_string_free (undo, TRUE);
        g_string_free (redo, TRUE);
        undo = redo = NULL;
           
        cs_dirtied ();

        clutter_actor_queue_redraw (stage);
      default:
        break;
    }
  return TRUE;
}

gboolean cs_move_start (ClutterActor  *actor,
                        ClutterEvent  *event)
{
  start_x = manipulate_x = event->button.x;
  start_y = manipulate_y = event->button.y;

  undo = g_string_new ("");
  redo = g_string_new ("");

  selection_to_position_commands (undo);

  g_signal_connect (clutter_actor_get_stage (actor), "captured-event",
                    G_CALLBACK (manipulate_move_capture), actor);
  clutter_actor_queue_redraw (actor);
  return TRUE;
}


void cs_move_snap_paint (void)
{
  ClutterVertex verts[4];

    if(0){
      ClutterActor *parent = cs_get_current_container ();
      if (parent)
        {
          cogl_set_source_color4ub (255, 0, 255, 128);
          cs_draw_actor_outline (parent, NULL);
        }
    }

  if (cs_selected_count ()==0 && lasso == NULL)
    return;

  if (cs_selected_count ()==1)
    {
      ClutterActor *actor;
      {
        GList *l = cs_selected_get_list ();
        actor = l->data;
        g_list_free (l);
      }

      clutter_actor_get_abs_allocation_vertices (actor,
                                                 verts);
  
      {
        /* potentially draw lines auto snapping has matched */
        cogl_set_source_color4ub (128, 128, 255, 255);
        switch (hor_pos)
          {
            case 1:
             {
                gfloat coords[]={ verts[2].x, -2000,
                                  verts[2].x, 2000 };
                cogl_path_polyline (coords, 2);
                cogl_path_stroke ();
             }
             break;
            case 2:
             {
                gfloat coords[]={ (verts[2].x+verts[1].x)/2, -2000,
                                  (verts[2].x+verts[1].x)/2, 2000 };
                cogl_path_polyline (coords, 2);
                cogl_path_stroke ();
             }
             break;
            case 3:
             {
                gfloat coords[]={ verts[1].x, -2000,
                                  verts[1].x, 2000 };
                cogl_path_polyline (coords, 2);
                cogl_path_stroke ();
             }
          }
        switch (ver_pos)
          {
            case 1:
             {
                gfloat coords[]={ -2000, verts[1].y,
                                   2000, verts[1].y};
                cogl_path_polyline (coords, 2);
                cogl_path_stroke ();
             }
             break;
            case 2:
             {
                gfloat coords[]={ -2000, (verts[2].y+verts[1].y)/2,
                                   2000, (verts[2].y+verts[1].y)/2};
                cogl_path_polyline (coords, 2);
                cogl_path_stroke ();
             }
             break;
            case 3:
             {
                gfloat coords[]={ -2000, verts[2].y,
                                   2000, verts[2].y};
                cogl_path_polyline (coords, 2);
                cogl_path_stroke ();
             }
          }
      }
    }

  cs_selected_paint ();
  cs_animator_editor_stage_paint ();

}
