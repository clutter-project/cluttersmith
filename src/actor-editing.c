#include <clutter/clutter.h>
#include <mx/mx.h>
#include <stdlib.h>
#include <string.h>
#include "cluttersmith.h"
#include "util.h"

static guint stage_capture_handler = 0;

ClutterActor      *lasso;
static gint        lx, ly;
static GHashTable *selection = NULL; /* what would be added/removed by
                                        current lasso*/



void cs_selected_init (void);
static void init_multi_select (void)
{
  selection = g_hash_table_new_full (g_direct_hash, g_direct_equal, NULL, NULL);
  cs_selected_init ();
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
      clutter_actor_get_stage(cluttersmith->parasite_root));
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


/* snap positions, in relation to actor */
static gint ver_pos = 0; /* 1 = start 2 = mid 3 = end */
static gint hor_pos = 0; /* 1 = start 2 = mid 3 = end */

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


static void draw_actor_outline (ClutterActor *actor,
                                gpointer      data)
{
  ClutterVertex verts[4];
  clutter_actor_get_abs_allocation_vertices (actor,
                                             verts);

  {
    {
      gfloat coords[]={ verts[0].x, verts[0].y, 
         verts[1].x, verts[1].y, 
         verts[3].x, verts[3].y, 
         verts[2].x, verts[2].y, 
         verts[0].x, verts[0].y };

      cogl_path_polyline (coords, 5);
      cogl_path_stroke ();
    }
  }
}

