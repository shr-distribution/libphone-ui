
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <glib-object.h>
#include <freesmartphone.h>
#include <fsoframework.h>
#include <frameworkd-glib/opimd/frameworkd-glib-opimd-dbus.h>
#include <frameworkd-glib/opimd/frameworkd-glib-opimd-contacts.h>
#include "helpers.h"
#include "dbus.h"
#include "phoneui-utils-contacts.h"


struct _query_list_pack {
	gpointer data;
	int *count;
	void (*callback)(gpointer, gpointer);
	DBusGProxy *query;
// 	FreeSmartphonePIMContactQuery *query;
// 	FreeSmartphonePIMContacts *contacts;
};

struct _field_type_pack {
	gpointer data;
	void (*callback)(GError *, char *, gpointer);
	FreeSmartphonePIMFields *fields;
};

struct _fields_pack {
	gpointer data;
	void (*callback)(GError *, GHashTable *, gpointer);
	FreeSmartphonePIMFields *fields;
};

struct _fields_with_type_pack {
	gpointer data;
	void (*callback)(GError *, char **, int, gpointer);
	FreeSmartphonePIMFields *fields;
};

struct _field_pack {
	gpointer data;
	void (*callback)(GError *, gpointer);
	FreeSmartphonePIMFields *fields;
};

struct _contact_lookup_pack {
	gpointer *data;
	void (*callback)(GError *, GHashTable *, gpointer);
	char *number;
	FreeSmartphonePIMContacts *contacts;
};

struct _contact_add_pack {
	FreeSmartphonePIMContacts *contacts;
	void (*callback)(GError *, char *, gpointer);
	gpointer data;
};

struct _contact_delete_pack {
	FreeSmartphonePIMContact *contact;
	void (*callback)(GError *, gpointer);
	gpointer data;
};

struct _contact_pack {
	FreeSmartphonePIMContact *contact;
	void (*callback)(GError *, gpointer);
	gpointer data;
};

struct _contact_get_pack {
	FreeSmartphonePIMContact *contact;
	gpointer data;
	void (*callback)(GError *, GHashTable *, gpointer);
};

static int _compare_func(gconstpointer a, gconstpointer b)
{
	return phoneui_utils_contact_compare(*((GHashTable **)a), *((GHashTable **)b));
}

static void
_result_callback(GError *error, GPtrArray *contacts, gpointer data)
{
	struct _query_list_pack *pack = data;

	if (error || !contacts)
		return;
	g_ptr_array_sort(contacts, _compare_func);
	g_ptr_array_foreach(contacts, pack->callback, pack->data);
	opimd_contact_query_dispose(pack->query, NULL, NULL);
}

static void
_query_count_callback(GError *error, int count, gpointer data)
{
	struct _query_list_pack *pack = data;

	if (error) {
		return;
	}

	*pack->count = count;
	opimd_contact_query_get_multiple_results
				(pack->query, count, _result_callback, pack);
}
static void
_query_callback(GError *error, char *path, gpointer data)
{
	struct _query_list_pack *pack = data;

	if (error) {
		return;
	}

	pack->query = dbus_connect_to_opimd_contact_query(path);
	opimd_contact_query_get_result_count
			(pack->query, _query_count_callback, pack);
}

void
phoneui_utils_contacts_get(int *count,
			   void (*callback)(gpointer, gpointer),
			   gpointer userdata)
{
	GHashTable *qry;
	struct _query_list_pack *pack;

	pack = malloc(sizeof(*pack));
	pack->callback = callback;
	pack->data = userdata;
	pack->count = count;
	qry = g_hash_table_new_full
			(g_str_hash, g_str_equal, NULL, _helpers_free_gvalue);

	opimd_contacts_query(qry, _query_callback, pack);
}

static void
_contacts_field_type_callback(GObject *source, GAsyncResult *res, gpointer data)
{
	(void) source;
	(void) res;
	GError *error = NULL;
	char *type = NULL;
	struct _field_type_pack *pack = data;

	// FIXME !!!!!!!!!!!!!!!!!!!!1
/*	type = free_smartphone_pim_fields_get_type_finish
					(pack->fields, res, &error);*/
	if (pack->callback) {
		pack->callback(error, type, pack->data);
	}
	if (type) {
		free(type);
	}
	if (error) {
		g_error_free(error);
	}
	g_object_unref(pack->fields);
	free(pack);
}

