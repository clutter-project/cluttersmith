
#ifndef CLUTTERSMITH_H
#define CLUTTERSMITH_H

/* Things that need to go in headers / get proper API: */
extern ClutterActor *active_actor;
extern guint CS_REVISION;
extern guint CS_STORED_REVISION;

void actor_editing_init (gpointer stage);

extern ClutterActor *parasite_root;
extern ClutterActor *parasite_ui;

void set_title (const gchar *new_title);
void select_item (ClutterActor *item);

void tree_populate (ClutterActor *scene_graph,
                    ClutterActor *active_actor);

/* actor-editing: */

/* selection */

GList   *cluttersmith_selected_get_list  (void);
gint     cluttersmith_selected_count     (void);
gboolean cluttersmith_selected_has_actor (ClutterActor *actor);
void     cluttersmith_selected_clear     (void);
void     cluttersmith_selected_init      (void);
void     cluttersmith_selected_add       (ClutterActor *actor);
void     cluttersmith_selected_remove    (ClutterActor *actor);
void     cluttersmith_selected_foreach   (GCallback     cb,
                                          gpointer      data);
gpointer cluttersmith_selected_match     (GCallback     match_fun,
                                          gpointer      data);


#endif
