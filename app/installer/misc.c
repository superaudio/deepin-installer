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

#include "misc.h"
#include "part_util.h"
#include <pwd.h>
#include <sys/types.h>

extern struct passwd* getpwent (void);
extern void endpwent (void);

void copy_file (const gchar *src, const gchar *dest)
{
  g_printf ("copy file\n");  

}

JS_EXPORT_API 
JSObjectRef installer_get_system_users()
{
    JSObjectRef array = json_array_create ();

    struct passwd *user;
    gchar *username = NULL;
    int i = 0;

    while ((user = getpwent ()) != NULL){
        username = g_strdup (user->pw_name);
        json_array_insert (array, i, jsvalue_from_cstr (get_global_context(), username));
        i++;
        g_free (username);
    }

    endpwent ();

    return array;
}

JS_EXPORT_API 
void installer_create_user (const gchar *username, const gchar *hostname, const gchar *password)
{
    g_printf ("create user\n");
}

JS_EXPORT_API 
void installer_reboot ()
{
    GError *error = NULL;
    GDBusProxy *ck_proxy = g_dbus_proxy_new_for_bus_sync(G_BUS_TYPE_SYSTEM,
            G_DBUS_PROXY_FLAGS_NONE,
            NULL,
            "org.freedesktop.ConsoleKit",
            "/org/freedesktop/ConsoleKit/Manager",
            "org.freedesktop.ConsoleKit.Manager",
            NULL,
            &error);

    g_assert (ck_proxy != NULL);
    if (error != NULL) {
        g_warning ("installer reboot: ck proxy %s\n", error->message);
        g_error_free (error);
    }

    GVariant *can_restart_var = g_dbus_proxy_call_sync (ck_proxy,
                                "CanRestart",
                                NULL,
                                G_DBUS_CALL_FLAGS_NONE,
                                -1,
                                NULL,
                                &error);

    g_assert (can_restart_var != NULL);
    if (error != NULL) {
        g_warning ("installer reboot: CanRestart %s\n", error->message);
        g_error_free (error);
    }

    gboolean can_restart = FALSE;
    g_variant_get (can_restart_var, "(b)", &can_restart);

    g_variant_unref (can_restart_var);

    if (can_restart) {
        g_dbus_proxy_call (ck_proxy,
                           "Restart",
                           NULL,
                           G_DBUS_CALL_FLAGS_NONE,
                           -1,
                           NULL,
                           NULL,
                           NULL);
    } else {
        g_warning ("installer reboot: can restart is false\n");
    }

    g_object_unref (ck_proxy);
}

void write_hostname (const gchar *hostname)
{
    g_printf ("write hostname\n");
}

void mount_procfs ()
{
    GError *error = NULL;
    gint status = -1;

    extern const gchar* target;
    if (target == NULL) {
        g_warning ("mount procfs:target is NULL\n");
        return ;
    }

    gchar *mount_dev = g_strdup_printf ("mount -v --bind /dev %s/dev", target);
    g_spawn_command_line_sync (mount_dev, NULL, NULL, &status, &error);
    if (error != NULL) {
        g_warning ("mount procfs:mount dev %s\n", error->message);
        g_error_free (error);
    }
    if (status != 0) {
        g_warning ("mount procfs:mount dev failed\n");
    }
    g_free (mount_dev);

    gchar *mount_devpts = g_strdup_printf ("mount -vt devpts devpts %s/dev/pts", target);
    g_spawn_command_line_sync (mount_devpts, NULL, NULL, &status, &error);
    if (error != NULL) {
        g_warning ("mount procfs:mount devpts %s\n", error->message);
        g_error_free (error);
    }
    if (status != 0) {
        g_warning ("mount procfs:mount devpts failed\n");
    }
    g_free (mount_devpts);


    gchar *mount_proc = g_strdup_printf ("mount -vt proc proc %s/proc", target);
    g_spawn_command_line_sync (mount_proc, NULL, NULL, &status, &error);
    if (error != NULL) {
        g_warning ("mount procfs:mount proc %s\n", error->message);
        g_error_free (error);
    }
    if (status != 0) {
        g_warning ("mount procfs:mount proc failed\n");
    }
    g_free (mount_proc);

    gchar *mount_sys = g_strdup_printf ("mount -vt sysfs sysfs %s/sys", target);
    g_spawn_command_line_sync (mount_sys, NULL, NULL, &status, &error);
    if (error != NULL) {
        g_warning ("mount procfs:mount sys %s\n", error->message);
        g_error_free (error);
    }
    if (status != 0) {
        g_warning ("mount procfs:mount sys failed\n");
    }
    g_free (mount_sys);

    //gchar *mount_shm = g_strdup_printf ("mount -vt tmpfs shm %s/dev/shm", target);
}
