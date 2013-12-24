/**
 * Copyright (c) 2011 ~ 2013 Deepin, Inc.
 *               2011 ~ 2013 Long Wei
 *
 * Author:      Long Wei <yilang2007lw@gmail.com>
 * Maintainer:  Long Wei <yilang2007lw@gamil.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 **/
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <signal.h>
#include "dwebview.h"
#include "i18n.h"
#include "fs_util.h"
#include "part_util.h"
#include "misc.h"

#define INSTALLER_HTML_PATH     "file://"RESOURCE_DIR"/installer/index.html"
#define INSTALLER_WIN_WIDTH     750
#define INSTALLER_WIN_HEIGHT    540

static GtkWidget *installer_container = NULL;

gboolean installer_is_running ()
{
    int server_sockfd;
    socklen_t server_len;
    struct sockaddr_un server_addr;

    server_addr.sun_path[0] = '\0'; //make it be an name unix socket
    int path_size = g_sprintf (server_addr.sun_path+1, "%s", "installer.app.deepin");
    server_addr.sun_family = AF_UNIX;
    server_len = 1 + path_size + offsetof(struct sockaddr_un, sun_path);

    server_sockfd = socket(AF_UNIX, SOCK_STREAM, 0);

    if (0 == bind(server_sockfd, (struct sockaddr *)&server_addr, server_len)) {
        return FALSE;
    } else {
        return TRUE;
    }
}

static gboolean
move_window (GtkWidget *widget, GdkEventButton *event, gpointer user_data)
{
    g_debug ("installer:move window");

    if (event->y > 90 || ((event->x > 506) && (event->x < 726) && (event->y > 60) && (90 > event->y)) || (event->x > 725) && (event->y < 30)) {
        g_debug ("move window:html click area");
        return TRUE;
    }

    if (event->button == 1) {
        g_debug ("move window:in drag x_root->%g, y_root->%g", event->x_root, event->y_root);

        gtk_widget_set_can_focus (widget, TRUE);
        gtk_widget_grab_focus (widget);

        gtk_window_begin_move_drag (GTK_WINDOW (widget), 
                                    event->button, 
                                    event->x_root,
                                    event->y_root,
                                    event->time);

    }

    return FALSE;
}

static void
move_window_center ()
{
    GdkScreen *screen = gtk_window_get_screen (GTK_WINDOW (installer_container));
    gint s_width = gdk_screen_get_width (screen);
    gint s_height = gdk_screen_get_height (screen);
    gint x = s_width > 1250 ? ((s_width - (750)) / 2) : 0;
    gint y = s_height > 540 ? ((s_height - 540) / 2) : 0;
    
    gtk_window_move (GTK_WINDOW (installer_container), x, y);
}

static void
adapt_location_for_help ()
{
    GdkScreen *screen = gtk_window_get_screen (GTK_WINDOW (installer_container));
    gint s_width = gdk_screen_get_width (screen);
    gint s_height = gdk_screen_get_height (screen);
    gint x = s_width > 1250 ? ((s_width - (750  + 500)) / 2) : 0;
    gint y = s_height > 540 ? ((s_height - 540) / 2) : 0;

    gtk_window_move (GTK_WINDOW (installer_container), x, y);
}

JS_EXPORT_API 
void installer_show_help ()
{
    GError *error = NULL;

    gchar *help_cmd = g_find_program_in_path ("installerhelp");
    if (help_cmd == NULL) {
        g_warning ("installerhelp not found\n");
        return ;
    }

    g_spawn_command_line_async (help_cmd, &error);
    if (error != NULL) {
        g_warning ("run installerhelp:%s\n", error->message);
        g_error_free (error);
    }
    g_free (help_cmd);

    adapt_location_for_help ();
}

JS_EXPORT_API 
void installer_hide_help ()
{
    GError *error = NULL;

    gchar *cmd = g_strdup ("pkill -9 installerhelp");
    g_spawn_command_line_async (cmd, &error);
    if (error != NULL) {
        g_warning ("hide installer help:%s\n", error->message);
        g_error_free (error);
    }
    g_free (cmd);
}

