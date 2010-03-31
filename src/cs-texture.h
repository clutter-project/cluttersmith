#ifndef _HAVE_CB_TEXTURE_H
#define _HAVE_CB_TEXTURE_H

#include <glib-object.h>
#include <clutter/clutter.h>

G_BEGIN_DECLS

#define CB_TYPE_TEXTURE cs_texture_get_type()

#define CB_TEXTURE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  CB_TYPE_TEXTURE, CSTexture))

#define CB_TEXTURE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  CB_TYPE_TEXTURE, CSTextureClass))

#define CB_IS_TEXTURE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  CB_TYPE_TEXTURE))

#define CB_IS_TEXTURE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  CB_TYPE_TEXTURE))

#define CB_TEXTURE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  CB_TYPE_TEXTURE, CSTextureClass))

typedef struct _CSTexture        CSTexture;
typedef struct _CSTextureClass   CSTextureClass;
typedef struct _CSTexturePrivate CSTexturePrivate;

struct _CSTexture
{
  ClutterTexture              parent;
  CSTexturePrivate *priv;
}; 

struct _CSTextureClass 
{
  ClutterTextureClass parent_class;
}; 

GType         cs_texture_get_type    (void) G_GNUC_CONST;
ClutterActor *cs_texture_new         (void);

G_END_DECLS

#endif
