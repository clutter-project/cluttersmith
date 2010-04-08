#ifndef __CS_HISTORY_H_
#define __CS_HISTORY_H_

#include <clutter/clutter.h>

void cs_history_undo (ClutterActor *ignored);
void cs_history_redo (ClutterActor *ignored);

void cs_history_add  (const gchar *name,
                      const gchar *javascript_do,
                      const gchar *javascript_undo);

void cs_history_do   (const gchar *name,
                      const gchar *javascript_do,
                      const gchar *javascript_undo);

#endif
