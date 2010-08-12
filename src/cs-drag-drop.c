#include <clutter/clutter.h>
#include <string.h>
#include "cluttersmith.h"
#include <gtk/gtk.h>
#include <clutter-gtk/clutter-gtk.h>

static void
drag_data_received (GtkWidget          *widget,
                    GdkDragContext     *dc,
                    gint                x,
                    gint                y,
                    GtkSelectionData   *data,
                    guint               info,
                    guint               time,
                    gpointer            p)
{
  GFile *file;
  GFile *copy;
  gchar *tmp;
  const gchar *urilist = NULL;
  gchar **uris;
  gchar **uri;
  ClutterActor *actor, *parent;

  urilist = (const gchar *)gtk_selection_data_get_data (data);
  if (!urilist)
    return;

  uris = g_strsplit (urilist, "\r\n", -1);

  for (uri = uris; *uri; uri++)
    {
      GError *error = NULL;
      GFileInfo *info;
      const gchar *content_type;

      file = g_file_new_for_uri (*uri);
      tmp = g_strdup_printf ("%s/%s", cs_get_project_root (), g_file_get_basename (file));
      copy = g_file_new_for_path (tmp);

      info = g_file_query_info (file, "standard::*", G_FILE_QUERY_INFO_NONE, NULL, NULL);

      if (!info)
        goto skip;
      content_type = g_file_info_get_content_type (info);

      if (!g_str_has_prefix (content_type, "image"))
        goto skip;


      g_file_copy (file, copy, G_FILE_COPY_OVERWRITE, FALSE, NULL, NULL, &error);
      if (error)
        {
          g_print ("Failed to copy image: %s\n", error->message);
          goto skip;
        }

      parent = cs_get_current_container ();
      actor = clutter_texture_new_from_file (tmp, &error);

      if (error)
        {
          g_print ("Failed to open image: %s\n", error->message);
          goto skip;
        }

      clutter_container_add_actor (CLUTTER_CONTAINER (parent), actor);

      clutter_actor_set_position (actor, x - clutter_actor_get_x (cs->fake_stage),
                                         y - clutter_actor_get_y (cs->fake_stage));
      x += 10;
      y += 10;
      cs_selected_clear ();
      cs_actor_make_id_unique (actor, g_file_get_basename (copy));
      cs_selected_add (actor);

      cs_dirtied ();
skip: 
      g_free (tmp);
      g_object_unref (info);
      g_object_unref (file);
      g_object_unref (copy);
    }
  g_strfreev (uris);
}

void cs_drag_drop_init (GtkWidget *clutter)
{
  const GtkTargetEntry drop_types[] = {{ "text/uri-list", 0, 0 }};

  gtk_drag_dest_set (clutter, GTK_DEST_DEFAULT_ALL, drop_types, 1, GDK_ACTION_COPY);

  g_signal_connect(clutter, "drag-data-received",
                   G_CALLBACK(drag_data_received),
                   NULL);
}
