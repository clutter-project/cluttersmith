#include <clutter/clutter.h>
#include <mx/mx.h>
#include <stdlib.h>
#include <string.h>
#include "cluttersmith.h"

static gfloat manipulate_x;
static gfloat manipulate_y;

static gfloat manipulate_pan_start_x = 0;
static gfloat manipulate_pan_start_y = 0;


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

          g_object_get (cs, "origin-x", &originx,
                            "origin-y", &originy,
                            NULL);
          originx += manipulate_x-ex;
          originy += manipulate_y-ey;
          g_object_set (cs, "origin-x", originx,
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
            cs_zoom (!(event->button.modifier_state & CLUTTER_SHIFT_MASK), manipulate_x, manipulate_y);
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



