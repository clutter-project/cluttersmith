#ifndef UTIL_H
#define UTIL_H

#include "clutter-states.h"

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

ClutterActor  *cluttersmith_get_actor              (const gchar  *id);
ClutterActor  *cluttersmith_get_stage              (void);


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
ClutterActor  *cs_container_get_child_no           (ClutterContainer *container,
                                                    gint              pos);
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


ClutterActor * cs_find_nearest                     (ClutterActor *actor,
                                                    gboolean      vertical,
                                                    gboolean      reverse);

void           cs_properties_store_defaults        (void);
void           cs_properties_restore_defaults      (void);

ClutterAnimator * cs_states_make_animator (ClutterStates *states,
                                           const gchar   *source_state,
                                           const gchar   *target_state);

void           cluttersmith_init                   (void);
void           cluttersmith_print                  (gchar *a);

/* initially copies value from propertyB to propertyA,
 * it is possible to reassing a new property to propertyA
 */
void           cs_bridge_properties                (GObject     *objectA,
                                                    const gchar *propertyA,
                                                    GObject     *objectB,
                                                    const gchar *propertyB);

#endif
