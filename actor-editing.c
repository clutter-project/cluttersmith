#include <clutter/clutter.h>
#include <nbtk/nbtk.h>
#include <stdlib.h>
#include "cluttersmith.h"
#include "util.h"

static guint stage_capture_handler = 0;

ClutterActor      *lasso;
static gint        lx, ly;
static GHashTable *selection = NULL; /* what would be added/removed by
                                        current lasso*/
GHashTable *selected  = NULL;


static void init_multi_select (void)
{
  selection = g_hash_table_new_full (g_direct_hash, g_direct_equal, NULL, NULL);
  selected = g_hash_table_new_full (g_direct_hash, g_direct_equal, NULL, NULL);

}

GList *cluttersmith_get_selected (void)
{
  GList *ret = g_hash_table_get_keys (selected);
  if (!ret)
    {
      if (active_actor)
        ret = g_list_append (NULL, active_actor);
    }
  return ret;
}

void cluttersmith_selected_foreach (GCallback cb, gpointer data)
{
  void (*each)(ClutterActor *actor, gpointer data)=(void*)cb;
  GList *s, *selected;
  selected = cluttersmith_get_selected ();
  s=selected;
  for (s=selected; s; s=s->next)
    {
      ClutterActor *actor = s->data;
      if (actor == clutter_actor_get_stage (actor))
        continue;
      each(actor, data);
    }
  g_list_free (selected);
}


void cluttersmith_clear_selected (void)
{
  g_hash_table_remove_all (selected);
}


gchar *subtree_to_string (ClutterActor *root);

ClutterActor *active_actor = NULL;


/* snap positions, in relation to actor */
static gint ver_pos = 0; /* 1 = start 2 = mid 3 = end */
static gint hor_pos = 0; /* 1 = start 2 = mid 3 = end */

