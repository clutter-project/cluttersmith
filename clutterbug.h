
#ifndef CLUTTER_BUG_H
#define CLUTTER_BUG_H

/* Things that need to go in headers / get proper API: */
void select_item (ClutterActor *button, ClutterActor *item);
extern ClutterActor *selected_actor;
extern guint CB_REV;
extern guint CB_SAVED_REV;

void actor_editing_init (gpointer stage);

extern ClutterActor *parasite_root;
extern ClutterActor *parasite_ui;

void set_title (const gchar *new_title);
void select_item (ClutterActor *button, ClutterActor *item);

/* actor-editing: */

extern GHashTable *selected;

#include "util.h"
#endif
