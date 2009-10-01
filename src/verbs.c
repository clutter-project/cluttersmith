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
  parent = cs_get_current_container ();
  new_actor = cs_duplicator (actor, parent);
  {
    gfloat x, y;
    clutter_actor_get_position (new_actor, &x, &y);
    x+=10;y+=10;
    clutter_actor_set_position (new_actor, x, y);
  }
  cs_selected_clear ();
  cs_selected_add (new_actor);
}

void cb_duplicate_selected (ClutterActor *actor)
{
  cs_selected_foreach (G_CALLBACK (each_duplicate), NULL);
  cs_dirtied ();;
}

static void each_remove (ClutterActor *actor)
{
  ClutterActor *parent;
  parent = clutter_actor_get_parent (actor);
  clutter_actor_destroy (actor);
}

void cb_remove_selected (ClutterActor *actor)
{
  ClutterActor *active = cs_get_active ();
  if (active)
    {
      ClutterActor *parent = clutter_actor_get_parent (active);
      cs_set_active (NULL);
      cs_selected_foreach (G_CALLBACK (each_remove), NULL);
      cs_dirtied ();;
      cs_selected_clear ();
      cs_selected_add (parent);
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
  ClutterActor *active = cs_get_active ();
  empty_clipboard ();
  if (active)
    {
      ClutterActor *parent = clutter_actor_get_parent (active);
      cs_set_active (NULL);
      cs_selected_foreach (G_CALLBACK (each_cut), NULL);
      cs_dirtied ();;
      cs_selected_clear ();
      cs_selected_add (parent);
    }

}

static void each_copy (ClutterActor *actor)
{
  ClutterActor *parent, *new_actor;
  parent = clutter_actor_get_parent (actor);

  new_actor = cs_duplicator (actor, parent);
  g_object_ref (new_actor);
  clutter_container_remove_actor (CLUTTER_CONTAINER (parent), new_actor);
  clipboard = g_list_append (clipboard, new_actor);
}

void cb_copy_selected (ClutterActor *actor)
{
  empty_clipboard ();
  cs_selected_foreach (G_CALLBACK (each_copy), NULL);
  cs_dirtied ();;
}


void cb_paste_selected (ClutterActor *actor)
{
  if (clipboard)
    {
      GList *i;
      ClutterActor *new_actor = NULL, *parent;

      parent = cs_get_current_container ();

      for (i=clipboard; i;i=i->next)
        {
          new_actor = cs_duplicator (i->data, parent);
          g_assert (new_actor);
          {
            gfloat x, y;
            clutter_actor_get_position (new_actor, &x, &y);
            clutter_actor_set_position (new_actor, x, y);
          }
        }
      cs_selected_clear ();
      if (new_actor)
        cs_selected_add (new_actor);
    }
  cs_dirtied ();;
}

void cb_raise_selected (ClutterActor *actor)
{
  cs_selected_foreach (G_CALLBACK (clutter_actor_raise), NULL);
  cs_dirtied ();;
}

void cb_lower_selected (ClutterActor *actor)
{
  cs_selected_foreach (G_CALLBACK (clutter_actor_lower), NULL);
  cs_dirtied ();;
}


void cb_raise_top_selected (ClutterActor *actor)
{
  cs_selected_foreach (G_CALLBACK (clutter_actor_raise_top), NULL);
  cs_dirtied ();;
}

void cb_lower_bottom_selected (ClutterActor *actor)
{
  cs_selected_foreach (G_CALLBACK (clutter_actor_lower_bottom), NULL);
  cs_dirtied ();;
}

static void each_reset_size (ClutterActor *actor)
{
  clutter_actor_set_size (actor, -1, -1);
}

void cb_reset_size (ClutterActor *actor)
{
  cs_selected_foreach (G_CALLBACK (each_reset_size), NULL);
  cs_dirtied ();;
}

void cb_quit (ClutterActor *actor)
{
  clutter_main_quit ();
}

void cb_focus_entry (ClutterActor *actor)
{
  ClutterActor *entry;
  entry = nbtk_entry_get_clutter_text (NBTK_ENTRY (cs_find_by_id_int (actor, "title")));
  g_assert (entry);
  if (entry)
    {
      g_print ("trying to set key focus\n");
      clutter_stage_set_key_focus (CLUTTER_STAGE (clutter_actor_get_stage (entry)), entry);
    }
}

void cb_select_none (ClutterActor *actor)
{
  cs_selected_clear ();
}

void cb_select_all (ClutterActor *actor)
{
  GList *l, *list;
  cs_selected_clear ();
  list = clutter_container_get_children (CLUTTER_CONTAINER (cs_get_current_container()));
  for (l=list; l;l=l->next)
    {
      cs_selected_add (l->data);
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
  parent = cs_get_current_container ();
  group = clutter_group_new ();
  /* find upper left item,. and place group there,.. reposition children to
   * fit this
   */
  clutter_container_add_actor (CLUTTER_CONTAINER (parent), group);
  /* get add_parent */
  /* create group */

  min_x = 2000000.0;
  min_y = 2000000.0;

  cs_selected_foreach (G_CALLBACK (each_group), group);
  delta[0]=min_x;
  delta[1]=min_y;
  cs_selected_foreach (G_CALLBACK (each_group_move), delta);
  clutter_actor_set_position (group, min_x, min_y);
  cs_selected_clear ();
  cs_selected_add (group);
  cs_dirtied ();;
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
  cs_set_active (NULL);
  cs_selected_foreach (G_CALLBACK (each_ungroup), &created_list);
  cs_selected_clear (); 
  for (i=created_list; i; i=i->next)
    {
      cs_selected_add (i->data);
    }
  g_list_free (created_list);
  cs_dirtied ();;
}

void cb_select_parent (ClutterActor *actor)
{
  ClutterActor *active_actor = cs_selected_get_any ();
  if (active_actor)
    {
      ClutterActor *parent = clutter_actor_get_parent (active_actor);
      if (parent) 
        {
          cs_selected_clear ();
          cs_selected_add (parent);
          parent = clutter_actor_get_parent (parent);
          cs_set_current_container (parent);
          clutter_actor_queue_redraw (active_actor);
        }
    }
}

void
cb_help (ClutterActor *actor)
{
  cs_set_project_root (PKGDATADIR "docs");
}

void
cb_ui_mode (ClutterActor *actor)
{
  switch (cs_get_ui_mode ())
    {
        case CS_UI_MODE_BROWSE:
          cs_set_ui_mode (CS_UI_MODE_CHROME);
          g_print ("Run mode : edit only\n");
          break;
/*        case CS_UI_MODE_EDIT:
          cs_set_ui_mode (CS_UI_MODE_CHROME);
          g_print ("Run mode : ui with edit\n");
          break;*/
        case CS_UI_MODE_CHROME: 
        default:
          cs_set_ui_mode (CS_UI_MODE_BROWSE);
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



static gboolean delayed_destroy (gpointer actor)
{
  clutter_actor_destroy (actor);
  return FALSE;
}

static void popup_menu_hidden (gpointer popup)
{
  g_idle_add (delayed_destroy, popup);
}

static void popup_action_activated (gpointer a, gpointer b)
{
  clutter_actor_hide (a);
}

static NbtkPopup *cs_popup_new (void)
{
  NbtkPopup *popup;
  popup = NBTK_POPUP (nbtk_popup_new ());
  g_signal_connect (popup, "action-activated", G_CALLBACK (popup_action_activated), NULL);
  g_signal_connect (popup, "hide", G_CALLBACK (popup_menu_hidden), NULL);
  return popup;
}


static void foo (void)
{
  g_print ("hoi\n");
}

static void dialog_toggle (gpointer action,
                           ClutterActor *dialog)
{
  if (clutter_actor_get_paint_visibility (dialog))
    {
      clutter_actor_hide (dialog);
    }
  else
    {
      clutter_actor_show(dialog);
    }
  clutter_actor_queue_relayout (dialog);
  clutter_actor_queue_relayout (clutter_actor_get_parent (dialog));
  cs_sync_chrome ();
}

static NbtkAction *dialog_toggle_action (const gchar *name,
                                         ClutterActor *actor)
{
  NbtkAction *action;
  gchar *label;
  if (actor)
    label = g_strdup_printf ("%s %s",
            clutter_actor_get_paint_visibility (actor)?"hide":"show",
            name);
  else
    label = g_strdup (name);
  action = nbtk_action_new_full (label, G_CALLBACK (dialog_toggle), actor);
  g_free (label);
  return action;
}

static void cb_save_dialog_state (gpointer ignored)
{
  cs_save_dialog_state ();
}

void dialogs_popup (gint x,
                    gint y)
{
  NbtkPopup *popup = cs_popup_new ();
  NbtkAction *action;
  x = cs_last_x;
  y = cs_last_y;
  nbtk_popup_add_action (popup, nbtk_action_new_full ("Hide All",  foo,  NULL));
  nbtk_popup_add_action (popup, nbtk_action_new_full ("Show All",  foo,  NULL));

  action = dialog_toggle_action ("Toolbar", cluttersmith->dialog_toolbar);
  nbtk_popup_add_action (popup, action);

  action = dialog_toggle_action ("Tree", cluttersmith->dialog_tree);
  nbtk_popup_add_action (popup, action);

  action = dialog_toggle_action ("Property Editor", cluttersmith->dialog_property_inspector);
  nbtk_popup_add_action (popup, action);

  action = dialog_toggle_action ("Templates", cluttersmith->dialog_templates);
  nbtk_popup_add_action (popup, action);

  action = dialog_toggle_action ("Scenes", cluttersmith->dialog_scenes);
  nbtk_popup_add_action (popup, action);

  action = dialog_toggle_action ("Config", cluttersmith->dialog_config);
  nbtk_popup_add_action (popup, action);

  nbtk_popup_add_action (popup, nbtk_action_new_full ("", NULL, NULL));
  nbtk_popup_add_action (popup, nbtk_action_new_full ("Remember positions and visibility", G_CALLBACK (cb_save_dialog_state), NULL));
  clutter_group_add (cluttersmith->parasite_root, popup);
  clutter_actor_set_position (CLUTTER_ACTOR (popup), x, y);
  clutter_actor_show (CLUTTER_ACTOR (popup));
}

void playback_popup (gint x,
                     gint y)
{
  NbtkPopup *popup = cs_popup_new ();
  NbtkAction *action;

  action = nbtk_action_new_full ("Edit", G_CALLBACK (cb_ui_mode),  NULL);
  nbtk_popup_add_action (popup, action);
  action = nbtk_action_new_full ("Quit", G_CALLBACK (cb_quit),  NULL);
  nbtk_popup_add_action (popup, action);
  clutter_group_add (cluttersmith->parasite_root, popup);
  clutter_actor_set_position (CLUTTER_ACTOR (popup), x, y);
  clutter_actor_show (CLUTTER_ACTOR (popup));
}

void root_popup (gint x,
                 gint y)
{
  NbtkPopup *popup = cs_popup_new ();
  NbtkAction *action;

  action = nbtk_action_new_full ("Browse", G_CALLBACK (cb_ui_mode),  NULL);
  nbtk_popup_add_action (popup, action);
  action = nbtk_action_new_full ("Dialogs", G_CALLBACK (dialogs_popup),  NULL);
  nbtk_popup_add_action (popup, action);
  action = nbtk_action_new_full ("Quit", G_CALLBACK (cb_quit),  NULL);
  nbtk_popup_add_action (popup, action);

  clutter_group_add (cluttersmith->parasite_root, popup);
  clutter_actor_set_position (CLUTTER_ACTOR (popup), x, y);
  clutter_actor_show (CLUTTER_ACTOR (popup));
}


void object_popup (ClutterActor *actor,
                   gint          x,
                   gint          y)
{
  NbtkPopup *popup = cs_popup_new ();
  nbtk_popup_add_action (popup, nbtk_action_new_full ("Create link",  foo,  NULL));
  nbtk_popup_add_action (popup, nbtk_action_new_full ("Follow link",  foo,  NULL));

  nbtk_popup_add_action (popup, nbtk_action_new_full ("Cut", NULL, NULL));
  nbtk_popup_add_action (popup, nbtk_action_new_full ("Copy", NULL, NULL));
  nbtk_popup_add_action (popup, nbtk_action_new_full ("Paste", NULL, NULL));
  nbtk_popup_add_action (popup, nbtk_action_new_full ("Duplicate", NULL, NULL));
  nbtk_popup_add_action (popup, nbtk_action_new_full ("Remove", NULL, NULL));


  clutter_group_add (cluttersmith->parasite_root, popup);
  clutter_actor_set_position (CLUTTER_ACTOR (popup), x, y);
  clutter_actor_show (CLUTTER_ACTOR (popup));
}


void selection_popup (gint x,
                      gint y)
{
  NbtkPopup *popup = cs_popup_new ();
  nbtk_popup_add_action (popup, nbtk_action_new_full ("Selection",  foo,  NULL));
  nbtk_popup_add_action (popup, nbtk_action_new_full ("Cut", NULL, NULL));
  nbtk_popup_add_action (popup, nbtk_action_new_full ("Copy", NULL, NULL));
  nbtk_popup_add_action (popup, nbtk_action_new_full ("Paste", NULL, NULL));
  nbtk_popup_add_action (popup, nbtk_action_new_full ("Duplicate", NULL, NULL));
  nbtk_popup_add_action (popup, nbtk_action_new_full ("Remove", NULL, NULL));
  clutter_group_add (cluttersmith->parasite_root, popup);
  clutter_actor_set_position (CLUTTER_ACTOR (popup), x, y);
  clutter_actor_show (CLUTTER_ACTOR (popup));
}
