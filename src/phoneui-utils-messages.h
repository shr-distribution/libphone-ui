/*
 *  Copyright (C) 2009, 2010
 *      Authors (alphabetical) :
 *		Klaus 'mrmoku' Kurzmann <mok@fluxnetz.de>
 *		Marco Trevisan (Treviño) <mail@3v1n0.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 */

#ifndef _PHONEUI_UTILS_MESSAGES_H
#define _PHONEUI_UTILS_MESSAGES_H

#include <glib.h>

int phoneui_utils_message_add(GHashTable *message, void (*callback)(GError *, char *msgpath, gpointer), gpointer data);
int phoneui_utils_message_add_fields(const char *direction, long timestamp, const char *content, const char *source, gboolean is_new, const char *peer, void (*callback)(GError *, char *msgpath, gpointer), gpointer data);
int phoneui_utils_message_update(const char *path, GHashTable *message, void (*callback) (GError *, gpointer), gpointer data);
int phoneui_utils_message_update_fields(const char *path, const char *direction, long timestamp, const char *content, const char *source, gboolean is_new, const char *peer, void (*callback) (GError *, gpointer), gpointer data);

int phoneui_utils_message_delete(const char *message_path, void (*callback)(GError *, gpointer), gpointer data);
int phoneui_utils_message_set_new_status(const char *path, gboolean is_new, void (*callback) (GError *, gpointer), gpointer data);
int phoneui_utils_message_set_read_status(const char *path, int read, void (*callback) (GError *, gpointer), gpointer data);
int phoneui_utils_message_set_sent_status(const char *path, int sent, void (*callback) (GError *, gpointer), gpointer data);
int phoneui_utils_message_get(const char *message_path, void (*callback)(GError *, GHashTable *, gpointer), gpointer data);

void phoneui_utils_messages_query(const char *sortby, gboolean sortdesc, gboolean disjunction, int limit_start, int limit, gboolean resolve_number, const GHashTable *options, void (*callback)(GError *, GHashTable **, int, gpointer), gpointer data);
void phoneui_utils_messages_query_full(const char *sortby, gboolean sortdesc, gboolean disjunction, int limit_start, int limit, gboolean resolve_number, const char *direction, long timestamp, const char *content, const char *source, gboolean is_new, const char *peer, void (*callback)(GError *, GHashTable **, int, gpointer), gpointer data);

void phoneui_utils_messages_get_full(const char *sortby, gboolean sortdesc, int limit_start, int limit, gboolean resolve_number, const char *direction, void (*callback)(GError *, GHashTable **, int, gpointer), gpointer data);
void phoneui_utils_messages_get(void (*callback) (GError *, GHashTable **, int, void *), gpointer data);

#endif