void
phoneui_utils_contacts_field_type_get(const char *name,
                void (*callback)(GError *, char *, gpointer), gpointer data)
{
	struct _field_type_pack *pack;

	pack = malloc(sizeof(*pack));
	pack->callback = callback;
	pack->data = data;
	pack->fields = free_smartphone_pim_get_fields_proxy(_dbus(),
					FSO_FRAMEWORK_PIM_ServiceDBusName,
					FSO_FRAMEWORK_PIM_ContactsServicePath);
	free_smartphone_pim_fields_get_type_(pack->fields, name,
					     _contacts_field_type_callback, pack);
}

static void
_fields_strip_system_fields(GHashTable *fields)
{
	/* Remove the system fields */
	g_hash_table_remove(fields, "Path");
}

static void
_fields_get_callback(GObject *source, GAsyncResult *res, gpointer data)
{
	(void) source;
	GError *error = NULL;
	GHashTable *fields;
	struct _fields_pack *pack = data;

	g_debug("_fields_get_callback");
	fields = free_smartphone_pim_fields_list_fields_finish
						(pack->fields, res, &error);
	if (pack->callback) {
		if (fields) {
			g_debug("Stripping system fields");
			_fields_strip_system_fields(fields);
		}
		g_debug("Calling callback");
		pack->callback(error, fields, pack->data);
	}
	if (fields) {
		g_hash_table_unref(fields);
	}
	if (error) {
		g_error_free(error);
	}
	g_debug("Freeing stuff");
	g_object_unref(pack->fields);
	free(pack);
	g_debug("_fields_get_callback done");
}

void
phoneui_utils_contacts_fields_get(void (*callback)(GError *, GHashTable *, gpointer),
				  gpointer data)
{
	struct _fields_pack *pack;

	g_debug("phoneui_utils_contacts_fields_get START");

	pack = malloc(sizeof(*pack));
	pack->callback = callback;
	pack->data = data;
	pack->fields = free_smartphone_pim_get_fields_proxy(_dbus(),
					FSO_FRAMEWORK_PIM_ServiceDBusName,
					FSO_FRAMEWORK_PIM_ContactsServicePath);
	free_smartphone_pim_fields_list_fields(pack->fields,
					       _fields_get_callback, pack);
	g_debug("phoneui_utils_contacts_fields_get DONE");
}

static void
_fields_get_with_type_cb(GObject *source, GAsyncResult *res, gpointer data)
{
	(void) source;
	GError *error = NULL;
	char **fields;
	int count;
	struct _fields_with_type_pack *pack = data;

	fields = free_smartphone_pim_fields_list_fields_with_type_finish
					(pack->fields, res, &count, &error);
	if (pack->callback) {
		pack->callback(error, fields, count, pack->data);
	}
	if (error) {
		g_error_free(error);
	}
	// FIXME: free fields here?
	g_object_unref(pack->fields);
	free(pack);
}

void
phoneui_utils_contacts_fields_get_with_type(const char *type,
		void (*callback)(GError *, char **, int, gpointer), gpointer data)
{
	struct _fields_with_type_pack *pack =
			malloc(sizeof(struct _fields_with_type_pack));
	pack->data = data;
	pack->callback = callback;
	pack->fields = free_smartphone_pim_get_fields_proxy(_dbus(),
					FSO_FRAMEWORK_PIM_ServiceDBusName,
					FSO_FRAMEWORK_PIM_ContactsServicePath);
	free_smartphone_pim_fields_list_fields_with_type(pack->fields, type,
						_fields_get_with_type_cb, pack);
}

static void
_field_add_callback(GObject *source, GAsyncResult *res, gpointer data)
{
	(void) source;
	GError *error = NULL;
	struct _field_pack *pack = data;

	free_smartphone_pim_fields_add_field_finish(pack->fields, res, &error);
	if (pack->callback) {
		pack->callback(error, pack->data);
	}
	g_object_unref(pack->fields);
	free(pack);
}

