#include <clutter/clutter.h>

void export_png (const gchar *scene,
                 const gchar *png)
{
  g_print ("exporting scene %s to %s\n", scene, png);
}

void export_pdf (const gchar *pdf)
{
  g_print ("exporting scenes to %s\n", pdf);
}
