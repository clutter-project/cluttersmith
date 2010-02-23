#include "cluttersmith.h"
#include <animator-editor.h>

enum
{
  PROP_0,
};

struct _CsAnimatorEditorPrivate
{
  ClutterActor    *background;
  ClutterAnimator *animator;
};

#define GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), \
                                                       CS_TYPE_ANIMATOR_EDITOR, \
                                                       CsAnimatorEditorPrivate))
G_DEFINE_TYPE (CsAnimatorEditor, cs_animator_editor, CLUTTER_TYPE_ACTOR)

static void
cs_animator_editor_finalize (GObject *object)
{
  G_OBJECT_CLASS (cs_animator_editor_parent_class)->finalize (object);
}

static void
cs_animator_editor_dispose (GObject *object)
{
    CsAnimatorEditor *editor = (CsAnimatorEditor *) object;
    CsAnimatorEditorPrivate *priv;
    priv = editor->priv;

    if (priv->animator)
      g_object_unref (priv->animator);
    priv->animator = NULL;

    G_OBJECT_CLASS (cs_animator_editor_parent_class)->dispose (object);
}

static void
cs_animator_editor_set_property (GObject      *object,
                               guint         prop_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
    switch (prop_id) {
    default:
        break;
    }
}

static void
cs_animator_editor_get_property (GObject *object, guint prop_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  switch (prop_id)
    {
      default:
        break;
    }
}

static void
cs_animator_editor_allocate (ClutterActor *actor, const ClutterActorBox *box,
                           ClutterAllocationFlags flags)
{
  CsAnimatorEditor        *editor = (CsAnimatorEditor *) actor;
  CsAnimatorEditorPrivate *priv   = editor->priv;

  CLUTTER_ACTOR_CLASS (cs_animator_editor_parent_class)->allocate
                             (actor, box, flags);

  if (priv->background ) 
      clutter_actor_allocate_preferred_size (priv->background,
                                             flags);
}

static void
cs_animator_editor_paint (ClutterActor *actor)
{
  CsAnimatorEditor        *editor = (CsAnimatorEditor *) actor;
  CsAnimatorEditorPrivate *priv   = editor->priv;

  if (priv->background)
     clutter_actor_paint (priv->background);
}

static void
cs_animator_editor_pick (ClutterActor *actor, const ClutterColor *color)
{
    cs_animator_editor_paint (actor);
}

static void
cs_animator_editor_map (ClutterActor *self)
{
    CsAnimatorEditorPrivate *priv = CS_ANIMATOR_EDITOR (self)->priv;

    CLUTTER_ACTOR_CLASS (cs_animator_editor_parent_class)->map (self);

    if (priv->background)
      clutter_actor_map (CLUTTER_ACTOR (priv->background));
}

static void
cs_animator_editor_unmap (ClutterActor *self)
{
    CsAnimatorEditorPrivate *priv = CS_ANIMATOR_EDITOR (self)->priv;

    CLUTTER_ACTOR_CLASS (cs_animator_editor_parent_class)->unmap (self);

    if (priv->background)
      clutter_actor_unmap (CLUTTER_ACTOR (priv->background));
}


static void
cs_animator_editor_class_init (CsAnimatorEditorClass *klass)
{
    GObjectClass      *o_class = (GObjectClass *) klass;
    ClutterActorClass *a_class = (ClutterActorClass *) klass;

    o_class->dispose      = cs_animator_editor_dispose;
    o_class->finalize     = cs_animator_editor_finalize;
    o_class->set_property = cs_animator_editor_set_property;
    o_class->get_property = cs_animator_editor_get_property;

    a_class->allocate = cs_animator_editor_allocate;
    a_class->paint    = cs_animator_editor_paint;
    a_class->pick     = cs_animator_editor_pick;
    a_class->map      = cs_animator_editor_map;
    a_class->unmap    = cs_animator_editor_unmap;

    g_type_class_add_private (klass, sizeof (CsAnimatorEditorPrivate));
}

static void
cs_animator_editor_init (CsAnimatorEditor *self)
{
  CsAnimatorEditorPrivate *priv = GET_PRIVATE (self);

  self->priv = priv;

  priv->background = clutter_rectangle_new ();
  priv->animator = NULL;
  clutter_actor_set_reactive (priv->background, TRUE);
  clutter_actor_set_parent (priv->background, CLUTTER_ACTOR (self));
  clutter_actor_show (priv->background);
}

void cs_animator_editor_set_animator (CsAnimatorEditor *animator_editor,
                                      ClutterAnimator  *animator)
{
  CsAnimatorEditorPrivate *priv = animator_editor->priv;
  if (priv->animator)
    g_object_unref (priv->animator);
  priv->animator = g_object_ref (animator);
}