void
phoneui_utils_contacts_field_add(const char *name, const char *type,
			void (*callback)(GError *, gpointer), void *data)
{
	struct _field_pack *pack;

	pack = malloc(sizeof(*pack));
	pack->callback = callback;
	pack->data = data;
	pack->fields = free_smartphone_pim_get_fields_proxy(_dbus(),
					FSO_FRAMEWORK_PIM_ServiceDBusName,
					FSO_FRAMEWORK_PIM_ContactsServicePath);

	free_smartphone_pim_fields_add_field(pack->fields, name, type, _field_add_callback, pack);
}

static void
_field_remove_callback(GObject *source, GAsyncResult *res, gpointer data)
{
	(void) source;
	GError *error = NULL;
	struct _field_pack *pack = data;

	free_smartphone_pim_fields_delete_field_finish(pack->fields, res, &error);
	if (pack->callback) {
		pack->callback(error, pack->data);
	}
	g_object_unref(pack->fields);
	free(pack);
}

void
phoneui_utils_contacts_field_remove(const char *name,
				    void (*callback)(GError *, gpointer),
				    gpointer data)
{
	struct _field_pack *pack;

	pack = malloc(sizeof(*pack));
	pack->callback = callback;
	pack->data = data;
	pack->fields = free_smartphone_pim_get_fields_proxy(_dbus(),
					FSO_FRAMEWORK_PIM_ServiceDBusName,
					FSO_FRAMEWORK_PIM_ContactsServicePath);
	free_smartphone_pim_fields_delete_field(pack->fields, name,
						_field_remove_callback, pack);
}


static void
_contact_lookup_callback(GObject *source, GAsyncResult *res, gpointer data)
{
	(void) source;
	GError *error = NULL;
	char *path = NULL;
	struct _contact_lookup_pack *pack = data;
	g_debug("_contact_lookup_callback");
	path = free_smartphone_pim_contacts_get_single_entry_single_field_finish
					(pack->contacts, res, &error);
        g_debug("got path %s", path);
	if (error || !path || !*path) {
		pack->callback(error, NULL, pack->data);
		if (error) {
			g_warning("error: (%d) %s", error->code, error->message);
			g_error_free(error);
		}
	}
	else {
		g_debug("Found contact: %s", path);
		phoneui_utils_contact_get(path, pack->callback, pack->data);
	}
	if (path) {
		free(path);
	}
	free(pack->number);
	g_object_unref(pack->contacts);
	free(pack);
}

static void
_contact_lookup_type_callback(GError *error, char **fields,
			      int count, gpointer data)
{
	/*FIXME: should I clean fields? */
	GHashTable *query;
	int i;
	struct _contact_lookup_pack *pack = data;

	if (error || !fields || !*fields) {
		pack->callback(error, NULL, pack->data);
		return;
	}

	query = g_hash_table_new_full(g_str_hash, g_str_equal,
				      free, _helpers_free_gvalue);

	GValue *value = _helpers_new_gvalue_string(pack->number);
	if (!value) {
		g_hash_table_destroy(query);
		// FIXME: create a nice error and pass it
		pack->callback(NULL, NULL, pack->data);
		free(pack->number);
		free(pack);
		return;
	}
	GValue *tmp = _helpers_new_gvalue_boolean(TRUE);
	if (!tmp) {
		_helpers_free_gvalue(value);
		g_hash_table_destroy(query);
		// FIXME: create a nice error and pass it
		pack->callback(NULL, NULL, pack->data);
		free(pack->number);
		free(pack);
		return;
	}

	g_hash_table_insert(query, strdup("_at_least_one"), tmp);

	for (i = 0; i < count; i++) {
		g_debug("\tTrying field: \"%s\"", fields[i]);
		g_hash_table_insert(query, strdup(fields[i]), value);
	}
	g_debug("All fields collected");
	pack->contacts = free_smartphone_pim_get_contacts_proxy(_dbus(),
					FSO_FRAMEWORK_PIM_ServiceDBusName,
					FSO_FRAMEWORK_PIM_ContactsServicePath);
        free_smartphone_pim_contacts_get_single_entry_single_field
		(pack->contacts, g_hash_table_ref(query), "Path",
		 _contact_lookup_callback, pack);

 	g_hash_table_unref(query);
}

