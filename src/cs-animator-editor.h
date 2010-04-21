#ifndef __CS_ANIMATOR_EDITOR_H__
#define __CS_ANIMATOR_EDITOR_H__

#include <clutter/clutter.h>

G_BEGIN_DECLS

#define CS_TYPE_ANIMATOR_EDITOR    \
  (cs_animator_editor_get_type ())
#define CS_ANIMATOR_EDITOR(obj)    \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj),  \
                               CS_TYPE_ANIMATOR_EDITOR, \
                               CsAnimatorEditor))
#define CS_ANIMATOR_EDITOR_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass),    \
                            CS_TYPE_ANIMATOR_EDITOR, \
                            CsAnimatorEditorClass))
#define IS_CS_ANIMATOR_EDITOR(obj)    \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
                               CS_TYPE_ANIMATOR_EDITOR))
#define IS_CS_ANIMATOR_EDITOR_CLASS(klass)  \
  (G_TYPE_CHECK_CLASS_TYPE ((klass),    \
                            CS_TYPE_ANIMATOR_EDITOR))
#define CS_ANIMATOR_EDITOR_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj),     \
                              CS_TYPE_ANIMATOR_EDITOR, \
                              CsAnimatorEditorClass))

typedef struct _CsAnimatorEditorPrivate CsAnimatorEditorPrivate;
typedef struct _CsAnimatorEditor CsAnimatorEditor;
typedef struct _CsAnimatorEditorClass CsAnimatorEditorClass;

struct _CsAnimatorEditor
{
  ClutterActor parent;

  CsAnimatorEditorPrivate *priv;
};

struct _CsAnimatorEditorClass
{
  ClutterActorClass parent_class;
};

GType cs_animator_editor_get_type (void) G_GNUC_CONST;

void cs_animator_editor_set_animator (CsAnimatorEditor *animator_editor,
                                      ClutterAnimator  *animator);

void cs_animator_editor_set_progress (CsAnimatorEditor *animator_editor,
                                      gdouble           progress); 
gdouble cs_animator_editor_get_progress (CsAnimatorEditor *animator_editor);
void cs_animator_editor_stage_paint (void);

G_END_DECLS

#endif /* __CS_ANIMATOR_EDITOR_H__ */