JS_EXPORT_API 
gboolean installer_is_help_running ()
{
    gboolean running = FALSE;
    GError *error = NULL;
    gchar *output = NULL;

    gchar *cmd = g_strdup ("sh -c \"ps aux |grep installerhelp\"");
    g_spawn_command_line_sync (cmd, &output, NULL, NULL, &error); 
    g_free (cmd);
    if (error != NULL) {
        g_warning ("is help running:%s\n", error->message);
        g_error_free (error);
    }
    gchar **items = g_strsplit (output, "\n", -1);
    if (g_strv_length (items) > 3) {
        running = TRUE;
    }
    g_strfreev (items);
    g_free (output);

    return running;
}

JS_EXPORT_API
void installer_finish_install ()
{
    installer_hide_help ();
    finish_install_cleanup ();
    gtk_main_quit ();
}

JS_EXPORT_API
void installer_finish_reboot ()
{
    installer_hide_help ();
    finish_install_cleanup ();
    //installer_reboot ();
    g_spawn_command_line_async ("reboot", NULL);
    gtk_main_quit ();
}

JS_EXPORT_API
void installer_emit_webview_ok ()
{
    static gboolean inited = FALSE;
    if (!inited) {
        inited = TRUE;
        init_parted ();
        inhibit_disk ();
    }
}

static void
sigterm_cb (int sig)
{
    installer_finish_install ();
}

int main(int argc, char **argv)
{
    gtk_init (&argc, &argv);

    setlocale(LC_MESSAGES, "");
    textdomain("INSTALLER");

    if (geteuid () != 0) {
        g_warning ("must run installer as root\n");
        exit (0);
    }

    if (installer_is_running ()) {
        g_warning ("another instance of installer is running\n");
        exit (0);
    }

    signal (SIGTERM, sigterm_cb);
    signal (SIGINT, sigterm_cb);
    signal (SIGQUIT, sigterm_cb);
    signal (SIGKILL, sigterm_cb);
    signal (SIGTSTP, sigterm_cb);

    installer_container = create_web_container (FALSE, TRUE);

    gtk_window_set_decorated (GTK_WINDOW (installer_container), FALSE);
    //gtk_window_set_skip_taskbar_hint (GTK_WINDOW (installer_container), TRUE);
    gtk_window_set_skip_pager_hint (GTK_WINDOW (installer_container), TRUE);

    GtkWidget *webview = d_webview_new_with_uri (INSTALLER_HTML_PATH);

    g_signal_connect (installer_container, "button-press-event", G_CALLBACK (move_window), NULL);
    gtk_container_add (GTK_CONTAINER (installer_container), GTK_WIDGET (webview));
    gtk_window_set_default_size (GTK_WINDOW (installer_container), INSTALLER_WIN_WIDTH, INSTALLER_WIN_HEIGHT);
    gtk_window_set_resizable (GTK_WINDOW (installer_container), FALSE);
    //gtk_window_set_position (GTK_WINDOW (installer_container), GTK_WIN_POS_CENTER);
    GdkGeometry geometry;
    geometry.min_width = INSTALLER_WIN_WIDTH;
    geometry.max_width = INSTALLER_WIN_WIDTH;
    geometry.base_width = INSTALLER_WIN_WIDTH;
    geometry.min_height = INSTALLER_WIN_HEIGHT;
    geometry.max_height = INSTALLER_WIN_HEIGHT;
    geometry.base_height = INSTALLER_WIN_HEIGHT;

    gtk_window_set_geometry_hints (GTK_WINDOW (installer_container), webview, &geometry, GDK_HINT_MIN_SIZE | GDK_HINT_MAX_SIZE | GDK_HINT_BASE_SIZE);
    move_window_center ();
    gtk_widget_set_opacity (GTK_WIDGET (installer_container), 0);
    gtk_widget_realize (installer_container);
    gtk_widget_show_all (installer_container);

    monitor_resource_file ("installer", webview);
    gtk_main ();

    return 0;
}