int
phoneui_utils_contact_lookup(const char *number,
			void (*callback)(GError *, GHashTable *, gpointer),
			gpointer data)
{
	struct _contact_lookup_pack *pack;

	/* makes no sense to continue without callback */
	if (!callback) {
		g_message("phoneui_utils_contact_lookup without callback?");
		return 1;
	}
	pack = malloc(sizeof(*pack));
	pack->data = data;
	pack->callback = callback;
	pack->number = strdup(number);
	phoneui_utils_contacts_fields_get_with_type("phonenumber",
					_contact_lookup_type_callback, pack);

	return 0;
}

static void
_contact_delete_callback(GObject *source, GAsyncResult *res, gpointer data)
{
	(void) source;
	GError *error = NULL;
	struct _contact_pack *pack = data;
	free_smartphone_pim_contact_delete_finish(pack->contact, res, &error);
	if (pack->callback) {
		pack->callback(error, pack->data);
	}
	if (error) {
		g_error_free(error);
	}
	g_object_unref(pack->contact);
	free(pack);
}

int
phoneui_utils_contact_delete(const char *path,
				void (*callback) (GError *, gpointer),
				void *data)
{
	struct _contact_pack *pack;

	pack = malloc(sizeof(*pack));
	pack->callback = callback;
	pack->data = data;
	pack->contact = free_smartphone_pim_get_contact_proxy(_dbus(),
				FSO_FRAMEWORK_PIM_ServiceDBusName, path);
	free_smartphone_pim_contact_delete(pack->contact,
					   _contact_delete_callback, pack);
	return 0;
}


int
phoneui_utils_contact_update(const char *path,
			     GHashTable *contact_data,
			     void (*callback)(GError *, gpointer),
			     void* data)
{
	opimd_contact_update(path, contact_data, callback, data);
	return 0;
}

int
phoneui_utils_contact_add(GHashTable *contact_data,
			  void (*callback)(GError*, char *, gpointer),
			  void* data)
{
	opimd_contacts_add(contact_data, callback, data);
	return 0;
}


static void
_contact_get_callback(GError *error, GHashTable *contact, gpointer data)
{
	struct _contact_get_pack *pack = data;

	if (pack->callback) {
		pack->callback(error, contact, pack->data);
	}

	free(pack);
}

int
phoneui_utils_contact_get(const char *contact_path,
			  void (*callback)(GError *, GHashTable *, gpointer),
			  gpointer data)
{
	struct _contact_get_pack *pack;

	pack = malloc(sizeof(*pack));
	pack->callback = callback;
	pack->data = data;

	opimd_contact_get_content(contact_path, _contact_get_callback, pack);
	return 0;
}

void
phoneui_utils_contact_get_fields_for_type(const char* contact_path,
					  const char* type,
					  void (*callback)(GError *, GHashTable *, gpointer),
					  void *data)
{
	struct _contact_get_pack *pack;

	char *_type = calloc(sizeof(char), strlen(type)+2);
	_type[0] = '$';
	strcat(_type, type);
	pack = malloc(sizeof(*pack));
	pack->data = data;
	pack->callback = callback;
	opimd_contact_get_multiple_fields(contact_path, _type,
					  _contact_get_callback, pack);
	free(_type);
}

char *
phoneui_utils_contact_display_phone_get(GHashTable *properties)
{
	const char *phone = NULL;
	gpointer _key, _val;
	GHashTableIter iter;

	g_hash_table_iter_init(&iter, properties);
	while (g_hash_table_iter_next(&iter, &_key, &_val)) {
		const char *key = (const char *)_key;
		const GValue *val = (const GValue *) _val;

		if (!val) {
			g_debug("  hmm... field has no value?");
			continue;
		}

		/* Use the types mechanism */
		/* sanitize phone numbers */
		if (strstr(key, "Phone") || strstr(key, "phone")) {
			const char *s_val;
			char **strv;
			if (!G_IS_VALUE(_val)) {
				g_debug("it's NOT a gvalue!!!");
				continue;
			}
			if (G_VALUE_HOLDS_BOXED(val)) {
				strv = (char **)g_value_get_boxed(val);
				s_val = strv[0];
			}
			else if (G_VALUE_HOLDS_STRING(val)) {
				s_val = g_value_get_string(val);
			}
			else {
				g_debug("Value for field %s is neither string nor boxed :(", key);
				continue;
			}

			/* if key is exactly 'Phone' we want that is default
			 * phone number for this contact */
			if (strcmp(key, "Phone")) {
				phone = s_val;
			}
			/* otherwise we take it as default if it is the
			 * first phone number we see ... */
			else if (!phone) {
				phone = s_val;
			}
		}
	}

	return (phone) ? strdup(phone) : NULL;
}

