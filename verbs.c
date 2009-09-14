#include <clutter/clutter.h>
#include "clutterbug.h"
#include "util.h"

/****/

/**[ copy and paste ]*******************************************************/

static ClutterActor *copy_buf = NULL; /* XXX: should be a GList */

void cb_duplicate_selected (ClutterActor *actor)
{
  if (selected_actor)
    {
      ClutterActor *new_actor;
      ClutterActor *parent;
      
      parent = clutter_actor_get_parent (selected_actor);
      new_actor = util_duplicator (selected_actor, parent);
      {
        gfloat x, y;
        clutter_actor_get_position (new_actor, &x, &y);
        x+=10;y+=10;
        clutter_actor_set_position (new_actor, x, y);
      }
      select_item (NULL, new_actor);
    }
}


void cb_remove_selected (ClutterActor *actor)
{
  if (selected_actor)
    {
      ClutterActor *old_selected = selected_actor;
      if (selected_actor == clutter_actor_get_stage (actor))
        return;
      select_item (NULL, clutter_actor_get_parent (selected_actor));
      clutter_actor_destroy (old_selected);

      CB_REV++;
    }
}


void cb_cut_selected (ClutterActor *actor)
{
  if (selected_actor)
    {
      ClutterActor *parent;

      parent = clutter_actor_get_parent (selected_actor);
      g_object_ref (selected_actor);
      clutter_container_remove_actor (CLUTTER_CONTAINER (parent), selected_actor);
      if (copy_buf)
        g_object_unref (copy_buf);
      copy_buf = selected_actor;
      select_item (NULL, parent);
    }
}

void cb_copy_selected (ClutterActor *actor)
{
  if (selected_actor)
    {
      ClutterActor *new_actor, *parent;

      parent = clutter_actor_get_parent (selected_actor);
      new_actor = util_duplicator (selected_actor, parent);
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
  if (selected_actor && copy_buf)
    {
      ClutterActor *new_actor, *parent;

      if (CLUTTER_IS_CONTAINER (selected_actor))
        {
          parent = selected_actor;
        }
      else
        {
          parent = clutter_actor_get_parent (selected_actor);
        }
      new_actor = util_duplicator (copy_buf, parent);
      {
        gfloat x, y;
        clutter_actor_get_position (new_actor, &x, &y);
        x+=10;y+=10;
        clutter_actor_set_position (new_actor, x, y);
      }
      select_item (NULL, new_actor);
    }
}

void cb_raise_selected (ClutterActor *actor)
{
  if (selected_actor)
    {
      clutter_actor_raise (selected_actor, NULL);
      CB_REV++;
    }
}

void cb_lower_selected (ClutterActor *actor)
{
  if (selected_actor)
    {
      clutter_actor_lower (selected_actor, NULL);
      CB_REV++;
    }
}


void cb_raise_top_selected (ClutterActor *actor)
{
  if (selected_actor)
    {
      clutter_actor_raise_top (selected_actor);
      CB_REV++;
    }
}

void cb_lower_bottom_selected (ClutterActor *actor)
{
  if (selected_actor)
    {
      clutter_actor_lower_bottom (selected_actor);
      CB_REV++;
    }
}


void cb_reset_size (ClutterActor *actor)
{
  if (selected_actor)
    {
      clutter_actor_set_size (selected_actor, -1, -1);
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
  {0,                    CLUTTER_BackSpace, cb_remove_selected},
  {0,                    CLUTTER_Delete,    cb_remove_selected},
  {0,                    CLUTTER_Page_Up,   cb_raise_selected},
  {0,                    CLUTTER_Page_Down, cb_lower_selected},
  {0,                    CLUTTER_Home,      cb_raise_top_selected},
  {0,                    CLUTTER_End,       cb_lower_bottom_selected},
  {0, 0, NULL},
};

gboolean manipulator_key_pressed (ClutterActor *stage, guint key)
{
  gint i;
  for (i=0; keybindings[i].key_symbol; i++)
    {
      if (keybindings[i].key_symbol == key)
        {
          keybindings[i].callback (stage);
          return TRUE;
        }
    }
  return FALSE;
}
