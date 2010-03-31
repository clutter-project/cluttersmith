#include <string.h>
#include <clutter/clutter.h>
#include <mx/mx.h>
#include "cluttersmith.h"

/****/

/**[ copy and paste ]*******************************************************/

static GList *clipboard = NULL;


static void each_duplicate (ClutterActor *actor,
                            GList        **new)
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
  *new = g_list_append (*new, new_actor);
}

void cs_duplicate (ClutterActor *ignored)
{
  GList *n, *new = NULL;
  cs_selected_foreach (G_CALLBACK (each_duplicate), &new);
  cs_selected_clear ();
  for (n=new; n; n=n->next)
    cs_selected_add (n->data);
  g_list_free (new);
  cs_dirtied ();
}

static void each_remove (ClutterActor *actor)
{
  ClutterActor *parent;
  parent = clutter_actor_get_parent (actor);
  clutter_actor_destroy (actor);
}

void cs_edit (ClutterActor *ignored)
{
  ClutterActor *active = cs_get_active ();
  if (active)
    {
      cs_edit_actor_start (active);
    }
}


void cs_remove (ClutterActor *ignored)
{
  ClutterActor *active = cs_get_active ();
  if (active)
    {
      ClutterActor *parent = clutter_actor_get_parent (active);

      if (active == cluttersmith->fake_stage)
        return;

      cs_set_active (NULL);
      cs_selected_foreach (G_CALLBACK (each_remove), NULL);
      cs_dirtied ();
      cs_selected_clear ();
      {
        GList *children;
        children = clutter_container_get_children (CLUTTER_CONTAINER (parent));
        if (children)
          {
            cs_selected_add (children->data);
            g_list_free (children);
          }
        else
          {
            cs_selected_add (parent);
          }
      }
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

void cs_cut (ClutterActor *ignored)
{
  ClutterActor *active = cs_get_active ();
  empty_clipboard ();
  if (active)
    {
      ClutterActor *parent = clutter_actor_get_parent (active);
      cs_set_active (NULL);
      cs_selected_foreach (G_CALLBACK (each_cut), NULL);
      cs_dirtied ();
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

void cs_copy (ClutterActor *ignored)
{
  empty_clipboard ();
  cs_selected_foreach (G_CALLBACK (each_copy), NULL);
  cs_dirtied ();
}


void cs_paste (ClutterActor *ignored)
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
        {
          cs_selected_add (new_actor);
        }
    }
  cs_dirtied ();
}

void cs_raise (ClutterActor *ignored)
{
  cs_selected_foreach (G_CALLBACK (clutter_actor_raise), NULL);
  cs_dirtied ();
}

void cs_lower (ClutterActor *ignored)
{
  cs_selected_foreach (G_CALLBACK (clutter_actor_lower), NULL);
  cs_dirtied ();
}


void cs_raise_top (ClutterActor *ignored)
{
  cs_selected_foreach (G_CALLBACK (clutter_actor_raise_top), NULL);
  cs_dirtied ();
}

void cs_lower_bottom (ClutterActor *ignored)
{
  cs_selected_foreach (G_CALLBACK (clutter_actor_lower_bottom), NULL);
  cs_dirtied ();
}

static void each_reset_size (ClutterActor *actor)
{
  clutter_actor_set_size (actor, -1, -1);
}

void cs_reset_size (ClutterActor *ignored)
{
  cs_selected_foreach (G_CALLBACK (each_reset_size), NULL);
  cs_dirtied ();
}

void cs_quit (ClutterActor *ignored)
{
  clutter_main_quit ();
}

void cs_focus_title (ClutterActor *ignored)
{
  ClutterActor *entry;
  entry = mx_entry_get_clutter_text (MX_ENTRY (cs_find_by_id_int (cluttersmith->parasite_root, "title")));
  g_assert (entry);
  if (entry)
    {
      clutter_stage_set_key_focus (CLUTTER_STAGE (clutter_actor_get_stage (entry)), entry);

      clutter_actor_show(cluttersmith->dialog_toolbar);
      clutter_actor_queue_relayout (cluttersmith->dialog_toolbar);
      clutter_actor_queue_relayout (clutter_actor_get_parent (cluttersmith->dialog_toolbar));
      cs_sync_chrome ();
      clutter_actor_queue_redraw (cluttersmith->dialog_toolbar);
    }
}

void cs_select_none (ClutterActor *ignored)
{
  cs_selected_clear ();
}

void cs_select_all (ClutterActor *ignored)
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

void cs_view_reset (ClutterActor *ignored)
{
  g_object_set (cluttersmith, "zoom", 100.0, "origin-x", 0.0, "origin-y", 0.0, NULL);
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

ClutterActor *cs_group (ClutterActor *ignored)
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
  cs_dirtied ();
  return group;
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

void cs_ungroup (ClutterActor *ignored)
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
  cs_dirtied ();
}

void cs_make_group_box (ClutterActor *ignored)
{
  ClutterActor *active_actor = cs_selected_get_any ();
  gboolean vertical = TRUE;
  if (!active_actor)
    return;
  
  {
    GList *children, *c;
    gint minx=9000, miny=9000, maxx=-9000, maxy=-90000;
    children = clutter_container_get_children (CLUTTER_CONTAINER (active_actor));
    for (c=children; c; c=c->next)
      {
        gfloat x, y;
        clutter_actor_get_position (c->data, &x, &y);
        if (x < minx)
          minx = x;
        if (y < miny)
          miny = y;
        if (x > maxx)
          maxx = x;
        if (y > maxy)
          maxy = y;
      }
    if (maxx-minx > maxy - miny)
      vertical = FALSE;
  }

  active_actor = cs_actor_change_type (active_actor, "MxBoxLayout");
  g_object_set (G_OBJECT (active_actor), "vertical", vertical, NULL);
  cs_selected_clear (); 
  cs_selected_add (active_actor);
  cs_dirtied ();
}

void cs_make_box (ClutterActor *ignored)
{
  cs_group (ignored);
  cs_make_group_box (ignored);
}

void cs_make_group (ClutterActor *ignored)
{
  ClutterActor *active_actor = cs_selected_get_any ();
  if (!active_actor)
    return;
  
  active_actor = cs_actor_change_type (active_actor, "ClutterGroup");
  cs_selected_clear (); 
  cs_selected_add (active_actor);
  cs_dirtied ();
}

void cs_select_parent (ClutterActor *ignored)
{
  ClutterActor *active_actor = cs_selected_get_any ();
  if (active_actor)
    {
      ClutterActor *parent = clutter_actor_get_parent (active_actor);
      if (parent)
        {
          ClutterActor *gparent;
          gparent = clutter_actor_get_parent (parent);
          if (parent != cluttersmith->fake_stage)
            {
              cs_selected_clear ();
              cs_selected_add (parent);
              cs_set_current_container (gparent);
              clutter_actor_queue_redraw (active_actor);
            }
        }
    }
  else if (cs_get_current_container () != cluttersmith->fake_stage)
    {
      ClutterActor *parent = cs_get_current_container ();
      if (parent && parent != cluttersmith->fake_stage )
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
cs_help (ClutterActor *ignored)
{
  cluttersmith_set_project_root (PKGDATADIR "docs");
}


static void select_nearest (gboolean vertical,
                            gboolean reverse)
{
  ClutterActor *actor = cs_selected_get_any ();
  if (actor)
    {
      ClutterActor *new = cs_find_nearest (actor, vertical, reverse);
      if (new)
        {
          cs_selected_clear ();
          cs_selected_add (new);
        }
    }
  else
    {
      GList *children = clutter_container_get_children (
                            CLUTTER_CONTAINER (cs_get_current_container ()));
      if (children)
        {
          cs_selected_clear ();
          cs_selected_add (children->data);
          g_list_free (children);
        }
    }
}

void
cs_keynav_left (ClutterActor *ignored)
{
  select_nearest (FALSE, TRUE);
}

void
cs_keynav_right (ClutterActor *ignored)
{
  select_nearest (FALSE, FALSE);
}

void
cs_keynav_up (ClutterActor *ignored)
{
  select_nearest (TRUE, TRUE);
}

void
cs_keynav_down (ClutterActor *ignored)
{
  select_nearest (TRUE, FALSE);
}




void
cs_ui_mode (ClutterActor *ignored)
{
  switch (cs_get_ui_mode ())
    {
        case CS_UI_MODE_BROWSE:
          cs_set_ui_mode (CS_UI_MODE_CHROME);
          break;
/*        case CS_UI_MODE_EDIT:
          cs_set_ui_mode (CS_UI_MODE_CHROME);
          g_print ("Run mode : ui with edit\n");
          break;*/
        case CS_UI_MODE_CHROME: 
        default:
          cs_set_ui_mode (CS_UI_MODE_BROWSE);
          break;
    }
}

static void update_annotation_opacity (gint link_opacity, gint callout_opacity)
{
  GList *c, *children;
  children = cs_container_get_children_recursive (CLUTTER_CONTAINER (cluttersmith->fake_stage));
  for (c = children; c; c = c->next)
    {
      if (MX_IS_STYLABLE (c->data))
        {
          const gchar *sc = mx_stylable_get_style_class (c->data);
          if (sc && g_str_equal (sc, "ClutterSmithLink"))
            {
              clutter_actor_animate (c->data, CLUTTER_LINEAR, 800, "opacity", link_opacity, NULL);
              if (link_opacity == 0)
                clutter_actor_hide (c->data);
              else
                clutter_actor_show (c->data);
            }
          else if (sc && g_str_equal (sc, "ClutterSmithCallout"))
            {
              if (callout_opacity == 0)
                clutter_actor_hide (c->data);
              else
                clutter_actor_show (c->data);
              clutter_actor_animate (c->data, CLUTTER_LINEAR, 800, "opacity", callout_opacity, NULL);
            }
        }
    }
}

void mode_browse (ClutterActor *ignored)
{
  cs_set_ui_mode (CS_UI_MODE_BROWSE);
  cs_sync_chrome ();
  update_annotation_opacity (0x44, 0x00);
}

void mode_annotate (ClutterActor *ignored)
{
  g_object_set (cluttersmith->dialog_templates, "expanded", FALSE, NULL);
  g_object_set (cluttersmith->dialog_tree, "expanded", FALSE, NULL);
  g_object_set (cluttersmith->dialog_property_inspector, "expanded", FALSE, NULL);
  g_object_set (cluttersmith->dialog_states, "expanded", FALSE, NULL);
  g_object_set (cluttersmith->dialog_annotate, "expanded", TRUE, NULL);
  g_object_set (cluttersmith->dialog_callbacks, "expanded", FALSE, NULL);
  g_object_set (cluttersmith->dialog_config, "expanded", FALSE, NULL);
  g_object_set (cluttersmith->dialog_editor, "expanded", FALSE, NULL);
  g_object_set (cluttersmith->dialog_export, "expanded", FALSE, NULL);
  g_object_set (cluttersmith->dialog_project, "expanded", TRUE, NULL);
  g_object_set (cluttersmith->dialog_selected, "expanded", FALSE, NULL);

  cs_set_ui_mode (CS_UI_MODE_CHROME);
  cs_sync_chrome ();
  update_annotation_opacity (0xcc, 0xff);
}

void mode_sketch (ClutterActor *ignored)
{
  g_object_set (cluttersmith->dialog_templates, "expanded", TRUE, NULL);
  g_object_set (cluttersmith->dialog_tree, "expanded", FALSE, NULL);
  g_object_set (cluttersmith->dialog_property_inspector, "expanded", FALSE, NULL);
  g_object_set (cluttersmith->dialog_states, "expanded", FALSE, NULL);
  g_object_set (cluttersmith->dialog_annotate, "expanded", FALSE, NULL);
  g_object_set (cluttersmith->dialog_callbacks, "expanded", FALSE, NULL);
  g_object_set (cluttersmith->dialog_config, "expanded", FALSE, NULL);
  g_object_set (cluttersmith->dialog_editor, "expanded", FALSE, NULL);
  g_object_set (cluttersmith->dialog_export, "expanded", FALSE, NULL);
  g_object_set (cluttersmith->dialog_project, "expanded", FALSE, NULL);
  g_object_set (cluttersmith->dialog_selected, "expanded", FALSE, NULL);

  cs_set_ui_mode (CS_UI_MODE_CHROME);
  cs_sync_chrome ();
  update_annotation_opacity (0x66, 0x88);
}

void mode_edit (ClutterActor *ignored)
{

  g_object_set (cluttersmith->dialog_templates, "expanded", FALSE, NULL);
  g_object_set (cluttersmith->dialog_tree, "expanded", TRUE, NULL);
  g_object_set (cluttersmith->dialog_property_inspector, "expanded", TRUE, NULL);
  g_object_set (cluttersmith->dialog_states, "expanded", FALSE, NULL);
  g_object_set (cluttersmith->dialog_annotate, "expanded", FALSE, NULL);
  g_object_set (cluttersmith->dialog_callbacks, "expanded", FALSE, NULL);
  g_object_set (cluttersmith->dialog_config, "expanded", FALSE, NULL);
  g_object_set (cluttersmith->dialog_editor, "expanded", FALSE, NULL);
  g_object_set (cluttersmith->dialog_export, "expanded", FALSE, NULL);
  g_object_set (cluttersmith->dialog_project, "expanded", FALSE, NULL);
  g_object_set (cluttersmith->dialog_selected, "expanded", TRUE, NULL);

  cs_set_ui_mode (CS_UI_MODE_CHROME);
  cs_sync_chrome ();
  update_annotation_opacity (0x66, 0x55);
}

void mode_animate (ClutterActor *ignored)
{
  g_object_set (cluttersmith->dialog_templates, "expanded", FALSE, NULL);
  g_object_set (cluttersmith->dialog_tree, "expanded", FALSE, NULL);
  g_object_set (cluttersmith->dialog_property_inspector, "expanded", TRUE, NULL);
  g_object_set (cluttersmith->dialog_states, "expanded", TRUE, NULL);
  g_object_set (cluttersmith->dialog_annotate, "expanded", FALSE, NULL);
  g_object_set (cluttersmith->dialog_callbacks, "expanded", FALSE, NULL);
  g_object_set (cluttersmith->dialog_config, "expanded", FALSE, NULL);
  g_object_set (cluttersmith->dialog_editor, "expanded", FALSE, NULL);
  g_object_set (cluttersmith->dialog_export, "expanded", FALSE, NULL);
  g_object_set (cluttersmith->dialog_project, "expanded", FALSE, NULL);
  g_object_set (cluttersmith->dialog_selected, "expanded", TRUE, NULL);

  cs_set_ui_mode (CS_UI_MODE_CHROME);
  cs_sync_chrome ();
  update_annotation_opacity (0x66, 0x55);
}

void mode_callbacks (ClutterActor *ignored)
{
  g_object_set (cluttersmith->dialog_templates, "expanded", FALSE, NULL);
  g_object_set (cluttersmith->dialog_tree, "expanded", TRUE, NULL);
  g_object_set (cluttersmith->dialog_property_inspector, "expanded", FALSE, NULL);
  g_object_set (cluttersmith->dialog_states, "expanded", FALSE, NULL);
  g_object_set (cluttersmith->dialog_annotate, "expanded", FALSE, NULL);
  g_object_set (cluttersmith->dialog_callbacks, "expanded", TRUE, NULL);
  g_object_set (cluttersmith->dialog_config, "expanded", FALSE, NULL);
  g_object_set (cluttersmith->dialog_editor, "expanded", FALSE, NULL);
  g_object_set (cluttersmith->dialog_export, "expanded", FALSE, NULL);
  g_object_set (cluttersmith->dialog_project, "expanded", FALSE, NULL);
  g_object_set (cluttersmith->dialog_selected, "expanded", TRUE, NULL);

  cs_set_ui_mode (CS_UI_MODE_CHROME);
  cs_sync_chrome ();
  update_annotation_opacity (0x66, 0x55);
}

void mode_code (ClutterActor *ignored)
{
  g_object_set (cluttersmith->dialog_templates, "expanded", FALSE, NULL);
  g_object_set (cluttersmith->dialog_tree, "expanded", FALSE, NULL);
  g_object_set (cluttersmith->dialog_property_inspector, "expanded", FALSE, NULL);
  g_object_set (cluttersmith->dialog_states, "expanded", FALSE, NULL);
  g_object_set (cluttersmith->dialog_annotate, "expanded", FALSE, NULL);
  g_object_set (cluttersmith->dialog_callbacks, "expanded", FALSE, NULL);
  g_object_set (cluttersmith->dialog_config, "expanded", FALSE, NULL);
  g_object_set (cluttersmith->dialog_editor, "expanded", TRUE, NULL);
  g_object_set (cluttersmith->dialog_export, "expanded", FALSE, NULL);
  g_object_set (cluttersmith->dialog_project, "expanded", FALSE, NULL);
  g_object_set (cluttersmith->dialog_project, "expanded", FALSE, NULL);
  g_object_set (cluttersmith->dialog_selected, "expanded", FALSE, NULL);

  cs_set_ui_mode (CS_UI_MODE_CHROME);
  cs_sync_chrome ();

  update_annotation_opacity (0x66, 0x55);
}

void mode_export (ClutterActor *ignored)
{
  g_object_set (cluttersmith->dialog_templates, "expanded", FALSE, NULL);
  g_object_set (cluttersmith->dialog_tree, "expanded", FALSE, NULL);
  g_object_set (cluttersmith->dialog_property_inspector, "expanded", FALSE, NULL);
  g_object_set (cluttersmith->dialog_states, "expanded", FALSE, NULL);
  g_object_set (cluttersmith->dialog_annotate, "expanded", FALSE, NULL);
  g_object_set (cluttersmith->dialog_callbacks, "expanded", FALSE, NULL);
  g_object_set (cluttersmith->dialog_config, "expanded", FALSE, NULL);
  g_object_set (cluttersmith->dialog_editor, "expanded", FALSE, NULL);
  g_object_set (cluttersmith->dialog_export, "expanded", TRUE, NULL);
  g_object_set (cluttersmith->dialog_project, "expanded", FALSE, NULL);
  g_object_set (cluttersmith->dialog_selected, "expanded", FALSE, NULL);

  cs_set_ui_mode (CS_UI_MODE_CHROME);
  cs_sync_chrome ();
  update_annotation_opacity (0x0, 0x0);
}

void mode_config (ClutterActor *ignored)
{
  g_object_set (cluttersmith->dialog_templates, "expanded", FALSE, NULL);
  g_object_set (cluttersmith->dialog_tree, "expanded", FALSE, NULL);
  g_object_set (cluttersmith->dialog_property_inspector, "expanded", FALSE, NULL);
  g_object_set (cluttersmith->dialog_states, "expanded", FALSE, NULL);
  g_object_set (cluttersmith->dialog_annotate, "expanded", FALSE, NULL);
  g_object_set (cluttersmith->dialog_callbacks, "expanded", FALSE, NULL);
  g_object_set (cluttersmith->dialog_config, "expanded", TRUE, NULL);
  g_object_set (cluttersmith->dialog_editor, "expanded", FALSE, NULL);
  g_object_set (cluttersmith->dialog_export, "expanded", FALSE, NULL);
  g_object_set (cluttersmith->dialog_project, "expanded", TRUE, NULL);
  g_object_set (cluttersmith->dialog_selected, "expanded", FALSE, NULL);

  clutter_actor_show (cluttersmith->dialog_config);
  cs_set_ui_mode (CS_UI_MODE_CHROME);
  cs_sync_chrome ();
  update_annotation_opacity (0x0, 0x0);
}

void mode_browse2 (ClutterActor *ignored)
{ mx_combo_box_set_index (MX_COMBO_BOX (cluttersmith->cs_mode), 0); }
void mode_annotate2 (ClutterActor *ignored)
{ mx_combo_box_set_index (MX_COMBO_BOX (cluttersmith->cs_mode), 1); }
void mode_sketch2 (ClutterActor *ignored)
{ mx_combo_box_set_index (MX_COMBO_BOX (cluttersmith->cs_mode), 2); }
void mode_edit2 (ClutterActor *ignored)
{ mx_combo_box_set_index (MX_COMBO_BOX (cluttersmith->cs_mode), 3); }
void mode_animate2 (ClutterActor *ignored)
{ mx_combo_box_set_index (MX_COMBO_BOX (cluttersmith->cs_mode), 4); }
void mode_callbacks2 (ClutterActor *ignored)
{ mx_combo_box_set_index (MX_COMBO_BOX (cluttersmith->cs_mode), 5); }
void mode_code2 (ClutterActor *ignored)
{ mx_combo_box_set_index (MX_COMBO_BOX (cluttersmith->cs_mode), 6); }
void mode_export2(ClutterActor *ignored)
{ mx_combo_box_set_index (MX_COMBO_BOX (cluttersmith->cs_mode), 7); }
void mode_config2 (ClutterActor *ignored)
{ mx_combo_box_set_index (MX_COMBO_BOX (cluttersmith->cs_mode), 8); }

/******************************************************************************/

typedef struct KeyBinding {
  ClutterModifierType modifier;
  guint key_symbol;
  void (*callback) (ClutterActor *actor);
  gpointer callback_data;
} KeyBinding;

static KeyBinding keybindings[]={
  {CLUTTER_CONTROL_MASK, CLUTTER_x,         cs_cut},
  {CLUTTER_CONTROL_MASK, CLUTTER_c,         cs_copy},
  {CLUTTER_CONTROL_MASK, CLUTTER_v,         cs_paste},
  {CLUTTER_CONTROL_MASK, CLUTTER_d,         cs_duplicate},
  {CLUTTER_CONTROL_MASK, CLUTTER_l,         cs_focus_title},


  /* check for the more specific modifier state before the more generic ones */
  {CLUTTER_CONTROL_MASK|
   CLUTTER_SHIFT_MASK,   CLUTTER_g,         cs_ungroup},
  {CLUTTER_CONTROL_MASK, CLUTTER_g,         (void*)cs_group},
  {0,                    CLUTTER_Escape,    cs_select_parent},

  {0,                    CLUTTER_BackSpace, cs_remove},
  {0,                    CLUTTER_Delete,    cs_remove},
  {0,                    CLUTTER_Page_Up,   cs_raise},
  {0,                    CLUTTER_Page_Down, cs_lower},
  {0,                    CLUTTER_Home,      cs_raise_top},
  {0,                    CLUTTER_End,       cs_lower_bottom},

  {0,                    CLUTTER_Return,    cs_edit},

  {0,                    CLUTTER_Up,        cs_keynav_up},
  {0,                    CLUTTER_Down,      cs_keynav_down},
  {0,                    CLUTTER_Left,      cs_keynav_left},
  {0,                    CLUTTER_Right,     cs_keynav_right},


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
          keybindings[i].callback (keybindings[i].callback_data);
          return TRUE;
        }
    }
  return FALSE;
}


static KeyBinding global_keybindings[]={
  {CLUTTER_CONTROL_MASK, CLUTTER_q,           cs_quit},
  {0,                    CLUTTER_Scroll_Lock, cs_ui_mode},


  /* XXX: these shouldnt be here, they will interfere with the
   * app in browse mode,. but it is here now to steal them
   * from various entires that also want select all
   */
  {CLUTTER_CONTROL_MASK|
   CLUTTER_SHIFT_MASK,   CLUTTER_a,         cs_select_none},
  {CLUTTER_CONTROL_MASK, CLUTTER_a,         cs_select_all},


  {0,   CLUTTER_F1,  mode_browse2},
  {0,   CLUTTER_F2,  mode_annotate2},
  {0,   CLUTTER_F3,  mode_sketch2},
  {0,   CLUTTER_F4,  mode_edit2},
  {0,   CLUTTER_F5,  mode_animate2},
  {0,   CLUTTER_F6,  mode_callbacks2},
  {0,   CLUTTER_F7,  mode_code2},
  {0,   CLUTTER_F8,  mode_export2},
  {0,   CLUTTER_F9,  mode_config2},

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
          global_keybindings[i].callback (global_keybindings[i].callback_data);
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

static void menu_menu_hidden (gpointer menu)
{
  g_idle_add (delayed_destroy, menu);
}

static void menu_action_activated (gpointer a, gpointer b)
{
  clutter_actor_hide (a);
}

MxMenu *cs_menu_new (void)
{
  MxMenu *menu;
  menu = MX_MENU (mx_menu_new ());
  g_signal_connect (menu, "action-activated", G_CALLBACK (menu_action_activated), NULL);
  g_signal_connect (menu, "hide", G_CALLBACK (menu_menu_hidden), NULL);
  return menu;
}

void playback_menu (gint x,
                     gint y)
{
  MxMenu *menu = cs_menu_new ();
  MxAction *action;

  action = mx_action_new_full ("Edit (scrollock)", "Edit (scrollock)", G_CALLBACK (cs_ui_mode),  NULL);
  mx_menu_add_action (menu, action);
  action = mx_action_new_full ("Quit (ctrl q)", "Quit (ctrl q)", G_CALLBACK (cs_quit),  NULL);
  mx_menu_add_action (menu, action);
  clutter_group_add (cluttersmith->parasite_root, menu);
  clutter_actor_set_position (CLUTTER_ACTOR (menu), x, y);
  clutter_actor_show (CLUTTER_ACTOR (menu));
}

void root_menu (gint x,
                 gint y)
{
  MxMenu *menu = cs_menu_new ();
  MxAction *action;

  action = mx_action_new_full ("Browse (scrollock)", "Browse (scrollock)", G_CALLBACK (cs_ui_mode),  NULL);
  mx_menu_add_action (menu, action);

  action = mx_action_new_full ("reset view", "reset view", G_CALLBACK (cs_view_reset),  NULL);
  mx_menu_add_action (menu, action);

  if (clipboard)
    mx_menu_add_action (menu, mx_action_new_full ("Paste (ctrl v)", "Paste (ctrl v)", G_CALLBACK (cs_paste), NULL));
  action = mx_action_new_full ("Quit (ctrl q)", "Quit (ctrl q)", G_CALLBACK (cs_quit),  NULL);
  mx_menu_add_action (menu, mx_action_new_full ("Select All (ctrl a)", "Select All (ctrl a)", G_CALLBACK (cs_select_all), NULL));
  mx_menu_add_action (menu, action);

  clutter_group_add (cluttersmith->parasite_root, menu);
  clutter_actor_set_position (CLUTTER_ACTOR (menu), x, y);
  clutter_actor_show (CLUTTER_ACTOR (menu));

  if (cs_get_current_container () != cluttersmith->fake_stage)
    mx_menu_add_action (menu, mx_action_new_full ("Move up in tree (Escape)", "Move up in tree (Escape)", G_CALLBACK (cs_select_parent), NULL));
}

static void add_common (MxMenu *menu)
{
  MxAction *action;
  action = mx_action_new_full ("reset view", "reset view", G_CALLBACK (cs_view_reset),  NULL);
  mx_menu_add_action (menu, action);
  mx_menu_add_action (menu, mx_action_new_full ("______", "______", NULL, NULL));
  mx_menu_add_action (menu, mx_action_new_full ("Edit (Return)", "Edit (Return)", G_CALLBACK (cs_edit), NULL));
  mx_menu_add_action (menu, mx_action_new_full ("Raise (PgUp)", "Raise (PgUp)", G_CALLBACK (cs_raise), NULL));
  mx_menu_add_action (menu, mx_action_new_full ("Send to front (Home)", "Send to front (Home)", G_CALLBACK (cs_raise_top), NULL));
  mx_menu_add_action (menu, mx_action_new_full ("Send to back (End)", "Send to back (End)", G_CALLBACK (cs_lower_bottom), NULL));
  mx_menu_add_action (menu, mx_action_new_full ("Lower (PgDn)", "Lower (PgDn)", G_CALLBACK (cs_lower), NULL));
  mx_menu_add_action (menu, mx_action_new_full ("______", "_____", NULL, NULL));

  mx_menu_add_action (menu, mx_action_new_full ("Cut (ctrl x)", "Cut (ctrl x)", G_CALLBACK (cs_cut), NULL));
  mx_menu_add_action (menu, mx_action_new_full ("Copy (ctrl c)", "Copy (ctrl c)", G_CALLBACK (cs_copy), NULL));
  if (clipboard)
    mx_menu_add_action (menu, mx_action_new_full ("Paste (ctrl v)", "Paste (ctrl v)", G_CALLBACK (cs_paste), NULL));
  mx_menu_add_action (menu, mx_action_new_full ("Duplicate (ctrl d)", "Duplicate (ctrl d)", G_CALLBACK (cs_duplicate), NULL));
  mx_menu_add_action (menu, mx_action_new_full ("Remove (delete)", "Remove (delete)", G_CALLBACK (cs_remove), NULL));
  mx_menu_add_action (menu, mx_action_new_full ("______", "______", NULL, NULL));
  mx_menu_add_action (menu, mx_action_new_full ("Select All (ctrl a)", "Select All (ctrl a)", G_CALLBACK (cs_select_all), NULL));
  mx_menu_add_action (menu, mx_action_new_full ("Select None (shift ctrl a)", "Select None (shift ctrl a)", G_CALLBACK (cs_select_none), NULL));

  if (cs_get_current_container () != cluttersmith->fake_stage)
    mx_menu_add_action (menu, mx_action_new_full ("Move up in tree (ctrl p)", "Move up in tree (ctrl p)", G_CALLBACK (cs_select_parent), NULL));
}

static gboolean is_link (ClutterActor *actor)
{
  return (actor && MX_IS_BUTTON (actor) &&
          mx_stylable_get_style_class (MX_STYLABLE (actor)) &&
          g_str_equal (mx_stylable_get_style_class (MX_STYLABLE (actor)),
                    "ClutterSmithLink"));
}

static void destination_set2 (MxAction *action,
                              gpointer    data)
{
  ClutterActor *actor = cs_selected_get_any ();
  ClutterActor *text;
  gchar *value;
  value = g_strdup_printf ("link=%s", mx_action_get_name (action));
  g_print ("%s\n", value);
  clutter_actor_set_name (actor, value);
  text = cs_container_get_child_no (CLUTTER_CONTAINER(actor), 0);
  clutter_text_set_text (CLUTTER_TEXT (text), mx_action_get_name (action));
  g_free (value);
  cs_dirtied ();
}

static MxAction *destination_set (const gchar *name)
{
  MxAction *action;
  gchar *label;
  label = g_strdup (name);
  action = mx_action_new_full (label, label, G_CALLBACK (destination_set2), NULL);
  g_free (label);
  return action;
}


static void change_scene2 (MxAction *action,
                           gpointer    data)
{
  cluttersmith_load_scene (mx_action_get_name (action));
}

static MxAction *change_scene (const gchar *name)
{
  MxAction *action;
  gchar *label;
  label = g_strdup (name);
  action = mx_action_new_full (label, label, G_CALLBACK (change_scene2), NULL);
  g_free (label);
  return action;
}


void new_scene (MxAction *action,
                gpointer    ignored)
{
  ClutterActor *actor = cs_selected_get_any ();
  ClutterActor *text;
  if (!is_link (actor))
    return;
  text = cs_container_get_child_no (CLUTTER_CONTAINER(actor), 0);
  /* the name:link=link in the edited hyperlink is updated automatically */
  edit_text_start (text);
}

void link_edit_link (MxAction *action,
                     gpointer    ignored)
{
  MxMenu *menu = cs_menu_new ();
  GDir *dir;
  const gchar *path = cs_get_project_root ();
  const gchar *name;
  gint x, y;
  x = cs_last_x;
  y = cs_last_y;


  if (!path)
    return;

  action = mx_action_new_full ("new scene", "new scene", G_CALLBACK (new_scene), NULL);
  mx_menu_add_action (menu, action);

  dir = g_dir_open (path, 0, NULL);

  while ((name = g_dir_read_name (dir)))
    {
      gchar *name2;
      if (!g_str_has_suffix (name, ".json"))
        continue;
      name2 = g_strdup (name);
      *strstr (name2, ".json")='\0';

      action = destination_set (name2);
      g_free (name2);
      mx_menu_add_action (menu, action);
    }
  g_dir_close (dir);

  clutter_group_add (cluttersmith->parasite_root, menu);
  clutter_actor_set_position (CLUTTER_ACTOR (menu), x, y);
  clutter_actor_show (CLUTTER_ACTOR (menu));
}


static void change_project2 (MxAction *action,
                           gpointer    data)
{
  cluttersmith_set_project_root (mx_action_get_name (action));
}

static MxAction *change_project (const gchar *name)
{
  MxAction *action;
  gchar *label;
  label = g_strdup (name);
  action = mx_action_new_full (label, label, G_CALLBACK (change_project2), NULL);
  g_free (label);
  return action;
}

void projects_dropdown (MxAction *action,
                        gpointer  ignored)
{
  MxMenu *menu = cs_menu_new ();
  gint x, y;
  x = cs_last_x;
  y = cs_last_y;

    {
  gchar *config_path = cs_make_config_file ("session-history");
  gchar *original = NULL;

  if (g_file_get_contents (config_path, &original, NULL, NULL))
    {
      gchar *start, *end;
      start=end=original;
      while (*start)
        {
          end = strchr (start, '\n');
          if (*end)
            {
              MxAction     *action;
              *end = '\0';

              action = change_project (start);
              mx_menu_add_action (menu, action);
              start = end+1;
            }
          else
            {
              start = end;
            }
        }
      g_free (original);
    }
  }

  clutter_group_add (cluttersmith->parasite_root, menu);
  clutter_actor_set_position (CLUTTER_ACTOR (menu), x, y);
  clutter_actor_show (CLUTTER_ACTOR (menu));
}



void scenes_dropdown (MxAction *action,
                      gpointer  ignored)
{
  MxMenu *menu = cs_menu_new ();
  GDir *dir;
  const gchar *path = cs_get_project_root ();
  const gchar *name;
  gint x, y;
  x = cs_last_x;
  y = cs_last_y;


  if (!path)
    return;

  dir = g_dir_open (path, 0, NULL);

  while ((name = g_dir_read_name (dir)))
    {
      gchar *name2;
      if (!g_str_has_suffix (name, ".json"))
        continue;
      name2 = g_strdup (name);
      *strstr (name2, ".json")='\0';

      action = change_scene (name2);
      g_free (name2);
      mx_menu_add_action (menu, action);
    }
  g_dir_close (dir);

  clutter_group_add (cluttersmith->parasite_root, menu);
  clutter_actor_set_position (CLUTTER_ACTOR (menu), x, y);
  clutter_actor_show (CLUTTER_ACTOR (menu));
}

static void change_animation2 (MxAction *action,
                              gpointer    data)
{
  mx_entry_set_text (MX_ENTRY (cluttersmith->animation_name), mx_action_get_name (action));
}

static MxAction *change_animation (const gchar *name)
{
  MxAction *action;
  gchar *label;
  label = g_strdup (name);
  action = mx_action_new_full (label, label, G_CALLBACK (change_animation2), NULL);
  g_free (label);
  return action;
}

void animations_dropdown (MxAction *action,
                          gpointer  ignored)
{
  MxMenu *menu = cs_menu_new ();
  GList *i;
  gint x, y;
  x = cs_last_x;
  y = cs_last_y;

  for (i = cluttersmith->animators; i; i=i->next)
    {
       action = change_animation (clutter_scriptable_get_id (i->data));
       mx_menu_add_action (menu, action);
    }

  clutter_group_add (cluttersmith->parasite_root, menu);
  clutter_actor_set_position (CLUTTER_ACTOR (menu), x, y);
  clutter_actor_show (CLUTTER_ACTOR (menu));
}


static GList *actor_types_build (GList *list, GType type)
{
  GType *types;
  guint  children;
  gint   no;

  if (!type)
    return list;

  list = g_list_prepend (list, (void*)g_type_name (type));

  types = g_type_children (type, &children);

  for (no=0; no<children; no++)
    {
      list = actor_types_build (list, types[no]);
    }
  if (types)
    g_free (types);
  return list;
}

static void change_type2 (ClutterActor *button,
                          const gchar  *name)
{
  ClutterActor *actor = cs_selected_get_any ();
  cs_container_remove_children (cluttersmith->property_editors);
  if (!actor)
    {
      return;
    }
  actor = cs_actor_change_type (actor, name);

  if (g_str_equal (name, "ClutterText"))
    {
      g_object_set (G_OBJECT (actor), "text", "New Text", NULL);
    }

  cs_selected_clear ();
  cs_selected_add (actor);
}

void cs_change_type (MxAction *action,
                     gpointer  ignored)
{
  MxMenu *menu = cs_menu_new ();
  GList *a, *actor_types;
  gint x, y;
  x = cs_last_x;
  y = cs_last_y;

  actor_types = actor_types_build (NULL, CLUTTER_TYPE_ACTOR);
  actor_types = g_list_sort (actor_types, (void*)strcmp);


  if (!actor_types)
    return;

  for (a = actor_types; a; a = a->next)
    {
      mx_menu_add_action (menu, mx_action_new_full (a->data, a->data,
                                             G_CALLBACK(change_type2), a->data));
    }
  g_list_free (actor_types);

  clutter_group_add (cluttersmith->parasite_root, menu);
  clutter_actor_set_position (CLUTTER_ACTOR (menu), x, 50 /* y */);
  clutter_actor_show (CLUTTER_ACTOR (menu));
}

void object_menu (ClutterActor *actor,
                   gint          x,
                   gint          y)
{
  MxMenu *menu = cs_menu_new ();


  if (is_link (actor))
    {
      gchar *label = "type: Link";
      mx_menu_add_action (menu, mx_action_new_full (label, label, NULL, NULL));
    }
  else
    {
      gchar *label = g_strdup_printf ("type: %s", G_OBJECT_TYPE_NAME (actor));
      mx_menu_add_action (menu, mx_action_new_full (label, label, G_CALLBACK (cs_change_type), NULL));
      g_free (label);
    }

  if (CLUTTER_IS_GROUP (actor))
    {
      mx_menu_add_action (menu, mx_action_new_full ("Ungroup (shift ctrl g)", "Ungroup (shift ctrl g)", G_CALLBACK (cs_ungroup), NULL));
      mx_menu_add_action (menu, mx_action_new_full ("Make box", "Make box", G_CALLBACK (cs_make_group_box), NULL));
    }
  else if (MX_IS_BUTTON (actor) &&
           mx_stylable_get_style_class (MX_STYLABLE (actor)) &&
           g_str_equal (mx_stylable_get_style_class (MX_STYLABLE (actor)),
                        "ClutterSmithLink"))
    {
      mx_menu_add_action (menu, mx_action_new_full ("Change destination", "Change destination", G_CALLBACK (link_edit_link), NULL));
    }
  else if (CLUTTER_IS_CONTAINER (actor))
    {
      mx_menu_add_action (menu, mx_action_new_full ("Make group", "Make group", G_CALLBACK (cs_make_group), NULL));
    }
  
  add_common (menu);

  clutter_group_add (cluttersmith->parasite_root, menu);
  clutter_actor_set_position (CLUTTER_ACTOR (menu), x, y);
  clutter_actor_show (CLUTTER_ACTOR (menu));
}

void selection_menu (gint x,
                      gint y)
{
  MxMenu *menu = cs_menu_new ();
  mx_menu_add_action (menu, mx_action_new_full ("Group (ctrl g)", "Group (ctrl g)", G_CALLBACK (cs_group), NULL));
  mx_menu_add_action (menu, mx_action_new_full ("Make box (ctrl b)", "Make box (ctrl b)", G_CALLBACK (cs_make_box), NULL));
  add_common (menu);

  clutter_group_add (cluttersmith->parasite_root, menu);
  clutter_actor_set_position (CLUTTER_ACTOR (menu), x, y);
  clutter_actor_show (CLUTTER_ACTOR (menu));
}

void cs_states_expand_complete (MxExpander *expander)
{
  if (mx_expander_get_expanded (expander))
    clutter_actor_show (cluttersmith->dialog_animator);
  else
    clutter_actor_hide (cluttersmith->dialog_animator);
}
