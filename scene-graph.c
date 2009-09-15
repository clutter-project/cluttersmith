#include <clutter/clutter.h>
#include <nbtk/nbtk.h>
#include <stdlib.h>
#include <string.h>
#include "cluttersmith.h"
#include "util.h"

static ClutterColor  white = {0xff,0xff,0xff,0xff};  /* XXX: should be in CSS */
static ClutterColor  yellow = {0xff,0xff,0x44,0xff};

static ClutterActor *clone = NULL;
static ClutterActor *dragged_item = NULL;
static ClutterActor *dropped_on_target = NULL;
static glong  tree_item_capture_handler = 0;
static gfloat tree_item_x;
static gfloat tree_item_y;

static ClutterActor *get_drop_target (ClutterActor *actor)
{
  if (!actor)
    return NULL;
  actor = g_object_get_data (G_OBJECT (actor), "actor");
  if (util_has_ancestor (actor, dragged_item))
    return NULL;
  if (CLUTTER_IS_CONTAINER (actor))
    return actor;
  return NULL;
}

static ClutterActor *get_nth_child (ClutterContainer *container, gint n)
{
  GList *list = clutter_container_get_children (container);
  ClutterActor *ret = g_list_nth_data (list, n);
  g_list_free (list);
  return ret;
}

static gboolean
tree_item_capture (ClutterActor *stage, ClutterEvent *event, gpointer data)
{
  switch (event->any.type)
    {
      case CLUTTER_MOTION:
        {
          clutter_actor_set_position (clone, event->motion.x, event->motion.y);
        }
        break;
      case CLUTTER_ENTER:
          {
            dropped_on_target = get_drop_target (event->any.source);
          }
        break;
      case CLUTTER_BUTTON_RELEASE:
        clutter_actor_destroy (clone);
        clone = NULL;

        if (dropped_on_target)
          {
            ClutterActor *dragged_actor = g_object_get_data (data, "actor");

            /* Determine where to add the actor, 0 is before existing
             * children 1 is after first child
             */


            if (dragged_actor &&
                !util_has_ancestor (dropped_on_target, dragged_actor) &&
                (dropped_on_target != dragged_actor))
              {
                gfloat x, y;

                clutter_actor_transform_stage_point (event->any.source, event->button.x, event->button.y, &x, &y);

                dropped_on_target = get_drop_target (event->any.source);


                if (dropped_on_target)
                  {
                    gint   add_self = 0;
                    gint   use_child = 0;
                    gint   child_no = 0;
                    ClutterActor *real_container = NULL;
                    GList *e, *existing_children;

                    g_object_ref (dragged_actor);
                    clutter_container_remove_actor (CLUTTER_CONTAINER (clutter_actor_get_parent (dragged_actor)), dragged_actor);
                    g_print ("[%p]\n", clutter_actor_get_parent (dragged_actor));

                    real_container = g_object_get_data (G_OBJECT(event->any.source),"actor");

                    existing_children = clutter_container_get_children (
                        CLUTTER_CONTAINER (get_nth_child (CLUTTER_CONTAINER(event->any.source),1)));
                    for (e=existing_children; e; e = e->next, child_no++)
                      {
                        gfloat child_y2;
                        clutter_actor_transform_stage_point (e->data, 0, event->button.y, NULL, &child_y2);
                        if (use_child == 0 && child_no>=0 &&
                            g_object_get_data (e->data, "actor") == dragged_actor)
                          add_self = 1;
                        if (child_y2 > 0)
                          {
                            use_child = child_no + 1;
                          }
                      }

                    g_list_free (existing_children);

                    existing_children = clutter_container_get_children (CLUTTER_CONTAINER (real_container));

                    for (e=existing_children; e; e = e->next)
                      {
                        g_object_ref (e->data);
                        clutter_container_remove_actor (CLUTTER_CONTAINER (real_container), e->data);
                      }

                    child_no = 0;
                    for (e=existing_children; e; e = e->next, child_no++)
                      {

                        if (use_child == 0 && child_no == 0)
                          {
                            g_print ("droppedA in %i %i %p\n", child_no, add_self, dragged_actor);
                            clutter_container_add_actor (CLUTTER_CONTAINER (real_container), dragged_actor);
                            use_child = -1;
                          }

                        g_assert (e->data != dragged_actor);
                        clutter_container_add_actor (CLUTTER_CONTAINER (real_container), e->data);
                        g_object_unref (e->data);

                        if (child_no == use_child-1 - add_self || (e->next == NULL && use_child != -1))
                          {
                            g_print ("droppedB in %i %i %p\n", child_no, add_self, dragged_actor);
                            clutter_container_add_actor (CLUTTER_CONTAINER (real_container), dragged_actor);
                            use_child = -1;
                          }

                      }

                    g_list_free (existing_children);
                  }
                g_object_unref (dragged_actor);
              }
            if (dragged_actor)
              select_item (dragged_actor);
          }
        dropped_on_target = NULL;
        dragged_item = NULL;
        g_signal_handler_disconnect (stage,
                                     tree_item_capture_handler);
        tree_item_capture_handler = 0;
      default:
        break;
    }
  return TRUE;
}


