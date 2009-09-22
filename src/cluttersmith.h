
#ifndef CLUTTERSMITH_H
#define CLUTTERSMITH_H

/* Things that need to go in headers / get proper API: */
extern guint CS_REVISION;
extern guint CS_STORED_REVISION;


void cluttersmith_actor_editing_init (gpointer stage);

extern ClutterActor *parasite_root;
extern ClutterActor *parasite_ui;
extern ClutterActor *fake_stage;

void   cluttersmith_set_project_root (const gchar *new_root);
gchar *cluttersmith_get_project_root (void);

void cluttersmith_open_layout (const gchar *new_title);
void cluttersmith_set_active (ClutterActor *item);
ClutterActor *cluttersmith_get_active (void);

void cluttersmith_tree_populate (ClutterActor *scene_graph,
                    ClutterActor *active_actor);

ClutterActor *cluttersmith_get_add_root (ClutterActor *actor);
void          cluttersmith_set_add_root (ClutterActor *actor);

void cluttsmith_show_chrome (void);
void cluttsmith_hide_chrome (void);


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
gboolean cluttersmith_save_timeout       (gpointer data);
gpointer cluttersmith_selected_match     (GCallback     match_fun,
                                          gpointer      data);
ClutterActor *cluttersmith_selected_get_any (void);

/*****/

void previews_reload (ClutterActor *actor);
char * cluttersmith_make_config_file (const char *filename);

#endif
