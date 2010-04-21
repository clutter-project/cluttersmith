#include <clutter/clutter.h>
#include <mx/mx.h>
#include <stdlib.h>
#include <string.h>
#include "cluttersmith.h"

static gboolean text_was_editable = FALSE;
static gboolean text_was_reactive = FALSE;

static void
edit_text_start (ClutterActor *actor)
{
  if (MX_IS_LABEL (actor))
    actor = mx_label_get_clutter_text (MX_LABEL (actor));

  g_object_get (actor, "editable", &text_was_editable,
                       "reactive", &text_was_reactive, NULL);
  g_object_set (actor, "editable", TRUE,
                       "reactive", TRUE, NULL);
  clutter_stage_set_key_focus (CLUTTER_STAGE (clutter_actor_get_stage (actor)), actor);
}

static void
edit_text_end (ClutterActor *actor)
{
  g_object_set (actor, "editable", text_was_editable,
                       "reactive", text_was_reactive, NULL);
  cs_dirtied ();
}

void
cs_edit_text_init (void)
{
  cs_actor_editors_add (CLUTTER_TYPE_TEXT, edit_text_start, edit_text_end);
  cs_actor_editors_add (MX_TYPE_LABEL, edit_text_start, edit_text_end);
}

