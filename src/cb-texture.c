#include "cb-texture.h"

#include <glib.h>

struct _CBTexturePrivate
{
  gchar *uri;
};

enum {
  PROP_0,

  /* ClutterMedia proprs */
  PROP_URI
};


#define TICK_TIMEOUT 0.5


G_DEFINE_TYPE (CBTexture,
               cb_texture,
               CLUTTER_TYPE_TEXTURE);

static void
set_uri (CBTexture   *texture,
         const gchar *uri)
{
  CBTexturePrivate *priv = texture->priv;
  GObject *self = G_OBJECT (texture);

  g_free (priv->uri);

  if (uri) 
    {
      priv->uri = g_strdup (uri);
      g_object_set (texture, "filename", uri, NULL);
    }
  else 
    {
      priv->uri = NULL;
    }

  g_object_notify (self, "uri");
}


static void
cb_texture_dispose (GObject *object)
{
  CBTexture        *self;
  CBTexturePrivate *priv; 

  self = CB_TEXTURE(object); 
  priv = self->priv;

  G_OBJECT_CLASS (cb_texture_parent_class)->dispose (object);
}

static void
cb_texture_finalize (GObject *object)
{
  CBTexture        *self;
  CBTexturePrivate *priv; 

  self = CB_TEXTURE (object);
  priv = self->priv;

  g_free (priv->uri);

  G_OBJECT_CLASS (cb_texture_parent_class)->finalize (object);
}

static void
cb_texture_set_property (GObject      *object, 
				        guint         property_id,
				        const GValue *value, 
				        GParamSpec   *pspec)
{
  CBTexture *texture;

  texture = CB_TEXTURE (object);

  switch (property_id)
    {
    case PROP_URI:
      set_uri (texture, g_value_get_string (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
cb_texture_get_property (GObject    *object, 
				        guint       property_id,
				        GValue     *value, 
				        GParamSpec *pspec)
{
  CBTexture *texture;
  CBTexturePrivate *priv;

  texture = CB_TEXTURE (object);
  priv = texture->priv;

  switch (property_id)
    {
    case PROP_URI:
      g_value_set_string (value, priv->uri);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
cb_texture_class_init (CBTextureClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (CBTexturePrivate));

  object_class->dispose      = cb_texture_dispose;
  object_class->finalize     = cb_texture_finalize;
  object_class->set_property = cb_texture_set_property;
  object_class->get_property = cb_texture_get_property;


  g_object_class_install_property
    (object_class, PROP_URI,
     g_param_spec_string ("uri",
                          "URI",
                          "The URI containing imagedata for the texture",
                          NULL,
                          G_PARAM_READABLE|G_PARAM_WRITABLE));


}

static void
cb_texture_init (CBTexture *texture)
{
  CBTexturePrivate *priv;

  texture->priv = priv =
    G_TYPE_INSTANCE_GET_PRIVATE (texture,
                                 CB_TYPE_TEXTURE,
                                 CBTexturePrivate);
}

/**
 * cb_texture_new:
 *
 * Creates a video texture.
 *
 * Return value: the newly created video texture actor
 */
ClutterActor*
cb_texture_new (void)
{
  return g_object_new (CB_TYPE_TEXTURE,
                       "disable-slicing", TRUE, 
                       NULL);
}
