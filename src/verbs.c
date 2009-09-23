#include <clutter/clutter.h>
#include <nbtk/nbtk.h>
#include "cluttersmith.h"
#include "util.h"

/****/

/**[ copy and paste ]*******************************************************/

static GList *clipboard = NULL;

static void each_duplicate (ClutterActor *actor)
{
  ClutterActor *parent, *new_actor;
  parent = cluttersmith_get_current_container ();
  new_actor = util_duplicator (actor, parent);
  {
    gfloat x, y;
    clutter_actor_get_position (new_actor, &x, &y);
    x+=10;y+=10;
    clutter_actor_set_position (new_actor, x, y);
  }
  cluttersmith_selected_clear ();
  cluttersmith_selected_add (new_actor);
}

void cb_duplicate_selected (ClutterActor *actor)
{
  cluttersmith_selected_foreach (G_CALLBACK (each_duplicate), NULL);
  cluttersmith_dirtied ();;
}

static void each_remove (ClutterActor *actor)
{
  ClutterActor *parent;
  parent = clutter_actor_get_parent (actor);
  clutter_actor_destroy (actor);
}

void cb_remove_selected (ClutterActor *actor)
{
  ClutterActor *active = cluttersmith_get_active ();
  if (active)
    {
      ClutterActor *parent = clutter_actor_get_parent (active);
      cluttersmith_set_active (NULL);
      cluttersmith_selected_foreach (G_CALLBACK (each_remove), NULL);
      cluttersmith_dirtied ();;
      cluttersmith_selected_clear ();
      cluttersmith_selected_add (parent);
    }
}

static void empty_clipboard (void)
{
  while (clipboard)
    {
      g_object_unref (clipboard->data);
      clipboard = g_list_remove (clipboard, clipboard->data);
    }
}

static void each_cut (ClutterActor *actor)
{
  ClutterActor *parent;
  parent = clutter_actor_get_parent (actor);
  g_object_ref (actor);
  clutter_container_remove_actor (CLUTTER_CONTAINER (parent), actor);
  clipboard = g_list_append (clipboard, actor);
  clutter_actor_destroy (actor);
}

void cb_cut_selected (ClutterActor *actor)
{
  ClutterActor *active = cluttersmith_get_active ();
  empty_clipboard ();
  if (active)
    {
      ClutterActor *parent = clutter_actor_get_parent (active);
      cluttersmith_set_active (NULL);
      cluttersmith_selected_foreach (G_CALLBACK (each_cut), NULL);
      cluttersmith_dirtied ();;
      cluttersmith_selected_clear ();
      cluttersmith_selected_add (parent);
    }

}

static void each_copy (ClutterActor *actor)
{
  ClutterActor *parent, *new_actor;
  parent = clutter_actor_get_parent (actor);

  new_actor = util_duplicator (actor, parent);
  g_object_ref (new_actor);
  clutter_container_remove_actor (CLUTTER_CONTAINER (parent), new_actor);
  clipboard = g_list_append (clipboard, new_actor);
}

void cb_copy_selected (ClutterActor *actor)
{
  empty_clipboard ();
  cluttersmith_selected_foreach (G_CALLBACK (each_copy), NULL);
  cluttersmith_dirtied ();;
}


void cb_paste_selected (ClutterActor *actor)
{
  if (clipboard)
    {
      GList *i;
      ClutterActor *new_actor = NULL, *parent;

      parent = cluttersmith_get_current_container ();

      for (i=clipboard; i;i=i->next)
        {
          new_actor = util_duplicator (i->data, parent);
          g_assert (new_actor);
          {
            gfloat x, y;
            clutter_actor_get_position (new_actor, &x, &y);
            clutter_actor_set_position (new_actor, x, y);
          }
        }
      cluttersmith_selected_clear ();
      if (new_actor)
        cluttersmith_selected_add (new_actor);
    }
  cluttersmith_dirtied ();;
}

void cb_raise_selected (ClutterActor *actor)
{
  cluttersmith_selected_foreach (G_CALLBACK (clutter_actor_raise), NULL);
  cluttersmith_dirtied ();;
}

