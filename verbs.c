#include <clutter/clutter.h>




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

static gboolean manipulator_key_pressed (ClutterActor *stage, guint key)
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