char *
phoneui_utils_contact_display_name_get(GHashTable *properties)
{
	gpointer _key, _val;
	const char *name = NULL, *surname = NULL;
	const char *middlename = NULL, *nickname = NULL;
	const char *affiliation = NULL;
	char *displayname = NULL;
	GHashTableIter iter;

	g_hash_table_iter_init(&iter, properties);
	while (g_hash_table_iter_next(&iter, &_key, &_val)) {
		const char *key = (const char *)_key;
		const GValue *val = (const GValue *) _val;

		if (!val) {
			g_debug("  hmm... field has no value?");
			continue;
		}

		if (!strcmp(key, "Name")) {
			const char *s_val = g_value_get_string(val);
			name = s_val;
		}
		else if (!strcmp(key, "Surname")) {
			const char *s_val = g_value_get_string(val);
			surname = s_val;
		}
		else if (!strcmp(key, "Middlename")) {
			const char *s_val = g_value_get_string(val);
			middlename = s_val;
		}
		else if (!strcmp(key, "Nickname")) {
			const char *s_val = g_value_get_string(val);
			nickname = s_val;
		}
		else if (!strcmp(key, "Affiliation")) {
			affiliation = g_value_get_string(val);
		}
	}

	/* construct some sane display name from the fields */
	if (name && nickname && surname && affiliation) {
		displayname = g_strdup_printf("%s '%s' %s (%s)",
				name, nickname, surname, affiliation);
	}
	else if (name && nickname && surname) {
		displayname = g_strdup_printf("%s '%s' %s",
				name, nickname, surname);
	}
	else if (name && middlename && surname && affiliation) {
		displayname = g_strdup_printf("%s %s %s (%s)",
				name, middlename, surname, affiliation);
	}
	else if (name && middlename && surname) {
		displayname = g_strdup_printf("%s %s %s",
				name, middlename, surname);
	}
	else if (name && surname && affiliation) {
		displayname = g_strdup_printf("%s %s (%s)",
				name, surname, affiliation);
	}
	else if (name && surname) {
		displayname = g_strdup_printf("%s %s",
				name, surname);
	}
	else if (nickname && affiliation) {
		displayname = g_strdup_printf("%s (%s)", nickname, affiliation);
	}
	else if (nickname) {
		displayname = g_strdup(nickname);
	}
	else if (name && affiliation) {
		displayname = g_strdup_printf("%s (%s)", name, affiliation);
	}
	else if (name) {
		displayname = g_strdup(name);
	}
	else if (surname && affiliation) {
		displayname = g_strdup_printf("%s (%s)", surname, affiliation);
	}
	else if (surname) {
		displayname = g_strdup(surname);
	}
	else if (affiliation) {
		displayname = g_strdup(affiliation);
	}

	return displayname;
}

int
phoneui_utils_contact_compare(GHashTable *contact1, GHashTable *contact2)
{
	int ret;
	char *name1 = phoneui_utils_contact_display_name_get(contact1);
	if (!name1)
		return -1;
	char *name2 = phoneui_utils_contact_display_name_get(contact2);
	if (!name2) {
		free (name1);
		return 1;
	}
	ret = strcoll(name1, name2);
	free(name1);
	free(name2);
	return ret;
}

char *
phoneui_utils_contact_get_dbus_path(int entryid)
{
	/* 15 = 1 for '\0', 1 for '/' and 13 for max possible int size */
	int len = strlen(FSO_FRAMEWORK_PIM_ContactsServicePath) + 15;
	char *ret = calloc(sizeof(char), len);
	if (!ret) {
		return NULL;
	}
	snprintf(ret, len-1, "%s/%d", FSO_FRAMEWORK_PIM_ContactsServicePath, entryid);

	return ret;
}
