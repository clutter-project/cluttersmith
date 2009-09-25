/* cluttersmith-context.h */

#ifndef _CLUTTERSMITH_CONTEXT_H
#define _CLUTTERSMITH_CONTEXT_H

#include <clutter/clutter.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define CLUTTERSMITH_TYPE_CONTEXT cluttersmith_context_get_type()

#define CLUTTERSMITH_CONTEXT(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  CLUTTERSMITH_TYPE_CONTEXT, CluttersmithContext))

#define CLUTTERSMITH_CONTEXT_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  CLUTTERSMITH_TYPE_CONTEXT, CluttersmithContextClass))

#define CLUTTERSMITH_IS_CONTEXT(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  CLUTTERSMITH_TYPE_CONTEXT))

#define CLUTTERSMITH_IS_CONTEXT_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  CLUTTERSMITH_TYPE_CONTEXT))

#define CLUTTERSMITH_CONTEXT_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  CLUTTERSMITH_TYPE_CONTEXT, CluttersmithContextClass))

typedef struct _CluttersmithContext CluttersmithContext;
typedef struct _CluttersmithContextClass CluttersmithContextClass;
typedef struct _CluttersmithContextPrivate CluttersmithContextPrivate;

struct _CluttersmithContext
{
  GObject parent;

  CluttersmithContextPrivate *priv;
  gint          cluttersmith_ui_mode;
  ClutterActor *parasite_root;
  ClutterActor *parasite_ui;
  ClutterActor *fake_stage;
  ClutterActor *scene_graph;
  gchar        *project_root;
};

struct _CluttersmithContextClass
{
  GObjectClass parent_class;
};

GType cluttersmith_context_get_type (void);

CluttersmithContext *cluttersmith_context_new (void);

G_END_DECLS

#endif /* _CLUTTERSMITH_CONTEXT_H */
