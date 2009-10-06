#ifndef UTIL_H
#define UTIL_H

/* like foreach, but returns the first non NULL return value (and
 * stops iterating)
 */
gpointer      cs_list_match                        (GList            *list,
                                                    GCallback         match_fun,
                                                    gpointer          data);

/* changes the type of actor into the new type, preserving common properties,
 * position in parent container and child properties
 */
ClutterActor  *cs_actor_change_type                (ClutterActor     *actor,
                                                    const gchar      *new_type);

void           cs_container_remove_children        (ClutterActor *container);
gboolean       cs_actor_has_ancestor               (ClutterActor *actor,
                                                    ClutterActor *ancestor);
gboolean       cs_movable_press                    (ClutterActor *actor,
                                                    ClutterEvent *event);
ClutterActor  *cs_find_by_id                       (ClutterActor *stage,
                                                    const gchar  *id);
ClutterActor  *cs_find_by_id_int                   (ClutterActor *stage,
                                                    const gchar  *id);

GList         *cs_container_get_children_recursive (ClutterActor *actor);
GList         *cs_actor_get_siblings               (ClutterActor *actor);
gint           cs_get_sibling_no                   (ClutterActor *actor);
gint           cs_actor_ancestor_depth             (ClutterActor *actor);
void           cs_container_replace_child          (ClutterContainer *container,
                                                    ClutterActor     *old,
                                                    ClutterActor     *new);
void           cs_container_add_actor_at           (ClutterContainer *container,
                                                    ClutterActor     *actor,
                                                    gint              pos);

ClutterActor  *cs_duplicator                       (ClutterActor *actor,
                                                    ClutterActor *parent);



ClutterActor  *cs_show                             (const gchar  *name);
ClutterActor  *cs_load_json                        (const gchar  *name);
ClutterScript *cs_get_script                       (ClutterActor *actor);
ClutterActor  *cs_replace_content2                 (ClutterActor *actor,
                                                    const gchar  *name,
                                                    const gchar  *new_script);

#endif