void cb_lower_selected (ClutterActor *actor)
{
  cluttersmith_selected_foreach (G_CALLBACK (clutter_actor_lower), NULL);
  cluttersmith_dirtied ();;
}


void cb_raise_top_selected (ClutterActor *actor)
{
  cluttersmith_selected_foreach (G_CALLBACK (clutter_actor_raise_top), NULL);
  cluttersmith_dirtied ();;
}

void cb_lower_bottom_selected (ClutterActor *actor)
{
  cluttersmith_selected_foreach (G_CALLBACK (clutter_actor_lower_bottom), NULL);
  cluttersmith_dirtied ();;
}

static void each_reset_size (ClutterActor *actor)
{
  clutter_actor_set_size (actor, -1, -1);
}

void cb_reset_size (ClutterActor *actor)
{
  cluttersmith_selected_foreach (G_CALLBACK (each_reset_size), NULL);
  cluttersmith_dirtied ();;
}

void cb_quit (ClutterActor *actor)
{
  clutter_main_quit ();
}

void cb_focus_entry (ClutterActor *actor)
{
  ClutterActor *entry;
  entry = nbtk_entry_get_clutter_text (NBTK_ENTRY (util_find_by_id_int (actor, "title")));
  g_assert (entry);
  if (entry)
    {
      g_print ("trying to set key focus\n");
      clutter_stage_set_key_focus (CLUTTER_STAGE (clutter_actor_get_stage (entry)), entry);
    }
}

void cb_select_none (ClutterActor *actor)
{
  cluttersmith_selected_clear ();
}

void cb_select_all (ClutterActor *actor)
{
  GList *l, *list;
  cluttersmith_selected_clear ();
  list = clutter_container_get_children (CLUTTER_CONTAINER (cluttersmith_get_current_container()));
  for (l=list; l;l=l->next)
    {
      cluttersmith_selected_add (l->data);
    }
  g_list_free (list);
}


static gfloat min_x=0;
static gfloat min_y=0;

static void each_group (ClutterActor *actor,
                        gpointer      new_parent)
{
  ClutterActor *parent;
  gfloat x, y;
  parent = clutter_actor_get_parent (actor);
  clutter_actor_get_position (actor, &x, &y);
  g_object_ref (actor);
  if (x < min_x)
    min_x = x;
  if (y < min_y)
    min_y = y;
  clutter_container_remove_actor (CLUTTER_CONTAINER (parent), actor);
  clutter_container_add_actor (CLUTTER_CONTAINER (new_parent), actor);
  g_object_unref (actor);
}

static void each_group_move (ClutterActor *actor,
                             gfloat       *delta)
{
  gfloat x, y;
  clutter_actor_get_position (actor, &x, &y);
  clutter_actor_set_position (actor, x-delta[0], y-delta[1]);
}

void cb_group (ClutterActor *actor)
{
  gfloat delta[2];
  ClutterActor *parent;
  ClutterActor *group;
  parent = cluttersmith_get_current_container ();
  group = clutter_group_new ();
  /* find upper left item,. and place group there,.. reposition children to
   * fit this
   */
  clutter_container_add_actor (CLUTTER_CONTAINER (parent), group);
  /* get add_parent */
  /* create group */

  min_x = 2000000.0;
  min_y = 2000000.0;

  cluttersmith_selected_foreach (G_CALLBACK (each_group), group);
  delta[0]=min_x;
  delta[1]=min_y;
  cluttersmith_selected_foreach (G_CALLBACK (each_group_move), delta);
  clutter_actor_set_position (group, min_x, min_y);
  cluttersmith_selected_clear ();
  cluttersmith_selected_add (group);
  cluttersmith_dirtied ();;
}