static void
cb_overlay_paint (ClutterActor *actor,
                  gpointer      user_data)
{
  ClutterVertex verts[4];

  if (!active_actor && g_hash_table_size (selected)==0)
    return;

  if (!CLUTTER_IS_STAGE (active_actor))
    {
      clutter_actor_get_abs_allocation_vertices (active_actor,
                                                 verts);

      cogl_set_source_color4ub (0, 25, 0, 50);

      {
        CoglTextureVertex tverts[] = 
           {
             {verts[0].x, verts[0].y, },
             {verts[2].x, verts[2].y, },
             {verts[3].x, verts[3].y, },
             {verts[1].x, verts[1].y, },
           };
        cogl_polygon (tverts, 4, FALSE);

        {
          gfloat coords[]={ verts[0].x, verts[0].y, 
             verts[1].x, verts[1].y, 
             verts[3].x, verts[3].y, 
             verts[2].x, verts[2].y, 
             verts[0].x, verts[0].y };

          cogl_path_polyline (coords, 5);
          cogl_set_source_color4ub (255, 0, 0, 128);
          cogl_path_stroke ();

            {
              gint i;
              for (i=-1;i<3;i++)
                {
                  gfloat coords[]={ 
                 (verts[1].x+verts[3].x)/2+i, (verts[1].y+verts[3].y)/2+i, 
                 verts[3].x+1, verts[3].y+i, 
                 (verts[2].x+verts[3].x)/2+i, (verts[2].y+verts[3].y)/2+i};
                 cogl_path_polyline (coords, 3);
                 cogl_set_source_color4ub (255, 0, 0, 128);
                 cogl_path_stroke ();
            }
            }
        }

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
  /* draw selected */
    GHashTableIter      iter;
    gpointer            key, value;

    g_hash_table_iter_init (&iter, selected);
    while (g_hash_table_iter_next (&iter, &key, &value))
      {
        clutter_actor_get_abs_allocation_vertices (key,
                                                   verts);

        cogl_set_source_color4ub (0, 25, 0, 50);

        {
          {
            gfloat coords[]={ verts[0].x, verts[0].y, 
               verts[1].x, verts[1].y, 
               verts[3].x, verts[3].y, 
               verts[2].x, verts[2].y, 
               verts[0].x, verts[0].y };

            cogl_path_polyline (coords, 5);
            cogl_set_source_color4ub (255, 0, 0, 128);
            cogl_path_stroke ();
          }
        }
      }

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

}

void actor_editing_init (gpointer stage)
{
  g_signal_connect_after (stage, "paint", G_CALLBACK (cb_overlay_paint), NULL);
  init_multi_select ();
}




static guint  manipulate_capture_handler = 0;
static gfloat manipulate_x;
static gfloat manipulate_y;

#define SNAP_THRESHOLD  2

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

static gboolean
manipulate_move_capture (ClutterActor *stage, ClutterEvent *event, gpointer data)
{
  switch (event->any.type)
    {
      case CLUTTER_MOTION:
        {
          gfloat x, y;
          clutter_actor_get_position (data, &x, &y);
          x-= manipulate_x-event->motion.x;
          y-= manipulate_y-event->motion.y;

          snap_position (data, x, y, &x, &y);

          clutter_actor_set_position (data, x, y);
          CS_REVISION++;

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


static gboolean manipulate_move_press (ClutterActor  *actor,
                                  ClutterEvent  *event)
{
  manipulate_x = event->button.x;
  manipulate_y = event->button.y;

  manipulate_capture_handler = 
     g_signal_connect (clutter_actor_get_stage (actor), "captured-event",
                       G_CALLBACK (manipulate_move_capture), actor);
  return TRUE;
}


static gboolean
manipulate_resize_capture (ClutterActor *stage, ClutterEvent *event, gpointer data)
{
  switch (event->any.type)
    {
      case CLUTTER_MOTION:
        {
          gfloat ex, ey;
          clutter_actor_transform_stage_point (data, event->motion.x, event->motion.y,
                                               &ex, &ey);
          gfloat w, h;
          clutter_actor_get_size (data, &w, &h);

          w-= manipulate_x-ex;
          h-= manipulate_y-ey;

          snap_size (data, w, h, &w, &h);

          clutter_actor_set_size (data, w, h);
          CS_REVISION++;

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

static gboolean manipulate_resize_press (ClutterActor  *actor,
                                         ClutterEvent  *event)
{
  manipulate_x = event->button.x;
  manipulate_y = event->button.y;

  clutter_actor_transform_stage_point (actor, event->button.x, event->button.y, &manipulate_x, &manipulate_y);

  manipulate_capture_handler = 
     g_signal_connect (clutter_actor_get_stage (actor), "captured-event",
                       G_CALLBACK (manipulate_resize_capture), actor);
  return TRUE;
}



static gboolean
intersects (gint min, gint max, gint minb, gint maxb)
{
  if (minb <= max && minb >= min)
    return TRUE;
  if (min <= maxb && min >= minb)
    return TRUE;
  return FALSE;
}

#define LASSO_BORDER 1


static gboolean
manipulate_lasso_capture (ClutterActor *stage, ClutterEvent *event, gpointer data)
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

          g_hash_table_remove_all (selection);

          {
            gint no;
            GList *j, *list;
           
            list = clutter_container_get_children_recursive (stage);

            for (no = 0, j=list; j;no++,j=j->next)
              {
                gfloat cx, cy;
                gfloat cw, ch;
                clutter_actor_get_transformed_position (j->data, &cx, &cy);
                clutter_actor_get_transformed_size (j->data, &cw, &ch);

                if (intersects (mx, mx + mw, cx, cx + cw) &&
                    intersects (my, my + mh, cy, cy + ch))
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
                  if (g_hash_table_lookup (selected, key))
                    {
                      g_hash_table_remove (selected, key);
                    }
                  else
                    {
                      g_hash_table_insert (selected, key, value);
                    }
                }
              else
                {
                  g_hash_table_insert (selected, key, value);
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



static gboolean manipulate_select_press (ClutterActor  *actor,
                                         ClutterEvent  *event)
{
   ClutterActor *hit;
   ClutterModifierType state = event->button.modifier_state;


   hit = clutter_stage_get_actor_at_pos (CLUTTER_STAGE (clutter_actor_get_stage (actor)),
                                         CLUTTER_PICK_ALL,
                                         event->button.x, event->button.y);

  if (!((state & CLUTTER_SHIFT_MASK) ||
        (state & CLUTTER_CONTROL_MASK)))
    {
      g_hash_table_remove_all (selected);
    }

  if (hit == clutter_actor_get_stage (actor))
    {
      if (lasso == NULL)
        {
          ClutterColor lassocolor       = {0xff,0x0,0x0,0x11};
          ClutterColor lassobordercolor = {0xff,0x0,0x0,0x88};
          lasso = clutter_rectangle_new_with_color (&lassocolor);
          clutter_rectangle_set_border_color (CLUTTER_RECTANGLE (lasso), &lassobordercolor);
          clutter_rectangle_set_border_width (CLUTTER_RECTANGLE (lasso), LASSO_BORDER);
          clutter_container_add_actor (CLUTTER_CONTAINER (parasite_root), lasso);
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
    }
  else
    {
#ifdef EDIT_SELF
     select_item (hit);
#else
     if (!util_has_ancestor (hit, parasite_root) || 1)
       select_item (hit);
     else
       g_print ("child of foo!\n");
#endif
    }


  return TRUE;
}



typedef enum RunMode {
  RUN_MODE_UI   = 1,
  RUN_MODE_EDIT = 2
} RunMode;

static gint run_mode = RUN_MODE_EDIT|RUN_MODE_UI;

static ClutterActor *edited_text = NULL;
static gboolean text_was_editable = FALSE;
static gboolean text_was_reactive = FALSE;

static gboolean edit_text_start (ClutterActor *actor)
{
  if (NBTK_IS_LABEL (actor))
    actor = nbtk_label_get_clutter_text (NBTK_LABEL (actor));
  edited_text = actor;

  g_object_get (edited_text, "editable", &text_was_editable,
                             "reactive", &text_was_reactive, NULL);
  g_object_set (edited_text, "editable", TRUE,
                             "reactive", TRUE, NULL);
  clutter_stage_set_key_focus (CLUTTER_STAGE (clutter_actor_get_stage (actor)), actor);
  return TRUE;
}

static gboolean edit_text_end (void)
{
  g_object_set (edited_text, "editable", text_was_editable,
                             "reactive", text_was_reactive, NULL);
  edited_text = NULL;
  return TRUE;
}

gboolean manipulator_key_pressed (ClutterActor *stage, ClutterModifierType modifier, guint key);

static gboolean
manipulate_capture (ClutterActor *actor, ClutterEvent *event, gpointer data)
{
  /* pass events through to text being edited */
  if (edited_text)
    { 
      if (event->any.type == CLUTTER_KEY_PRESS && event->key.keyval == CLUTTER_Escape)
        {
          edit_text_end ();
          return TRUE;
        }
      if (event->any.source == edited_text)
        return FALSE;
      else
        {
          /* break out when presses occur outside the ClutterText,
           * this currently means that proper mouse movement and
           * selection is not possible for NbtkLabels and Entries
           * when edited
           */
          switch (event->any.type)
            {
              case CLUTTER_BUTTON_PRESS:
                edit_text_end ();
              default:
                return TRUE;
            }
        }
    }

  if (event->any.type == CLUTTER_KEY_PRESS)
    {
      if (event->key.keyval == CLUTTER_Scroll_Lock ||
          event->key.keyval == CLUTTER_Caps_Lock)
        {
          switch (run_mode)
            {
              case 0:
                run_mode = 2;
                g_print ("Run mode : edit only\n");
                break;
              case 2:
                run_mode = 3;
                g_print ("Run mode : ui with edit\n");
                break;
              default: 
                run_mode = 0;
                g_print ("Run mode : browse\n");
                break;
            }

          if (run_mode & RUN_MODE_UI)
            {
              clutter_actor_show (parasite_ui);
            }
          else
            {
              clutter_actor_hide (parasite_ui);
            }
          return TRUE;
        }
    }

  if (!(run_mode & RUN_MODE_EDIT))
    {
      /* check if it is child of a link, if it is then we override anyways...
       *
       * Otherwise this is the case that slips contorl through to the
       * underlying scene graph.
       */
      if (event->any.type == CLUTTER_BUTTON_PRESS)
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
                   gchar *filename;
                   filename = g_strdup_printf ("json/%s.json", name+5);

                   set_title (name+5);
                   select_item (clutter_actor_get_stage (parasite_root));
                   return TRUE;
                 }
               hit = clutter_actor_get_parent (hit);
             }
          
        } 
      return FALSE;
    }
  
  if ((clutter_get_motion_events_enabled()==FALSE) ||
      util_has_ancestor (event->any.source, parasite_root))
    {
      return FALSE;
    }
  switch (event->any.type)
    {
      case CLUTTER_MOTION:
        {
#if 0
          ClutterActor *hit;

          hit = clutter_stage_get_actor_at_pos (CLUTTER_STAGE (clutter_actor_get_stage (actor)),
                                                CLUTTER_PICK_ALL,
                                                event->motion.x, event->motion.y);
          if (!util_has_ancestor (hit, parasite_root))
            select_item (hit);
          else
            g_print ("child of foo!\n");
#endif
        }
        break;
      case CLUTTER_BUTTON_PRESS:
        {
          gfloat x = event->button.x;
          gfloat y = event->button.y;
          gfloat w,h;

          if (active_actor && clutter_event_get_click_count (event) > 1)
            {
              if (CLUTTER_IS_TEXT (active_actor) ||
                  NBTK_IS_LABEL (active_actor))
                {
                  edit_text_start (active_actor);
                  return TRUE;
                }
            }

          if (active_actor)
            {
              clutter_actor_get_size (active_actor, &w, &h);
              clutter_actor_transform_stage_point (active_actor, x, y, &x, &y);
              x/=w;
              y/=h;
            }

          if (x<0.0 || y < 0.0 || x > 1.0 || y > 1.0 ||
              active_actor == NULL ||
              (active_actor && (active_actor == clutter_actor_get_stage (active_actor))))
            {
              manipulate_select_press (parasite_root, event);
            }
          else if (x>0.5 && y>0.5)
            {
              manipulate_resize_press (active_actor, event);
            }
          else
            {
              manipulate_move_press (active_actor, event);
            }

        }
        break;
      case CLUTTER_BUTTON_RELEASE:
        return TRUE;
      case CLUTTER_KEY_PRESS:
        manipulator_key_pressed (actor, clutter_event_get_state(event), event->key.keyval);
      case CLUTTER_ENTER:
      case CLUTTER_LEAVE:
        return FALSE;
      default:
        break;
    }
  return TRUE;
}

void cb_manipulate_init (ClutterActor *actor)
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
}
