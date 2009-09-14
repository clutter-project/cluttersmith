#include <clutter/clutter.h>
#include "cluttersmith.h"
#include "util.h"

/****/

/**[ copy and paste ]*******************************************************/

static ClutterActor *copy_buf = NULL; /* XXX: should be a GList */

void cb_duplicate_selected (ClutterActor *actor)
{
  if (active_actor)
    {
      ClutterActor *new_actor;
      ClutterActor *parent;
      
      parent = clutter_actor_get_parent (active_actor);
      new_actor = util_duplicator (active_actor, parent);
      {
        gfloat x, y;
        clutter_actor_get_position (new_actor, &x, &y);
        x+=10;y+=10;
        clutter_actor_set_position (new_actor, x, y);
      }
      select_item (new_actor);
    }
}


void cb_remove_selected (ClutterActor *actor)
{
  if (active_actor)
    {
      ClutterActor *old_selected = active_actor;
      if (active_actor == clutter_actor_get_stage (actor))
        return;
      select_item (clutter_actor_get_parent (active_actor));
      clutter_actor_destroy (old_selected);

      CB_REV++;
    }
}


void cb_cut_selected (ClutterActor *actor)
{
  if (active_actor)
    {
      ClutterActor *parent;

      parent = clutter_actor_get_parent (active_actor);
      g_object_ref (active_actor);
      clutter_container_remove_actor (CLUTTER_CONTAINER (parent), active_actor);
      if (copy_buf)
        g_object_unref (copy_buf);
      copy_buf = active_actor;
      select_item (parent);
    }
}

void cb_copy_selected (ClutterActor *actor)
{
  if (active_actor)
    {
      ClutterActor *new_actor, *parent;

      parent = clutter_actor_get_parent (active_actor);
      new_actor = util_duplicator (active_actor, parent);
      {
        gfloat x, y;
        clutter_actor_get_position (new_actor, &x, &y);
        x+=10;y+=10;
        clutter_actor_set_position (new_actor, x, y);
      }
      g_object_ref (new_actor);
      clutter_container_remove_actor (CLUTTER_CONTAINER (parent), new_actor);
      if (copy_buf)
        g_object_unref (copy_buf);
      copy_buf = new_actor;
    }
}

void cb_paste_selected (ClutterActor *actor)
{
  if (active_actor && copy_buf)
    {
      ClutterActor *new_actor, *parent;

      if (CLUTTER_IS_CONTAINER (active_actor))
        {
          parent = active_actor;
        }
      else
        {
          parent = clutter_actor_get_parent (active_actor);
        }
      new_actor = util_duplicator (copy_buf, parent);
      {
        gfloat x, y;
        clutter_actor_get_position (new_actor, &x, &y);
        x+=10;y+=10;
        clutter_actor_set_position (new_actor, x, y);
      }
      select_item (new_actor);
    }
}

void cb_raise_selected (ClutterActor *actor)
{
  if (active_actor)
    {
      clutter_actor_raise (active_actor, NULL);
      CB_REV++;
    }
}

void cb_lower_selected (ClutterActor *actor)
{
  if (active_actor)
    {
      clutter_actor_lower (active_actor, NULL);
      CB_REV++;
    }
}


void cb_raise_top_selected (ClutterActor *actor)
{
  if (active_actor)
    {
      clutter_actor_raise_top (active_actor);
      CB_REV++;
    }
}

void cb_lower_bottom_selected (ClutterActor *actor)
{
  if (active_actor)
    {
      clutter_actor_lower_bottom (active_actor);
      CB_REV++;
    }
}


void cb_reset_size (ClutterActor *actor)
{
  if (active_actor)
    {
      clutter_actor_set_size (active_actor, -1, -1);
      CB_REV++;
    }
}


void cb_quit (ClutterActor *actor)
{
  clutter_main_quit ();
}

void cb_focus_entry (ClutterActor *actor)
{
  ClutterActor *entry;
  entry = util_find_by_id (clutter_actor_get_stage (actor), "title");
  if (entry)
    clutter_stage_set_key_focus (CLUTTER_STAGE (clutter_actor_get_stage (actor)),
                                 entry);
}

void cb_select_none (ClutterActor *actor)
{
  g_print ("%s NYI\n", G_STRFUNC);
}

void cb_select_all (ClutterActor *actor)
{
  g_print ("%s NYI\n", G_STRFUNC);
}

void cb_group (ClutterActor *actor)
{
  g_print ("%s NYI\n", G_STRFUNC);
}

void cb_ungroup (ClutterActor *actor)
{
  g_print ("%s NYI\n", G_STRFUNC);
}

void cb_select_parent (ClutterActor *actor)
{
  if (active_actor)
    select_item (clutter_actor_get_parent (active_actor));
}

/******************************************************************************/

typedef struct KeyBinding {
  ClutterModifierType modifier;
  guint key_symbol;
  void (*callback) (ClutterActor *actor);
} KeyBinding;

static KeyBinding keybindings[]={
  {CLUTTER_CONTROL_MASK, CLUTTER_x,         cb_cut_selected},
  {CLUTTER_CONTROL_MASK, CLUTTER_c,         cb_copy_selected},
  {CLUTTER_CONTROL_MASK, CLUTTER_v,         cb_paste_selected},
  {CLUTTER_CONTROL_MASK, CLUTTER_d,         cb_duplicate_selected},
  {CLUTTER_CONTROL_MASK, CLUTTER_q,         cb_quit},
  {CLUTTER_CONTROL_MASK, CLUTTER_l,         cb_focus_entry},

  /* check for the more specific modifier state before the more generic ones */
  {CLUTTER_CONTROL_MASK|
   CLUTTER_SHIFT_MASK,   CLUTTER_a,         cb_select_none},
  {CLUTTER_CONTROL_MASK, CLUTTER_a,         cb_select_all},

  /* check for the more specific modifier state before the more generic ones */
  {CLUTTER_CONTROL_MASK|
   CLUTTER_SHIFT_MASK,   CLUTTER_g,         cb_group},
  {CLUTTER_CONTROL_MASK, CLUTTER_g,         cb_ungroup},
  {CLUTTER_CONTROL_MASK, CLUTTER_p,         cb_select_parent},

  {0,                    CLUTTER_BackSpace, cb_remove_selected},
  {0,                    CLUTTER_Delete,    cb_remove_selected},
  {0,                    CLUTTER_Page_Up,   cb_raise_selected},
  {0,                    CLUTTER_Page_Down, cb_lower_selected},
  {0,                    CLUTTER_Home,      cb_raise_top_selected},
  {0,                    CLUTTER_End,       cb_lower_bottom_selected},
  {0, 0, NULL},
};

gboolean manipulator_key_pressed (ClutterActor *stage, ClutterModifierType modifier, guint key)
{
  gint i;
  for (i=0; keybindings[i].key_symbol; i++)
    {
      if (keybindings[i].key_symbol == key &&
          ((keybindings[i].modifier & modifier) == keybindings[i].modifier))
        {
          keybindings[i].callback (stage);
          return TRUE;
        }
    }
  return FALSE;
}
