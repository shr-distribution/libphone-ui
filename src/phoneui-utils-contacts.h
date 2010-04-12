#ifndef _PHONEUI_UTILS_CONTACTS_H
#define _PHONEUI_UTILS_CONTACTS_H

#include <glib.h>

void phoneui_utils_contacts_get(int *count, void (*callback)(gpointer , gpointer), gpointer data);
void phoneui_utils_contacts_field_type_get(const char *name, void (*callback)(GError *, char *, gpointer), gpointer user_data);
void phoneui_utils_contacts_fields_get(void (*callback)(GError *, GHashTable *, gpointer), gpointer data);
void phoneui_utils_contacts_fields_get_with_type(const char *type, void (*callback)(GError *, char **, int, gpointer), gpointer data);
void phoneui_utils_contacts_field_add(const char *name, const char *type, void (*callback)(GError *, gpointer), gpointer data);
void phoneui_utils_contacts_field_remove(const char *name, void (*callback)(GError *, gpointer), gpointer data);


int phoneui_utils_contact_lookup(const char *_number, void (*_callback) (GError *, GHashTable *, gpointer), gpointer data);
int phoneui_utils_contact_delete(const char *path, void (*name_callback) (GError *, gpointer), gpointer data);
int phoneui_utils_contact_update(const char *path, GHashTable *contact_data, void (*callback)(GError *, gpointer), gpointer data);
int phoneui_utils_contact_add(GHashTable *contact_data, void (*callback)(GError*, char *, gpointer), gpointer data);
int phoneui_utils_contact_get(const char *contact_path, void (*callback)(GError *, GHashTable*, gpointer), gpointer data);
void phoneui_utils_contact_get_fields_for_type(const char *contact_path, const char *type, void (*callback)(GError *, GHashTable *, gpointer), gpointer data);

char *phoneui_utils_contact_display_phone_get(GHashTable *properties);
char *phoneui_utils_contact_display_name_get(GHashTable *properties);
int phoneui_utils_contact_compare(GHashTable *contact1, GHashTable *contact2);




#endif