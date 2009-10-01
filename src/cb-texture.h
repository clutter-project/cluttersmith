#ifndef _HAVE_CB_TEXTURE_H
#define _HAVE_CB_TEXTURE_H

#include <glib-object.h>
#include <clutter/clutter.h>

G_BEGIN_DECLS

#define CB_TYPE_TEXTURE cs_texture_get_type()

#define CB_TEXTURE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  CB_TYPE_TEXTURE, CBTexture))

#define CB_TEXTURE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  CB_TYPE_TEXTURE, CBTextureClass))

#define CB_IS_TEXTURE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  CB_TYPE_TEXTURE))

#define CB_IS_TEXTURE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  CB_TYPE_TEXTURE))

#define CB_TEXTURE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  CB_TYPE_TEXTURE, CBTextureClass))

typedef struct _CBTexture        CBTexture;
typedef struct _CBTextureClass   CBTextureClass;
typedef struct _CBTexturePrivate CBTexturePrivate;

struct _CBTexture
{
  ClutterTexture              parent;
  CBTexturePrivate *priv;
}; 

struct _CBTextureClass 
{
  ClutterTextureClass parent_class;
}; 

GType         cs_texture_get_type    (void) G_GNUC_CONST;
ClutterActor *cs_texture_new         (void);

G_END_DECLS

#endif
