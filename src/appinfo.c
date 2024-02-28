#include <gio/gio.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <gio/gdesktopappinfo.h>

GHashTable *app_info_wm_class_map;
GtkIconTheme *app_info_theme;

static void app_info_monitor_cb ( GAppInfoMonitor *mon, gpointer d )
{
  GDesktopAppInfo *app;
  GList *list, *iter;
  const gchar *class, *id;

  list = g_app_info_get_all();
  g_hash_table_remove_all(app_info_wm_class_map);
  for(iter=list; iter; iter=g_list_next(iter))
  {
    if( (id = g_app_info_get_id(iter->data)) &&
        (app = g_desktop_app_info_new(id)) )
    {
      if( (class = g_desktop_app_info_get_startup_wm_class(app)) )
        g_hash_table_insert(app_info_wm_class_map, g_strdup(class),
            g_strdup(id));
      g_object_unref(G_OBJECT(app));
    }
  }
}

void app_info_init ( void )
{
  GAppInfoMonitor *mon;

  app_info_wm_class_map = g_hash_table_new_full(g_str_hash, g_str_equal,
      g_free, g_free);
  app_info_theme = gtk_icon_theme_get_default();
  mon = g_app_info_monitor_get();
  g_signal_connect(G_OBJECT(mon), "changed", (GCallback)app_info_monitor_cb,
      NULL);
  app_info_monitor_cb(mon, NULL);
}

gchar *app_info_icon_test ( const gchar *icon, gboolean symbolic_pref )
{
  GtkIconInfo *info;
  gchar *sym_icon;

  if(!icon)
    return NULL;

/* if symbolic icon is preferred test for symbolic first */
  if(symbolic_pref)
  {
    sym_icon = g_strconcat(icon, "-symbolic", NULL);
    if( (info = gtk_icon_theme_lookup_icon(app_info_theme, sym_icon, 16, 0)) )
    {
      g_object_unref(G_OBJECT(info));
      return sym_icon;
    }
    g_free(sym_icon);
  }

/* if symbolic preference isn't set or symbolic icon isn't found */
  if( (info = gtk_icon_theme_lookup_icon(app_info_theme, icon, 16, 0)) )
  {
    g_object_unref(G_OBJECT(info));
    return g_strdup(icon);
  }

/* if symbolic preference isn't set, but non-symbolic icon isn't found */
  if(!symbolic_pref)
  {
    sym_icon = g_strconcat(icon, "-symbolic", NULL);
    if( (info = gtk_icon_theme_lookup_icon(app_info_theme, sym_icon, 16, 0)) )
    {
      g_object_unref(G_OBJECT(info));
      return sym_icon;
    }
    g_free(sym_icon);
  }

  return NULL;
}

gchar *app_info_icon_get ( const gchar *app_id, gboolean symbolic_pref )
{
  GDesktopAppInfo *app;
  char *icon, *icon_name;

  if( !(app = g_desktop_app_info_new(app_id)) )
    return NULL;

  if(g_desktop_app_info_get_nodisplay(app))
    icon = NULL;
  else
  {
    icon_name = g_desktop_app_info_get_string(app, "Icon");
    icon = app_info_icon_test(icon_name, symbolic_pref);
    g_free(icon_name);
  }

  g_object_unref(G_OBJECT(app));
  return icon;
}

static gchar *app_info_lookup_id ( gchar *app_id, gboolean symbolic_pref )
{
  gchar ***desktop, *wmmap, *icon = NULL;
  gint i,j;

  if( (icon = app_info_icon_test(app_id, symbolic_pref)) )
    return icon;

  desktop = g_desktop_app_info_search(app_id);
  for(j=0;desktop[j];j++)
  {
    if(!icon)
      for(i=0;desktop[j][i];i++)
        if( (icon = app_info_icon_get(desktop[j][i], symbolic_pref)) )
          break;
    g_strfreev(desktop[j]);
  }
  g_free(desktop);

  if(!icon && (wmmap = g_hash_table_lookup(app_info_wm_class_map, app_id)) )
    icon = app_info_icon_get(wmmap, symbolic_pref);

  return icon;
}

gchar *app_info_icon_lookup ( gchar *app_id, gboolean symbolic_pref )
{
  gchar *clean_app_id, *lower_app_id, *icon;

  if(g_str_has_suffix(app_id, "-symbolic"))
  {
    symbolic_pref = TRUE;
    clean_app_id = g_strndup(app_id, strlen(app_id) - 9);
  }
  else
    clean_app_id = g_strdup(app_id);

  if( (icon = app_info_lookup_id(clean_app_id, symbolic_pref)) )
  {
    g_free(clean_app_id);
    return icon;
  }
  lower_app_id = g_ascii_strdown(clean_app_id, -1);
  icon = app_info_lookup_id(lower_app_id, symbolic_pref);
  g_free(lower_app_id);
  g_free(clean_app_id);

  return icon;
}