static void each_ungroup (ClutterActor *actor,
                          GList       **created_list)
{
  ClutterActor *parent;
  parent = clutter_actor_get_parent (actor);
  if (CLUTTER_IS_CONTAINER (actor))
    {
      GList *c, *children;
      gfloat cx, cy;
      children = clutter_container_get_children (CLUTTER_CONTAINER (actor));
      clutter_actor_get_position (actor, &cx, &cy);
      for (c = children; c; c = c->next)
        {
          gfloat x, y;
          ClutterActor *child = c->data;
          g_object_ref (child);
          clutter_actor_get_position (child, &x, &y);
          clutter_actor_set_position (child, cx + x, cy + y);
          clutter_container_remove_actor (CLUTTER_CONTAINER (actor), child);
          clutter_container_add_actor (CLUTTER_CONTAINER (parent), child);
          g_object_unref (child);
          *created_list = g_list_append (*created_list, child);
        }
      g_list_free (children);
      clutter_actor_destroy (actor);
    }
}

void cb_ungroup (ClutterActor *actor)
{
  GList *i, *created_list = NULL;
  cluttersmith_set_active (NULL);
  cluttersmith_selected_foreach (G_CALLBACK (each_ungroup), &created_list);
  cluttersmith_selected_clear (); 
  for (i=created_list; i; i=i->next)
    {
      cluttersmith_selected_add (i->data);
    }
  g_list_free (created_list);
  cluttersmith_dirtied ();;
}

void cb_select_parent (ClutterActor *actor)
{
  ClutterActor *active_actor = cluttersmith_selected_get_any ();
  if (active_actor)
    {
      ClutterActor *parent = clutter_actor_get_parent (active_actor);
      if (parent) 
        {
          cluttersmith_selected_clear ();
          cluttersmith_selected_add (parent);
          parent = clutter_actor_get_parent (parent);
          cluttersmith_set_current_container (parent);
          clutter_actor_queue_redraw (active_actor);
        }
    }
}

void
cb_help (ClutterActor *actor)
{
  cluttersmith_set_project_root (PKGDATADIR "docs");
}

void
cb_ui_mode (ClutterActor *actor)
{
  switch (cluttersmith_get_ui_mode ())
    {
        case CLUTTERSMITH_UI_MODE_BROWSE:
          cluttersmith_set_ui_mode (CLUTTERSMITH_UI_MODE_EDIT);
          g_print ("Run mode : edit only\n");
          break;
        case CLUTTERSMITH_UI_MODE_EDIT:
          cluttersmith_set_ui_mode (CLUTTERSMITH_UI_MODE_CHROME);
          g_print ("Run mode : ui with edit\n");
          break;
        case CLUTTERSMITH_UI_MODE_CHROME: 
        default:
          cluttersmith_set_ui_mode (CLUTTERSMITH_UI_MODE_BROWSE);
          g_print ("Run mode : browse\n");
          break;
    }
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
  {CLUTTER_CONTROL_MASK, CLUTTER_l,         cb_focus_entry},

  /* check for the more specific modifier state before the more generic ones */
  {CLUTTER_CONTROL_MASK|
   CLUTTER_SHIFT_MASK,   CLUTTER_a,         cb_select_none},
  {CLUTTER_CONTROL_MASK, CLUTTER_a,         cb_select_all},

  /* check for the more specific modifier state before the more generic ones */
  {CLUTTER_CONTROL_MASK|
   CLUTTER_SHIFT_MASK,   CLUTTER_g,         cb_ungroup},
  {CLUTTER_CONTROL_MASK, CLUTTER_g,         cb_group},
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


static KeyBinding global_keybindings[]={
  {CLUTTER_CONTROL_MASK, CLUTTER_q,           cb_quit},
  {0,                    CLUTTER_Scroll_Lock, cb_ui_mode},
  {0,                    CLUTTER_F1,          cb_help},
  {0, 0, NULL},
};


gboolean manipulator_key_pressed_global (ClutterActor *stage, ClutterModifierType modifier, guint key)
{
  gint i;
  for (i=0; global_keybindings[i].key_symbol; i++)
    {
      if (global_keybindings[i].key_symbol == key &&
          ((global_keybindings[i].modifier & modifier) == global_keybindings[i].modifier))
        {
          global_keybindings[i].callback (stage);
          return TRUE;
        }
    }
  return FALSE;
}



