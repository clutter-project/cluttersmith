#include "cs-texture.h"
#include "cluttersmith.h"

#include <glib.h>

struct _CSTexturePrivate
{
  gchar *image;
};

enum {
  PROP_0,

  /* ClutterMedia proprs */
  PROP_IMAGE
};


#define TICK_TIMEOUT 0.5


G_DEFINE_TYPE (CSTexture,
               cs_texture,
               CLUTTER_TYPE_TEXTURE);

static void
set_uri (CSTexture   *texture,
         const gchar *name)
{
  CSTexturePrivate *priv = texture->priv;
  GObject *self = G_OBJECT (texture);

  g_free (priv->image);

  if (name) 
    {
      gchar *fullpath;
      priv->image = g_strdup (name);
      fullpath = g_strdup_printf ("%s/%s", cs_get_project_root (), name);
      g_object_set (texture, "filename", fullpath, NULL);
      g_free (fullpath);
    }
  else 
    {
      priv->image= NULL;
    }

  g_object_notify (self, "image");
}


static void
cs_texture_dispose (GObject *object)
{
  CSTexture        *self;
  CSTexturePrivate *priv; 

  self = CB_TEXTURE(object); 
  priv = self->priv;

  G_OBJECT_CLASS (cs_texture_parent_class)->dispose (object);
}

static void
cs_texture_finalize (GObject *object)
{
  CSTexture        *self;
  CSTexturePrivate *priv; 

  self = CB_TEXTURE (object);
  priv = self->priv;

  if (priv->image)
    g_free (priv->image);

  G_OBJECT_CLASS (cs_texture_parent_class)->finalize (object);
}

static void
cs_texture_set_property (GObject      *object, 
				        guint         property_id,
				        const GValue *value, 
				        GParamSpec   *pspec)
{
  CSTexture *texture;

  texture = CB_TEXTURE (object);

  switch (property_id)
    {
    case PROP_IMAGE:
      {
        set_uri (texture, g_value_get_string (value));
      }
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
cs_texture_get_property (GObject    *object, 
				        guint       property_id,
				        GValue     *value, 
				        GParamSpec *pspec)
{
  CSTexture *texture;
  CSTexturePrivate *priv;

  texture = CB_TEXTURE (object);
  priv = texture->priv;

  switch (property_id)
    {
    case PROP_IMAGE:
      g_value_set_string (value, priv->image);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
cs_texture_class_init (CSTextureClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (CSTexturePrivate));

  object_class->dispose      = cs_texture_dispose;
  object_class->finalize     = cs_texture_finalize;
  object_class->set_property = cs_texture_set_property;
  object_class->get_property = cs_texture_get_property;


  g_object_class_install_property
    (object_class, PROP_IMAGE,
     g_param_spec_string ("image",
                          "Image",
                          "An image file in the current cluttersmith project root",
                          NULL,
                          G_PARAM_READABLE|G_PARAM_WRITABLE));


}

static void
cs_texture_init (CSTexture *texture)
{
  CSTexturePrivate *priv;

  texture->priv = priv =
    G_TYPE_INSTANCE_GET_PRIVATE (texture,
                                 CB_TYPE_TEXTURE,
                                 CSTexturePrivate);
}

/**
 * cs_texture_new:
 *
 * Creates a video texture.
 *
 * Return value: the newly created video texture actor
 */
ClutterActor*
cs_texture_new (void)
{
  return g_object_new (CB_TYPE_TEXTURE, NULL);
}