static gboolean tree_item_press (ClutterActor  *actor,
                                 ClutterEvent  *event)
{
  tree_item_x = event->button.x;
  tree_item_y = event->button.y;

  if (CLUTTER_IS_STAGE (g_object_get_data (G_OBJECT (actor), "actor")))
    return TRUE;

  tree_item_capture_handler = 
     g_signal_connect (clutter_actor_get_stage (actor), "captured-event",
                       G_CALLBACK (tree_item_capture), actor);

  dragged_item = actor;
  clone = clutter_clone_new (actor);
  clutter_actor_set_opacity (clone, 0xbb);
  clutter_actor_set_position (clone, event->button.x, event->button.y);
  clutter_container_add_actor (CLUTTER_CONTAINER (clutter_actor_get_stage(actor)), clone);

  return TRUE;
}

static gboolean select_item_event (ClutterActor *button, ClutterEvent *event, ClutterActor *item)
{
  select_item (item);
  return TRUE;
}


static void
tree_populate_iter (ClutterActor *current_container,
                    ClutterActor *active_actor,
                    ClutterActor *iter,
                    gint   *count,
                    gint   *level)
{
  ClutterActor *vbox;
  ClutterActor *label;

  if (iter == NULL ||
#ifdef EDIT_SELF
      util_has_ancestor (iter, scene_graph)
#else
      util_has_ancestor (iter, parasite_root)
#endif
      )
    {
      return;
    }

  vbox = g_object_new (NBTK_TYPE_BOX_LAYOUT, "min-width", 100.0, "natural-width", 500.0, "vertical", TRUE,
                       "style-class", 
                
                       ((*count)%2==0)?
                          ((*level)%2==0)?"ParasiteTreeA1":"ParasiteTreeB1":
                          ((*level)%2==0)?"ParasiteTreeA2":"ParasiteTreeB2",
                      
                       NULL);

                       //((*level)%2==0)?"ParasiteTreeA":"ParasiteTreeB",

  label = clutter_text_new_with_text ("Sans 14px", G_OBJECT_TYPE_NAME (iter));
  clutter_actor_set_anchor_point (label, -24.0, 0.0);
  if (iter == active_actor)
    {
      clutter_text_set_color (CLUTTER_TEXT (label), &yellow);
    }
  else
    {
      clutter_text_set_color (CLUTTER_TEXT (label), &white);
    }
  g_signal_connect (label, "button-press-event", G_CALLBACK (select_item_event), iter);
  clutter_actor_set_reactive (label, TRUE);

  clutter_container_add_actor (CLUTTER_CONTAINER (vbox), label);

  clutter_container_add_actor (CLUTTER_CONTAINER (current_container), vbox);
  g_object_set_data (G_OBJECT (vbox), "actor", iter);

  if (CLUTTER_IS_CONTAINER (iter))
    {
      ClutterActor *child_vbox;
      GList *children, *c;
      children = clutter_container_get_children (CLUTTER_CONTAINER (iter));

      child_vbox = g_object_new (NBTK_TYPE_BOX_LAYOUT, "vertical", TRUE, NULL);
      clutter_container_add_actor (CLUTTER_CONTAINER (vbox), child_vbox);
      clutter_actor_set_anchor_point (child_vbox, -24.0, 0.0);

      /*g_signal_connect (vbox, "button-press-event", G_CALLBACK (vbox_press), child_vbox);*/

      (*level) = (*level)+1;
      for (c = children; c; c=c->next)
        {
          tree_populate_iter (child_vbox, active_actor, c->data, level, count);
        }
      (*level) = (*level)-1;
      g_list_free (children);
    }
  else
    {
      /*g_signal_connect (vbox, "button-press-event", G_CALLBACK (vbox_nop), NULL);*/
    }

  g_signal_connect (vbox, "button-press-event", G_CALLBACK (tree_item_press), NULL);
  clutter_actor_set_reactive (vbox, TRUE);
  (*count) = (*count)+1;
}

void
tree_populate (ClutterActor *scene_graph,
               ClutterActor *active_actor)
{
  gint level = 0;
  gint count = 0;
  util_remove_children (scene_graph);
  clutter_actor_set_width (scene_graph, 230);
  tree_populate_iter (scene_graph, active_actor, clutter_actor_get_stage (active_actor), &level, &count);
}