static void
cs_overlay_paint (ClutterActor *stage,
                  gpointer      user_data)
{
  ClutterVertex verts[4];

    {
      ClutterActor *parent = cs_get_current_container ();
      if (parent)
        {
          cogl_set_source_color4ub (255, 0, 255, 128);
          draw_actor_outline (parent, NULL);
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
  

      cogl_set_source_color4ub (0, 25, 0, 50);


      {
#if 0
        CoglTextureVertex tverts[] = 
           {
             {verts[0].x, verts[0].y, },
             {verts[2].x, verts[2].y, },
             {verts[3].x, verts[3].y, },
             {verts[1].x, verts[1].y, },
           };
        /* fill the item, if it is the only item in the selection */
        cogl_polygon (tverts, 4, FALSE);
#endif

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

  {
  /* draw outlines for actors */
    GHashTableIter      iter;
    gpointer            key, value;

    {
        {
          if (cluttersmith->fake_stage)
            {
              cogl_set_source_color4ub (0, 255, 0, 255);
              draw_actor_outline (cluttersmith->fake_stage, NULL);
            }
        }
    }


    cogl_set_source_color4ub (255, 0, 0, 128);
    cs_selected_foreach (G_CALLBACK (draw_actor_outline), NULL);

    g_hash_table_iter_init (&iter, selection);
    while (g_hash_table_iter_next (&iter, &key, &value))
      {
        clutter_actor_get_abs_allocation_vertices (key,
                                                   verts);

        cogl_set_source_color4ub (0, 0, 25, 50);

        {
          {
            gfloat coords[]={ verts[0].x, verts[0].y, 
               verts[1].x, verts[1].y, 
               verts[3].x, verts[3].y, 
               verts[2].x, verts[2].y, 
               verts[0].x, verts[0].y };

            cogl_path_polyline (coords, 5);
            cogl_set_source_color4ub (0, 0, 255, 128);
            cogl_path_stroke ();
          }
        }
      }
   }

  /* Draw path of currently animated actor */
  if (cluttersmith->current_animator)
    {
      ClutterActor *actor = cs_selected_get_any ();
      if (actor)
        {
          gfloat progress;
          GValue xv = {0, };
          GValue yv = {0, };
          GValue value = {0, };

          g_value_init (&xv, G_TYPE_FLOAT);
          g_value_init (&yv, G_TYPE_FLOAT);
          g_value_init (&value, G_TYPE_FLOAT);

          for (progress = 0.0; progress < 1.0; progress += 0.01)
            {
              ClutterVertex vertex = {0, };
              gfloat x, y;
              clutter_animator_compute_value (cluttersmith->current_animator,
                                           G_OBJECT (actor), "x", progress, &xv);
              clutter_animator_compute_value (cluttersmith->current_animator,
                                           G_OBJECT (actor), "y", progress, &yv);
              x = g_value_get_float (&xv);
              y = g_value_get_float (&yv);
              vertex.x = x;
              vertex.y = y;

              clutter_actor_apply_transform_to_point (clutter_actor_get_parent (actor),
                                                      &vertex, &vertex);

              cogl_path_line_to (vertex.x, vertex.y);
            }
          cogl_path_stroke ();

          {
            GList *k, *keys;

            keys = clutter_animator_get_keys (cluttersmith->current_animator,
                                              G_OBJECT (actor), "x", -1.0);
            for (k = keys; k; k = k->next)
              {
                gdouble progress = clutter_animator_key_get_progress (k->data);
                ClutterVertex vertex = {0, };
                gfloat x, y;
                g_print ("[%f]\n", progress);
                clutter_animator_compute_value (cluttersmith->current_animator,
                                             G_OBJECT (actor), "x", progress, &xv);
                clutter_animator_compute_value (cluttersmith->current_animator,
                                             G_OBJECT (actor), "y", progress, &yv);
                x = g_value_get_float (&xv);
                y = g_value_get_float (&yv);
                vertex.x = x;
                vertex.y = y;

                clutter_actor_apply_transform_to_point (clutter_actor_get_parent (actor),
                                                        &vertex, &vertex);

                cogl_path_new ();
                cogl_path_ellipse (vertex.x, vertex.y, 5, 5);
                cogl_path_fill ();
              }
            g_list_free (keys);
          }
          g_value_unset (&xv);
          g_value_unset (&yv);
        }
    }
}

gboolean update_overlay_positions (gpointer data)
{
  if (cs_selected_count ()==0 && lasso == NULL)
    {
      clutter_actor_hide (cluttersmith->active_panel);
      clutter_actor_hide (cluttersmith->move_handle);
      clutter_actor_hide (cluttersmith->resize_handle);
      return TRUE;
    }
  clutter_actor_show (cluttersmith->active_panel);
  clutter_actor_show (cluttersmith->move_handle);
  clutter_actor_show (cluttersmith->resize_handle);
      
  /*XXX: */ clutter_actor_hide (cluttersmith->active_panel);
  /*XXX: */ clutter_actor_hide (cluttersmith->move_handle);

  min_x = 65536;
  min_y = 65536;
  max_x = 0;
  max_y = 0;
  {
    cs_selected_foreach (G_CALLBACK (find_extent), data);
  }

   {
     clutter_actor_set_position (cluttersmith->resize_handle, max_x, max_y);
     clutter_actor_set_position (cluttersmith->move_handle, (max_x+min_x)/2, (max_y+min_y)/2);
     clutter_actor_set_position (cluttersmith->active_panel, min_x, max_y);
   }

 return TRUE;
}

void cs_actor_editing_init (gpointer stage)
{
  clutter_threads_add_repaint_func (update_overlay_positions, stage, NULL);
  g_signal_connect_after (stage, "paint", G_CALLBACK (cs_overlay_paint), NULL);
  g_signal_connect_after (stage, "paint", G_CALLBACK (cs_overlay_paint), NULL);
  init_multi_select ();
}

/*
 * Shared state used by all the separate states the event handling can
 * the substates are exclusive (can be extended to have a couple of stages
 * that allow falling through to each other, but a fixed event pipeline
 * is an simpler initial base).
 */
static guint  manipulate_capture_handler = 0;
static gfloat manipulate_x;
static gfloat manipulate_y;

#define SNAP_THRESHOLD  2

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

/* snap size, as affected by resizing lower right corner,
 * will need extension if other corners are to be supported,
 * it seems possible to do all needed alignments through
 * simple workarounds when only snapping for lower right).
 */
static void snap_size (ClutterActor *actor,
                       gfloat        in_width,
                       gfloat        in_height,
                       gfloat       *out_width,
                       gfloat       *out_height)
{
  *out_width = in_width;
  *out_height = in_height;

  ClutterActor *parent;

  parent = clutter_actor_get_parent (actor);

  if (CLUTTER_IS_CONTAINER (parent))
    {
      gfloat in_x = clutter_actor_get_x (actor);
      gfloat in_y = clutter_actor_get_y (actor);
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
          gfloat this_y = clutter_actor_get_y (c->data);
          gfloat this_end_y = this_y + this_height;

          /* skip self */
          if (c->data == actor)
            continue;

/*
 end aligned with start
                    this_x     this_mid_x     this_end_x
in_x    in_mid_x    in_end_x
*/
          if (abs (this_x - in_end_x) < best_x_diff)
            {
              best_x_diff = abs (this_x - in_end_x);
              best_x = this_x;
              hor_pos=3;
            }
          if (abs (this_y - in_end_y) < best_y_diff)
            {
              best_y_diff = abs (this_y - in_end_y);
              best_y = this_y;
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
              best_x = this_end_x;
              hor_pos=3;
            }
          if (abs (this_end_y - in_end_y) < best_y_diff)
            {
              best_y_diff = abs (this_end_y - in_end_y);
              best_y = this_end_y;
              ver_pos=3;
            }
        }

        {
          if (best_x_diff < SNAP_THRESHOLD)
            {
              *out_width = best_x-in_x;
            }
          else
            {
              hor_pos = 0;
            }
          if (best_y_diff < SNAP_THRESHOLD)
            {
              *out_height = best_y-in_y;
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
          cs_dirtied ();



          manipulate_x=event->motion.x;
          manipulate_y=event->motion.y;

        }
        break;
      case CLUTTER_BUTTON_RELEASE:
        g_signal_handler_disconnect (stage,
                                     manipulate_capture_handler);
        manipulate_capture_handler = 0;
        hor_pos = 0;
        ver_pos = 0;

        clutter_actor_queue_redraw (stage);
      default:
        break;
    }
  return TRUE;
}

gboolean manipulate_move_start (ClutterActor  *actor,
                                ClutterEvent  *event)
{
  manipulate_x = event->button.x;
  manipulate_y = event->button.y;

  manipulate_capture_handler = 
     g_signal_connect (clutter_actor_get_stage (actor), "captured-event",
                       G_CALLBACK (manipulate_move_capture), actor);
  clutter_actor_queue_redraw (actor);
  return TRUE;
}


static gboolean
manipulate_resize_capture (ClutterActor *stage,
                           ClutterEvent *event,
                           gpointer      data)
{
  switch (event->any.type)
    {
      case CLUTTER_MOTION:
        {
          ClutterActor *actor = cs_selected_get_any();
          /* resize is semi bust for more than one actor */
          gfloat ex, ey;
          clutter_actor_transform_stage_point (actor, event->motion.x, event->motion.y,
                                               &ex, &ey);
          gfloat w, h;
          clutter_actor_get_size (actor, &w, &h);

          w-= manipulate_x-ex;
          h-= manipulate_y-ey;

          snap_size (actor, w, h, &w, &h);

          clutter_actor_set_size (actor, w, h);
          cs_prop_tweaked (G_OBJECT (actor), "width");
          cs_prop_tweaked (G_OBJECT (actor), "height");
          cs_dirtied ();

          manipulate_x=ex;
          manipulate_y=ey;
        }
        break;
      case CLUTTER_BUTTON_RELEASE:
        hor_pos = 0;
        ver_pos = 0;
        clutter_actor_queue_redraw (stage);
        g_signal_handler_disconnect (stage,
                                     manipulate_capture_handler);
        manipulate_capture_handler = 0;
      default:
        break;
    }
  return TRUE;
}

gboolean manipulate_resize_start (ClutterActor  *actor,
                                  ClutterEvent  *event)
{
  ClutterActor *first_actor = cs_selected_get_any();
  manipulate_x = event->button.x;
  manipulate_y = event->button.y;

  clutter_actor_transform_stage_point (first_actor, event->button.x, event->button.y, &manipulate_x, &manipulate_y);

  manipulate_capture_handler = 
     g_signal_connect (clutter_actor_get_stage (actor), "captured-event",
                       G_CALLBACK (manipulate_resize_capture), actor);
  return TRUE;
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

  clutter_actor_get_transformed_position (cs_find_by_id_int (clutter_actor_get_stage (cluttersmith->parasite_root), "fake-stage-rect"), &offset_x, &offset_y);

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
  
  g_print ("%f %f %f %f\n", x, target_x, origin_x, zoom);

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
          /* resize is semi bust for more than one actor */
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
        g_signal_handler_disconnect (stage,
                                     manipulate_capture_handler);
        if (manipulate_x == manipulate_pan_start_x &&
            manipulate_y == manipulate_pan_start_y)
          {
            do_zoom (!(event->button.modifier_state & CLUTTER_SHIFT_MASK), manipulate_x, manipulate_y);
          }
        manipulate_capture_handler = 0;
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

  manipulate_capture_handler = 
     g_signal_connect (clutter_actor_get_stage (event->any.source), "captured-event",
                       G_CALLBACK (manipulate_pan_capture), NULL);
  return TRUE;
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

/* XXX: should be changed to deal with transformed coordinates to be able to
 * deal correctly with actors at any transformation and nesting.
 */
static gboolean
contains (gint min, gint max, gint minb, gint maxb)
{
  if (minb>=min && minb <=max &&
      maxb>=min && maxb <=max)
    return TRUE;
  return FALSE;
}

#define LASSO_BORDER 1


static gboolean
manipulate_lasso_capture (ClutterActor *stage,
                          ClutterEvent *event,
                          gpointer      data)
{
  switch (event->any.type)
    {
      case CLUTTER_MOTION:
        {
          gfloat ex=event->motion.x;
          gfloat ey=event->motion.y;

          gint mx = MIN (ex, lx);
          gint my = MIN (ey, ly);
          gint mw = MAX (ex, lx) - mx;
          gint mh = MAX (ey, ly) - my;

          clutter_actor_set_position (lasso, mx - LASSO_BORDER, my - LASSO_BORDER);
          clutter_actor_set_size (lasso, mw + LASSO_BORDER*2, mh+LASSO_BORDER*2);

          manipulate_x=ex;
          manipulate_y=ey;


          {
            gint no;
            GList *j, *list;
#if 0
            ClutterActor *sibling = cs_selected_get_any ();

            if (!sibling)
              {
                GHashTableIter      iter;
                gpointer            key = NULL, value;

                g_hash_table_iter_init (&iter, selection);
                g_hash_table_iter_next (&iter, &key, &value);
                if (key)
                  {
                    sibling = key;
                  }
              }
            
            g_hash_table_remove_all (selection);

            if (sibling)
              {
                list = get_siblings (sibling);
              }
            else
              {
                list = cs_container_get_children_recursive (stage);
              }
#else
            g_hash_table_remove_all (selection);
            list = clutter_container_get_children (CLUTTER_CONTAINER (cs_get_current_container ()));
#endif

            for (no = 0, j=list; j;no++,j=j->next)
              {
                gfloat cx, cy;
                gfloat cw, ch;
                clutter_actor_get_transformed_position (j->data, &cx, &cy);
                clutter_actor_get_transformed_size (j->data, &cw, &ch);

                if (contains (mx, mx + mw, cx, cx + cw) &&
                    contains (my, my + mh, cy, cy + ch))
                  {
                    g_hash_table_insert (selection, j->data, j->data);
                  }
              }
            g_list_free (list);
          }
        }
        break;
      case CLUTTER_BUTTON_RELEASE:
         {
          ClutterModifierType state = event->button.modifier_state;
          GHashTableIter      iter;
          gpointer            key, value;

          g_hash_table_iter_init (&iter, selection);
          while (g_hash_table_iter_next (&iter, &key, &value))
            {
              if (state & CLUTTER_CONTROL_MASK)
                {
                  if (cs_selected_has_actor (key))
                    cs_selected_remove (key);
                  else
                    cs_selected_add (key);
                }
              else
                {
                  cs_selected_add (key);
                }
            }
        }
        g_hash_table_remove_all (selection);

        clutter_actor_queue_redraw (stage);
        g_signal_handler_disconnect (stage,
                                     manipulate_capture_handler);
        manipulate_capture_handler = 0;
        clutter_actor_destroy (lasso);
        lasso = NULL;
      default:
        break;
    }
  return TRUE;
}

static gboolean manipulate_lasso_start (ClutterActor  *actor,
                                        ClutterEvent  *event)
{
  ClutterModifierType state = event->button.modifier_state;

  if (!((state & CLUTTER_SHIFT_MASK) ||
        (state & CLUTTER_CONTROL_MASK)))
    {
      cs_selected_clear ();
    }

  g_assert (lasso == NULL);

    {
      ClutterColor lassocolor       = {0xff,0x0,0x0,0x11};
      ClutterColor lassobordercolor = {0xff,0x0,0x0,0x88};
      lasso = clutter_rectangle_new_with_color (&lassocolor);
      clutter_rectangle_set_border_color (CLUTTER_RECTANGLE (lasso), &lassobordercolor);
      clutter_rectangle_set_border_width (CLUTTER_RECTANGLE (lasso), LASSO_BORDER);
      clutter_container_add_actor (CLUTTER_CONTAINER (cluttersmith->parasite_root), lasso);
    }
  lx = event->button.x;
  ly = event->button.y;

  clutter_actor_set_position (lasso, lx-LASSO_BORDER, ly-LASSO_BORDER);
  clutter_actor_set_size (lasso, LASSO_BORDER*2, LASSO_BORDER*2);

  manipulate_x = event->button.x;
  manipulate_y = event->button.y;

  manipulate_capture_handler = 
     g_signal_connect (clutter_actor_get_stage (actor), "captured-event",
                       G_CALLBACK (manipulate_lasso_capture), actor);

  return TRUE;
}

static ClutterActor *edited_actor = NULL;
static gboolean text_was_editable = FALSE;
static gboolean text_was_reactive = FALSE;

gboolean edit_text_start (ClutterActor *actor)
{
  if (MX_IS_LABEL (actor))
    actor = mx_label_get_clutter_text (MX_LABEL (actor));
  edited_actor = actor;

  g_object_get (edited_actor, "editable", &text_was_editable,
                             "reactive", &text_was_reactive, NULL);
  g_object_set (edited_actor, "editable", TRUE,
                             "reactive", TRUE, NULL);
  clutter_stage_set_key_focus (CLUTTER_STAGE (clutter_actor_get_stage (actor)), actor);
  return TRUE;
}

static gboolean edit_text_end (void)
{
  g_object_set (edited_actor, "editable", text_was_editable,
                              "reactive", text_was_reactive, NULL);
  edited_actor = NULL;
  cs_dirtied ();
  return TRUE;
}

gboolean cs_edit_actor_start (ClutterActor *actor)
{
  const gchar *name;
  if (!actor)
    return FALSE;
  if (CLUTTER_IS_TEXT (actor) ||
      MX_IS_LABEL (actor))
    return edit_text_start (actor);

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
  g_print ("unahndled enter edit for %s\n", G_OBJECT_TYPE_NAME (actor));

  return FALSE;
}

gboolean cs_edit_actor_end (void)
{
  if (!edited_actor)
    return FALSE;
  if (CLUTTER_IS_TEXT (edited_actor) ||
      MX_IS_LABEL (edited_actor))
    return edit_text_end ();
  return FALSE;
}

gint cs_last_x = 0;
gint cs_last_y = 0;

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
                      selection_popup (x,y);
                      return FALSE;
                    }
                  else
                    {
                      object_popup (cs_selected_get_any (), x,y);
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

                   manipulate_move_start (cluttersmith->parasite_root, event);
                }
            }
          else 
            { 
              gboolean stage_child = FALSE;
              if (clutter_event_get_button (event) == 3)
                {
                  root_popup (x,y);
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
                  manipulate_move_start (cluttersmith->parasite_root, event);
                }
              else
                {
                  manipulate_lasso_start (cluttersmith->parasite_root, event);
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
      playback_popup (event->button.x, event->button.y);
      return TRUE;
    }
  return FALSE;
}

void cs_manipulate_init (ClutterActor *actor)
{
  if (stage_capture_handler)
    {
      g_signal_handler_disconnect (clutter_actor_get_stage (actor),
                                   stage_capture_handler);
      stage_capture_handler = 0;
    }

  stage_capture_handler = 
     g_signal_connect_after (clutter_actor_get_stage (actor), "captured-event",
                             G_CALLBACK (manipulate_capture), NULL);
  g_signal_connect (clutter_actor_get_stage (actor), "button-press-event",
                    G_CALLBACK (playback_context), NULL);
}

gboolean cs_canvas_handler_enter (ClutterActor *actor)
{
  clutter_actor_set_opacity (actor, 255);
  return TRUE;
}


gboolean cs_canvas_handler_leave (ClutterActor *actor)
{
  clutter_actor_set_opacity (actor, 100);
  return TRUE;
}

static ClutterActor *current_container = NULL;


void cs_set_current_container (ClutterActor *actor)
{
  if (actor && CLUTTER_IS_CONTAINER (actor))
    current_container = actor;
}

ClutterActor *cs_get_current_container (void)
{
  if (!current_container)
    return cluttersmith->fake_stage;
  return current_container;
}

