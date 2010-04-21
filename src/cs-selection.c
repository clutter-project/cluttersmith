#include <clutter/clutter.h>
#include <stdlib.h>
#include <string.h>
#include "cluttersmith.h"

static GList      *selected = NULL;
static gint        lx, ly;
ClutterActor      *lasso;
static GHashTable *selection = NULL; /* what would be added/removed by
                                        current lasso*/
static gfloat manipulate_x;
static gfloat manipulate_y;

void cs_selected_init (void);
void init_multi_select (void)
{
  selection = g_hash_table_new_full (g_direct_hash, g_direct_equal, NULL, NULL);
  cs_selected_init ();
}


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

void cs_selected_init (void)
{
  /*selected = g_hash_table_new_full (g_direct_hash, g_direct_equal, NULL, NULL);*/
}

GList *cs_selected_get_list (void)
{
  return g_list_copy (selected);
}

static void update_active_actor (void)
{
  if (g_list_length (selected)==1)
    {
      cs_set_active (cs_selected_get_any ());
    }
  else
    {
      cs_set_active (NULL);
    }
  if (g_list_length (selected)>0)
    clutter_stage_set_key_focus (CLUTTER_STAGE (clutter_actor_get_stage (cluttersmith->parasite_root)), NULL);
}

void cs_selected_add (ClutterActor *actor)
{
  if (!g_list_find (selected, actor))
    selected = g_list_append (selected, actor);
  update_active_actor ();
}

void cs_selected_remove (ClutterActor *actor)
{
  if (g_list_find (selected, actor))
    selected = g_list_remove (selected, actor);
  update_active_actor ();
}

static void each_add_to_list (ClutterActor *actor,
                               gpointer      string)
{
  g_string_append_printf (string, "$(\"%s\"),", cs_get_id (actor));
}

void cs_selected_foreach (GCallback cb, gpointer data)
{
  void (*each)(ClutterActor *actor, gpointer data)=(void*)cb;
  GList *s;

  for (s = selected; s; s = s->next)
    {
      ClutterActor *actor = s->data;
      if (actor != clutter_actor_get_stage (actor))
        each(actor, data);
    }
}

gpointer cs_selected_match (GCallback match_fun, gpointer data)
{
  gpointer ret = NULL;
  ret = cs_list_match (selected, match_fun, data);
  return ret;
}

void cs_selected_clear (void)
{
  if (selected)
    g_list_free (selected);
  selected = NULL;
  update_active_actor ();
}


gint cs_selected_count (void)
{
  return g_list_length (selected);
}


gboolean cs_selected_has_actor (ClutterActor *actor)
{
  if (g_list_find (selected, actor))
    return TRUE;
  return FALSE;
}


/**
 * cs_selected_get_any:
 *
 * Return value: (transfer none): any single of the selected actors if any or NULL
 */

ClutterActor *cs_selected_get_any (void)
{
  if (selected)
    return selected->data;
  return NULL;
}



#define LASSO_BORDER 1

static GString *undo = NULL;
static GString *redo = NULL;

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
        g_signal_handlers_disconnect_by_func (stage, manipulate_lasso_capture, data);
        clutter_actor_destroy (lasso);
        lasso = NULL;
        SELECT_ACTION_POST("select lasso");
      default:
        break;
    }
  return TRUE;
}

gboolean
cs_selected_lasso_start (ClutterActor  *actor,
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

  g_signal_connect (clutter_actor_get_stage (actor), "captured-event",
                    G_CALLBACK (manipulate_lasso_capture), actor);
  undo = g_string_new ("");
  redo = g_string_new ("");
  SELECT_ACTION_PRE2();

  return TRUE;
}


void cs_selected_paint (void)
{
  ClutterVertex verts[4];
  /* draw outlines for actors */
    GHashTableIter      iter;
    gpointer            key, value;

    {
        {
          if (cluttersmith->fake_stage)
            {
              cogl_set_source_color4ub (0, 255, 0, 255);
              cs_draw_actor_outline (cluttersmith->fake_stage, NULL);
            }
        }
    }

    cogl_set_source_color4ub (255, 0, 0, 128);
    cs_selected_foreach (G_CALLBACK (cs_draw_actor_outline), NULL);

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
