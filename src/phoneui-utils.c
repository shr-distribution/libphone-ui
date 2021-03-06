/*
 *  Copyright (C) 2009, 2010
 *      Authors (alphabetical) :
 *		Tom "TAsn" Hacohen <tom@stosb.com>
 *		Klaus 'mrmoku' Kurzmann <mok@fluxnetz.de>
 *		Marco Trevisan (Treviño) <mail@3v1n0.net>
 *		Thomas Zimmermann <zimmermann@vdm-design.de>
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


#include <glib.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-bindings.h>
#include <freesmartphone.h>
#include <fsoframework.h>
#include <frameworkd-glib-dbus.h>
#include <phone-utils.h>
#include "phoneui-utils.h"
#include "phoneui-utils-sound.h"
#include "phoneui-utils-device.h"
#include "phoneui-utils-feedback.h"
#include "phoneui-utils-contacts.h"
#include "phoneui-utils-messages.h"
#include "dbus.h"
#include "helpers.h"

#define PIM_DOMAIN_PROXY(func) (void *(*)(DBusGConnection*, const char*, const char*)) func
#define PIM_QUERY_FUNCTION(func) (void (*)(void *, GHashTable *, GAsyncReadyCallback, gpointer)) func
#define PIM_QUERY_RESULTS(func) (void (*)(void *, int, GAsyncReadyCallback, gpointer)) func
#define PIM_QUERY_RESULTS_FINISH(func) (GHashTable** (*)(void*, GAsyncResult*, int*, GError**)) func
#define PIM_QUERY_COUNT(func) (void (*)(void *, GAsyncReadyCallback, gpointer)) func
#define PIM_QUERY_PROXY(func) (void *(*)(DBusGConnection*, const char*, const char*)) func
#define PIM_QUERY_DISPOSE(func) (void (*)(void *query, GAsyncReadyCallback, gpointer)) func

struct _usage_pack {
	FreeSmartphoneUsage *usage;
	void (*callback)(GError *, gpointer);
	gpointer data;
};

struct _usage_policy_pack {
	FreeSmartphoneUsage *usage;
	void (*callback)(GError *, FreeSmartphoneUsageResourcePolicy, gpointer);
	gpointer data;
};

struct _idle_pack {
	FreeSmartphoneDeviceIdleNotifier *idle;
	void (*callback)(GError *, gpointer);
	gpointer data;
};

struct _sms_send_pack {
	FreeSmartphoneGSMSMS *sms;
	FreeSmartphonePIMMessages *pim_messages;
	char *number;
	const char *message;
	char *pim_path;
	void (*callback)(GError *, int, const char *, gpointer);
	gpointer data;
};

struct _query_pack {
	enum PhoneUiPimDomain domain_type;
	void *query;
	void *domain;
	void (*callback)(GError *, GHashTable **, int, gpointer);
	gpointer data;
};

struct _call_get_pack {
	FreeSmartphonePIMCall *call;
	void (*callback)(GError *, GHashTable *, gpointer);
	gpointer data;
};
struct _pdp_pack {
	FreeSmartphoneGSMPDP *pdp;
	void (*callback)(GError *, gpointer);
	gpointer data;
};
struct _pdp_credentials_pack {
	FreeSmartphoneGSMPDP *pdp;
	void (*callback)(GError *, const char *, const char *, const char *, gpointer);
	gpointer data;
};
struct _network_pack {
	FreeSmartphoneNetwork *network;
	void (*callback)(GError *, gpointer);
	gpointer data;
};
struct _get_offline_mode_pack {
	void (*callback)(GError *, gboolean, gpointer);
	gpointer data;
};
struct _set_offline_mode_pack {
	void (*callback)(GError *, gpointer);
	gpointer data;
};
struct _set_pin_pack {
	void (*callback)(GError *, gpointer);
	gpointer data;
};

int
phoneui_utils_init(GKeyFile *keyfile)
{
	int ret;
	ret = phoneui_utils_sound_init(keyfile);
	ret = phoneui_utils_device_init(keyfile);
	ret = phoneui_utils_feedback_init(keyfile);

	// FIXME: remove when vala learned to handle multi-field contacts !!!
	g_debug("Initing libframeworkd-glib :(");
	frameworkd_handler_connect(NULL);

	return 0;
}

void
phoneui_utils_deinit()
{
	/*FIXME: stub*/
	phoneui_utils_sound_deinit();
}

static void
_pim_query_results_callback(GObject *source, GAsyncResult *res, gpointer data)
{
	(void) source;
	int count;
	GHashTable **results;
	GError *error = NULL;
	struct _query_pack *pack = data;
	void (*dispose_func)(void *query, GAsyncReadyCallback, gpointer);
	GHashTable** (*results_func)(void *query, GAsyncResult*, int* count, GError**);

	dispose_func = NULL;
	results_func = NULL;

	switch(pack->domain_type) {
		case PHONEUI_PIM_DOMAIN_CALLS:
			dispose_func = PIM_QUERY_DISPOSE(free_smartphone_pim_call_query_dispose_);
			results_func = PIM_QUERY_RESULTS_FINISH(
					free_smartphone_pim_call_query_get_multiple_results_finish);
			break;
		case PHONEUI_PIM_DOMAIN_CONTACTS:
			dispose_func = PIM_QUERY_DISPOSE(free_smartphone_pim_contact_query_dispose_);
			results_func = PIM_QUERY_RESULTS_FINISH(
				free_smartphone_pim_contact_query_get_multiple_results_finish);
			break;
/* FIXME, re-enable it when libfsoframework supports it again
		case PHONEUI_PIM_DOMAIN_DATES:
			dispose_func = PIM_QUERY_DISPOSE(free_smartphone_pim_date_query_dispose_);
			results_func = PIM_QUERY_RESULTS_FINISH(
				free_smartphone_pim_date_query_get_multiple_results_finish);
			break;
*/
		case PHONEUI_PIM_DOMAIN_MESSAGES:
			dispose_func = PIM_QUERY_DISPOSE(free_smartphone_pim_message_query_dispose_);
			results_func = PIM_QUERY_RESULTS_FINISH(
				free_smartphone_pim_message_query_get_multiple_results_finish);
			break;
		case PHONEUI_PIM_DOMAIN_NOTES:
			dispose_func = PIM_QUERY_DISPOSE(free_smartphone_pim_note_query_dispose_);
			results_func = PIM_QUERY_RESULTS_FINISH(
				free_smartphone_pim_note_query_get_multiple_results_finish);
			break;
/* FIXME, wait the async version in freesmartphone APIs
		case PHONEUI_PIM_DOMAIN_TASKS:
			dispose_func = PIM_QUERY_DISPOSE(free_smartphone_pim_task_query_dispose_);
			results_func = PIM_QUERY_RESULTS_FINISH(
				free_smartphone_pim_task_query_get_multiple_results_finish);
			break;
*/
		default:
			g_object_unref(pack->query);
			free(pack);
			return;
	}

	results = results_func(pack->query, res, &count, &error);
	dispose_func(pack->query, NULL, NULL);
	g_object_unref(pack->query);

	g_debug("Query gave %d entries", count);

	if (pack->callback) {
		pack->callback(error, results, count, pack->data);
	}

	if (error) {
		g_error_free(error);
	}

	free(pack);
}

static void
_pim_query_callback(GObject *source, GAsyncResult *res, gpointer data)
{
	(void) source;
	char *query_path = NULL;
	GError *error = NULL;
	void (*count_func)(void *query, GAsyncReadyCallback, gpointer);
	void (*results_func)(void *query, int count, GAsyncReadyCallback, gpointer);
	void *(*query_proxy)(DBusGConnection*, const char* busname, const char* path);
	struct _query_pack *pack = data;

	query_proxy = NULL;
	count_func = NULL;

	switch(pack->domain_type) {
		case PHONEUI_PIM_DOMAIN_CALLS:
			query_path = free_smartphone_pim_calls_query_finish
								(pack->domain, res, &error);
			break;
		case PHONEUI_PIM_DOMAIN_CONTACTS:
			query_path = free_smartphone_pim_contacts_query_finish
								(pack->domain, res, &error);
			break;
/* FIXME, re-enable it when libfsoframework supports it again
		case PHONEUI_PIM_DOMAIN_DATES:
			query_path = free_smartphone_pim_dates_query_finish
								(pack->domain, res, &error);
			break;
*/
		case PHONEUI_PIM_DOMAIN_MESSAGES:
			query_path = free_smartphone_pim_messages_query_finish
								(pack->domain, res, &error);
			break;
		case PHONEUI_PIM_DOMAIN_NOTES:
			query_path = free_smartphone_pim_notes_query_finish
								(pack->domain, res, &error);
			break;
/* FIXME, wait the async version in freesmartphone APIs
		case PHONEUI_PIM_DOMAIN_TASKS:
			query_path = free_smartphone_pim_tasks_query_finish
								(pack->domain, res, &error);
			break;
*/
		default:
			goto exit;
	}

	g_object_unref(pack->domain);

	if (error) {
		g_warning("Query error: (%d) %s",
			  error->code, error->message);
		pack->callback(error, NULL, 0, pack->data);
		g_error_free(error);
		goto exit;
	}

	switch(pack->domain_type) {
		case PHONEUI_PIM_DOMAIN_CALLS:
			query_proxy = PIM_QUERY_PROXY(free_smartphone_pim_get_call_query_proxy);
			count_func = PIM_QUERY_COUNT(free_smartphone_pim_call_query_get_result_count);
			results_func = PIM_QUERY_RESULTS
			               (free_smartphone_pim_call_query_get_multiple_results);
			break;
		case PHONEUI_PIM_DOMAIN_CONTACTS:
			query_proxy = PIM_QUERY_PROXY(free_smartphone_pim_get_contact_query_proxy);
			count_func = PIM_QUERY_COUNT(free_smartphone_pim_contact_query_get_result_count);
			results_func = PIM_QUERY_RESULTS
			               (free_smartphone_pim_contact_query_get_multiple_results);
			break;
/* FIXME, re-enable it when libfsoframework supports it again
		case PHONEUI_PIM_DOMAIN_DATES:
			query_proxy = PIM_QUERY_PROXY(free_smartphone_pim_get_date_query_proxy);
			count_func = PIM_QUERY_COUNT(free_smartphone_pim_date_query_get_result_count);
			results_func = PIM_QUERY_RESULTS
			               (free_smartphone_pim_date_query_get_multiple_results);
			break;
*/
		case PHONEUI_PIM_DOMAIN_MESSAGES:
			query_proxy = PIM_QUERY_PROXY(free_smartphone_pim_get_message_query_proxy);
			count_func = PIM_QUERY_COUNT(free_smartphone_pim_message_query_get_result_count);
			results_func = PIM_QUERY_RESULTS
			               (free_smartphone_pim_message_query_get_multiple_results);
			break;
		case PHONEUI_PIM_DOMAIN_NOTES:
			query_proxy = PIM_QUERY_PROXY(free_smartphone_pim_get_note_query_proxy);
			count_func = PIM_QUERY_COUNT(free_smartphone_pim_note_query_get_result_count);
			results_func = PIM_QUERY_RESULTS
			               (free_smartphone_pim_note_query_get_multiple_results);
			break;
/* FIXME, wait the async version in freesmartphone APIs
		case PHONEUI_PIM_DOMAIN_TASKS:
			query_proxy = PIM_QUERY_PROXY(free_smartphone_pim_get_task_query_proxy);
			count_func = PIM_QUERY_COUNT(free_smartphone_pim_task_query_get_result_count);
			results_func = PIM_QUERY_RESULTS
			               (free_smartphone_pim_task_query_get_multiple_results);
			break;
*/
		default:
			goto exit;
	}

	if (!query_proxy || !count_func || !query_path)
		goto exit;

	pack->query = query_proxy(_dbus(), FSO_FRAMEWORK_PIM_ServiceDBusName, query_path);
	results_func(pack->query, -1, _pim_query_results_callback, pack);
	free(query_path);
	return;

exit:
	if (query_path) free(query_path);
	free(pack);
}

static void
_pim_query_hashtable_clone_foreach_callback(void *k, void *v, void *data) {
	GHashTable *query = ((GHashTable *)data);
	char *key = (char *)k;
	GValue *value = (GValue *)v;
	GValue *new_value;

	if (key && key[0] != '_') {
		new_value = calloc(sizeof(GValue), 1);
		g_value_init(new_value, G_VALUE_TYPE(value));
		g_value_copy(value, new_value);
		g_hash_table_insert(query, strdup(key), new_value);
	}
}

void phoneui_utils_pim_query(enum PhoneUiPimDomain domain, const char *sortby,
	gboolean sortdesc, gboolean disjunction, int limit_start, int limit,
	gboolean resolve_number, const GHashTable *options,
	void (*callback)(GError *, GHashTable **, int, gpointer), gpointer data)
{
	struct _query_pack *pack;
	GHashTable *query;
	GValue *gval_tmp;
	const char *path;
	void *(*domain_get)(DBusGConnection*, const char* busname, const char* path);
	void (*query_function)(void *domain, GHashTable *query, GAsyncReadyCallback, gpointer);

	path = NULL;
	query_function = NULL;

	switch(domain) {
		case PHONEUI_PIM_DOMAIN_CALLS:
			path = FSO_FRAMEWORK_PIM_CallsServicePath;
			query_function = PIM_QUERY_FUNCTION(free_smartphone_pim_calls_query);
			domain_get = PIM_DOMAIN_PROXY(free_smartphone_pim_get_calls_proxy);
			break;
		case PHONEUI_PIM_DOMAIN_CONTACTS:
			path = FSO_FRAMEWORK_PIM_ContactsServicePath;
			query_function = PIM_QUERY_FUNCTION(free_smartphone_pim_contacts_query);
			domain_get = PIM_DOMAIN_PROXY(free_smartphone_pim_get_contacts_proxy);
			break;
/* FIXME, re-enable it when libfsoframework supports it again
		case PHONEUI_PIM_DOMAIN_DATES:
			path = FSO_FRAMEWORK_PIM_DatesServicePath;
			query_function = PIM_QUERY_FUNCTION(free_smartphone_pim_dates_query);
			domain_get = PIM_DOMAIN_PROXY(free_smartphone_pim_get_dates_proxy);
			break;
*/
		case PHONEUI_PIM_DOMAIN_MESSAGES:
			path = FSO_FRAMEWORK_PIM_MessagesServicePath;
			query_function = PIM_QUERY_FUNCTION(free_smartphone_pim_messages_query);
			domain_get = PIM_DOMAIN_PROXY(free_smartphone_pim_get_messages_proxy);
			break;
		case PHONEUI_PIM_DOMAIN_NOTES:
			path = FSO_FRAMEWORK_PIM_NotesServicePath;
			query_function = PIM_QUERY_FUNCTION(free_smartphone_pim_notes_query);
			domain_get = PIM_DOMAIN_PROXY(free_smartphone_pim_get_notes_proxy);
			break;
/* FIXME, add the sync version in freesmartphone APIs
		case PHONEUI_PIM_DOMAIN_TASKS:
			path = FSO_FRAMEWORK_PIM_TasksServicePath;
			query_function = PIM_QUERY_FUNCTION(free_smartphone_pim_tasks_query);
			domain_get = PIM_DOMAIN_PROXY(free_smartphone_pim_get_tasks_proxy);
			break;
*/
		default:
			return;
	}

	if (!path || !query_function)
		return;

	query = g_hash_table_new_full(g_str_hash, g_str_equal,
						   g_free, _helpers_free_gvalue);
	if (!query)
		return;

	if (sortby && strlen(sortby)) {
		gval_tmp = _helpers_new_gvalue_string(sortby);
		g_hash_table_insert(query, strdup("_sortby"), gval_tmp);
	}

	if (sortdesc) {
		gval_tmp = _helpers_new_gvalue_boolean(TRUE);
		g_hash_table_insert(query, strdup("_sortdesc"), gval_tmp);
	}

	if (disjunction) {
		gval_tmp = _helpers_new_gvalue_boolean(TRUE);
		g_hash_table_insert(query, strdup("_at_least_one"), gval_tmp);
	}

	if (resolve_number) {
		gval_tmp = _helpers_new_gvalue_boolean(TRUE);
		g_hash_table_insert(query, strdup("_resolve_phonenumber"), gval_tmp);
	}

	gval_tmp = _helpers_new_gvalue_int(limit_start);
	g_hash_table_insert(query, strdup("_limit_start"), gval_tmp);
	gval_tmp = _helpers_new_gvalue_int(limit);
	g_hash_table_insert(query, strdup("_limit"), gval_tmp);

	if (options) {
		g_hash_table_foreach((GHashTable *)options,
			_pim_query_hashtable_clone_foreach_callback, query);
	}

	pack = malloc(sizeof(*pack));
	pack->domain_type = domain;
	pack->callback = callback;
	pack->data = data;
	pack->domain = domain_get(_dbus(), FSO_FRAMEWORK_PIM_ServiceDBusName, path);
	pack->query = NULL;

	g_debug("Firing the query!");

	query_function(pack->domain, query, _pim_query_callback, pack);
	g_hash_table_unref(query);
}

static GHashTable *
_create_opimd_message(const char *number, const char *message)
{
	/* TODO: add timzone */

	GHashTable *message_opimd =
		g_hash_table_new_full(g_str_hash, g_str_equal,
				      NULL, _helpers_free_gvalue);
	GValue *tmp;

	tmp = _helpers_new_gvalue_string(number);
	g_hash_table_insert(message_opimd, "Peer", tmp);

	tmp = _helpers_new_gvalue_string("out");
	g_hash_table_insert(message_opimd, "Direction", tmp);

	tmp = _helpers_new_gvalue_string("SMS");
	g_hash_table_insert(message_opimd, "Source", tmp);

	tmp = _helpers_new_gvalue_string(message);
	g_hash_table_insert(message_opimd, "Content", tmp);

	tmp = _helpers_new_gvalue_boolean(TRUE);
	g_hash_table_insert(message_opimd, "New", tmp);

	tmp = _helpers_new_gvalue_int(time(NULL));
	g_hash_table_insert(message_opimd, "Timestamp", tmp);

	return message_opimd;
}

static void
_sms_send_callback(GObject *source, GAsyncResult *res, gpointer data)
{
	(void) source;
	GError *error = NULL;
	char *timestamp = NULL;
	struct _sms_send_pack *pack = data;
	int reference;

	free_smartphone_gsm_sms_send_text_message_finish(pack->sms, res,
						&reference, &timestamp, &error);

	if (pack->pim_path) {
		phoneui_utils_message_set_sent_status(pack->pim_path, !error, NULL, NULL);
		free(pack->pim_path);
	}

	if (pack->callback) {
		pack->callback(error, reference, timestamp, pack->data);
	}
	if (timestamp) {
		free(timestamp);
	}
	if (error) {
		g_warning("Error %d sending message: %s\n", error->code, error->message);
		g_error_free(error);
	}
	g_object_unref(pack->pim_messages);
	g_object_unref(pack->sms);
	free(pack->number);
	free(pack);
}

static void
_opimd_message_added(GObject *source_object, GAsyncResult *res, gpointer user_data)
{
	(void)source_object;
	struct _sms_send_pack *pack = user_data;
	char *msg_path = NULL;
	GError *error = NULL;

	/*FIXME: ATM it just saves it as saved and tell the user everything
	 * is ok, even if it didn't save. We really need to fix that,
	 * we should verify if glib's callbacks work */

	msg_path = free_smartphone_pim_messages_add_finish(pack->pim_messages, res,
				&error);

	if (!error && msg_path)
		pack->pim_path = msg_path;
	else if (error) {
		g_warning("Error %d saving message: %s\n", error->code, error->message);
		g_error_free(error);
		/*TODO redirect the error to the send_text_message function, adding it
		 * to the pack; then do the proper callback. */
	}

	free_smartphone_gsm_sms_send_text_message(pack->sms, pack->number,
				pack->message, FALSE, _sms_send_callback, pack);
}

static void
_sms_message_send(struct _sms_send_pack *pack)
{
	if (pack->pim_messages) {
		GHashTable *message_opimd = _create_opimd_message(pack->number, pack->message);
		free_smartphone_pim_messages_add(pack->pim_messages, message_opimd,
					_opimd_message_added, pack);
		g_hash_table_unref(message_opimd);
	} else {
		free_smartphone_gsm_sms_send_text_message(pack->sms, pack->number,
				pack->message, FALSE, _sms_send_callback, pack);
	}
}

int
phoneui_utils_sms_send(const char *message, GPtrArray * recipients, void (*callback)
		(GError *, int transaction_index, const char *timestamp, gpointer),
		gpointer data)
{
	unsigned int i;
	struct _sms_send_pack *pack;
	FreeSmartphoneGSMSMS *sms;
	FreeSmartphonePIMMessages *pim_messages;
	GHashTable *properties;
	char *number;

	if (!recipients) {
		return 1;
	}

	sms = free_smartphone_gsm_get_s_m_s_proxy(_dbus(),
					FSO_FRAMEWORK_GSM_ServiceDBusName,
					FSO_FRAMEWORK_GSM_DeviceServicePath);
	pim_messages = free_smartphone_pim_get_messages_proxy(_dbus(),
					FSO_FRAMEWORK_PIM_ServiceDBusName,
					FSO_FRAMEWORK_PIM_MessagesServicePath);

	/* cycle through all the recipients */
	for (i = 0; i < recipients->len; i++) {
		properties = g_ptr_array_index(recipients, i);
		number = phoneui_utils_contact_display_phone_get(properties);
		phone_utils_remove_filler_chars(number);
		if (!number) {
			continue;
		}
		g_message("%d.\t%s", i + 1, number);
		pack = malloc(sizeof(*pack));
		pack->callback = callback;
		pack->data = data;
		pack->sms = g_object_ref(sms);
		pack->pim_messages = g_object_ref(pim_messages);
		pack->pim_path = NULL;
		pack->message = message;
		pack->number = number;
		_sms_message_send(pack);
	}
	g_object_unref(sms);
	g_object_unref(pim_messages);

	return 0;
}



int
phoneui_utils_resource_policy_set(enum PhoneUiResource resource,
					enum PhoneUiResourcePolicy policy)
{
	(void) policy;
	switch (resource) {
	case PHONEUI_RESOURCE_GSM:
		break;
	case PHONEUI_RESOURCE_BLUETOOTH:
		break;
	case PHONEUI_RESOURCE_WIFI:
		break;
	case PHONEUI_RESOURCE_DISPLAY:
		break;
	case PHONEUI_RESOURCE_CPU:
		break;
	default:
		return 1;
		break;
	}

	return 0;
}

enum PhoneUiResourcePolicy
phoneui_utils_resource_policy_get(enum PhoneUiResource resource)
{
	switch (resource) {
	case PHONEUI_RESOURCE_GSM:
		break;
	case PHONEUI_RESOURCE_BLUETOOTH:
		break;
	case PHONEUI_RESOURCE_WIFI:
		break;
	case PHONEUI_RESOURCE_DISPLAY:
		break;
	case PHONEUI_RESOURCE_CPU:
		break;
	default:
		break;
	}

	return PHONEUI_RESOURCE_POLICY_ERROR;
}

void
phoneui_utils_fields_types_get(void *callback, void *userdata)
{
	/*FIXME stub*/
	(void) callback;
	(void) userdata;
	return;
}

static void
_suspend_callback(GObject *source, GAsyncResult *res, gpointer data)
{
	(void) source;
	GError *error = NULL;
	struct _usage_pack *pack = data;

	free_smartphone_usage_suspend_finish(pack->usage, res, &error);
	if (pack->callback) {
		pack->callback(error, pack->data);
	}
	if (error) {
		g_error_free(error);
	}
	g_object_unref(pack->usage);
	free(pack);
}

void
phoneui_utils_usage_suspend(void (*callback) (GError *, gpointer), gpointer data)
{
	struct _usage_pack *pack;

	pack = malloc(sizeof(*pack));
	pack->callback = callback;
	pack->data = data;
	pack->usage = free_smartphone_get_usage_proxy(_dbus(),
					FSO_FRAMEWORK_USAGE_ServiceDBusName,
					FSO_FRAMEWORK_USAGE_ServicePathPrefix);
	free_smartphone_usage_suspend(pack->usage, _suspend_callback, pack);
}

static void
_shutdown_callback(GObject *source, GAsyncResult *res, gpointer data)
{
	(void) source;
	GError *error = NULL;
	struct _usage_pack *pack = data;

	free_smartphone_usage_shutdown_finish(pack->usage, res, &error);
	if (pack->callback) {
		pack->callback(error, pack->data);
	}
	if (error) {
		g_error_free(error);
	}
	g_object_unref(pack->usage);
	free(pack);
}

void
phoneui_utils_usage_shutdown(void (*callback) (GError *, gpointer), gpointer data)
{
	struct _usage_pack *pack;

	pack = malloc(sizeof(*pack));
	pack->callback = callback;
	pack->data = data;
	pack->usage = free_smartphone_get_usage_proxy(_dbus(),
					FSO_FRAMEWORK_USAGE_ServiceDBusName,
					FSO_FRAMEWORK_USAGE_ServicePathPrefix);
	free_smartphone_usage_shutdown(pack->usage, _shutdown_callback, pack);
}

static void
_set_idle_state_callback(GObject *source, GAsyncResult *res, gpointer data)
{
	(void) source;
	GError *error = NULL;
	struct _idle_pack *pack = data;

	free_smartphone_device_idle_notifier_set_state_finish
						(pack->idle, res, &error);
	if (pack->callback) {
		pack->callback(error, pack->data);
	}
	if (error) {
		g_error_free(error);
	}
	g_object_unref(pack->idle);
	free(pack);
}

void
phoneui_utils_idle_set_state(FreeSmartphoneDeviceIdleState state,
			     void (*callback) (GError *, gpointer),
			     gpointer data)
{
	struct _idle_pack *pack;

	pack = malloc(sizeof(*pack));
	pack->callback = callback;
	pack->data = data;
	pack->idle = free_smartphone_device_get_idle_notifier_proxy(_dbus(),
				FSO_FRAMEWORK_DEVICE_ServiceDBusName,
				FSO_FRAMEWORK_DEVICE_IdleNotifierServicePath"/0");
	free_smartphone_device_idle_notifier_set_state(pack->idle, state,
						_set_idle_state_callback, pack);
}

static void
_get_policy_callback(GObject *source, GAsyncResult *res, gpointer data)
{
	(void) source;
	GError *error = NULL;
	FreeSmartphoneUsageResourcePolicy policy;
	struct _usage_policy_pack *pack = data;

	policy = free_smartphone_usage_get_resource_policy_finish
						(pack->usage, res, &error);
	pack->callback(error, policy, pack->data);
	if (error) {
		g_error_free(error);
	}
	g_object_unref(pack->usage);
	free(pack);
}

void
phoneui_utils_resources_get_resource_policy(const char *name,
	void (*callback) (GError *, FreeSmartphoneUsageResourcePolicy, gpointer),
	gpointer data)
{
	struct _usage_policy_pack *pack;

	pack = malloc(sizeof(*pack));
	pack->callback = callback;
	pack->data = data;
	pack->usage = free_smartphone_get_usage_proxy(_dbus(),
					FSO_FRAMEWORK_USAGE_ServiceDBusName,
					FSO_FRAMEWORK_USAGE_ServicePathPrefix);
	free_smartphone_usage_get_resource_policy(pack->usage, name,
						  _get_policy_callback, pack);
}

static void
_set_policy_callback(GObject *source, GAsyncResult *res, gpointer data)
{
	(void) source;
	GError *error = NULL;
	struct _usage_pack *pack = data;

	free_smartphone_usage_set_resource_policy_finish
						(pack->usage, res, &error);
	if (pack->callback) {
		pack->callback(error, pack->data);
	}
	if (error) {
		g_error_free(error);
	}
	g_object_unref(pack->usage);
	free(pack);
}

void
phoneui_utils_resources_set_resource_policy(const char *name,
			FreeSmartphoneUsageResourcePolicy policy,
			void (*callback) (GError *, gpointer), gpointer data)
{
	struct _usage_pack *pack;

	pack = malloc(sizeof(*pack));
	pack->callback = callback;
	pack->data = data;
	pack->usage = free_smartphone_get_usage_proxy(_dbus(),
					FSO_FRAMEWORK_USAGE_ServiceDBusName,
					FSO_FRAMEWORK_USAGE_ServicePathPrefix);
	free_smartphone_usage_set_resource_policy(pack->usage, name, policy,
						  _set_policy_callback, pack);
}

void
phoneui_utils_calls_query(const char *sortby, gboolean sortdesc,
			gboolean disjunction, int limit_start, int limit,
			gboolean resolve_number, const GHashTable *options,
			void (*callback)(GError *, GHashTable **, int, gpointer),
			gpointer data)
{
	phoneui_utils_pim_query(PHONEUI_PIM_DOMAIN_CALLS, sortby, sortdesc, disjunction,
				limit_start, limit, resolve_number, options, callback, data);
}

void
phoneui_utils_calls_get_full(const char *sortby, gboolean sortdesc,
			int limit_start, int limit, gboolean resolve_number,
			const char *direction, int answered,
			void (*callback) (GError *, GHashTable **, int, gpointer),
			gpointer data)
{

	GHashTable *qry = g_hash_table_new_full(g_str_hash, g_str_equal,
						NULL, _helpers_free_gvalue);

	if (direction && (!strcmp(direction, "in") || !strcmp(direction, "out"))) {
		g_hash_table_insert(qry, "Direction",
		    _helpers_new_gvalue_string(direction));
	}

	if (answered > -1) {
		g_hash_table_insert(qry, "Answered",
			    _helpers_new_gvalue_boolean(answered));
	}

	phoneui_utils_calls_query(sortby, sortdesc, FALSE, limit_start, limit,
				resolve_number, qry, callback, data);

	g_hash_table_unref(qry);
}

void
phoneui_utils_calls_get(int *count,
			void (*callback) (GError *, GHashTable **, int, gpointer),
			gpointer data)
{
	int limit = (count && *count > 0) ? *count : -1;

	phoneui_utils_calls_get_full("Timestamp", TRUE, 0, limit, TRUE, NULL, -1, callback, data);
}

static void
_call_get_callback(GObject *source, GAsyncResult *res, gpointer data)
{
	(void) source;
	GError *error = NULL;
	GHashTable *content;
	struct _call_get_pack *pack = data;

	content = free_smartphone_pim_call_get_content_finish
						(pack->call, res, &error);
	pack->callback(error, content, pack->data);
	if (error) {
		g_error_free(error);
	}
	if (content) {
		g_hash_table_unref(content);
	}
	g_object_unref(pack->call);
	free(pack);
}

int
phoneui_utils_call_get(const char *call_path,
		       void (*callback)(GError *, GHashTable*, gpointer),
		       gpointer data)
{
	struct _call_get_pack *pack;

	pack = malloc(sizeof(*pack));
	pack->data = data;
	pack->callback = callback;
	pack->call = free_smartphone_pim_get_call_proxy(_dbus(),
				FSO_FRAMEWORK_PIM_ServiceDBusName, call_path);
	g_debug("Getting data of call with path: %s", call_path);
	free_smartphone_pim_call_get_content(pack->call, _call_get_callback, pack);
	return (0);
}

static void
_pdp_activate_callback(GObject *source, GAsyncResult *res, gpointer data)
{
	(void) source;
	GError *error = NULL;
	struct _pdp_pack *pack = data;

	free_smartphone_gsm_pdp_activate_context_finish(pack->pdp, res, &error);
	if (pack->callback) {
		pack->callback(error, pack->data);
	}
	if (error) {
		g_error_free(error);
	}
	g_object_unref(pack->pdp);
	free(pack);
}

static void
_pdp_deactivate_callback(GObject *source, GAsyncResult *res, gpointer data)
{
	(void) source;
	GError *error = NULL;
	struct _pdp_pack *pack = data;

	free_smartphone_gsm_pdp_deactivate_context_finish(pack->pdp, res, &error);
	if (pack->callback) {
		pack->callback(error, pack->data);
	}

	if (error) {
		g_error_free(error);
	}
	g_object_unref(pack->pdp);
	free(pack);
}

void
phoneui_utils_pdp_activate_context(void (*callback)(GError *, gpointer),
			   gpointer userdata)
{
	struct _pdp_pack *pack;

	pack = malloc(sizeof(*pack));
	pack->callback = callback;
	pack->data = userdata;
	pack->pdp = free_smartphone_gsm_get_p_d_p_proxy(_dbus(),
					FSO_FRAMEWORK_GSM_ServiceDBusName,
					FSO_FRAMEWORK_GSM_DeviceServicePath);

	free_smartphone_gsm_pdp_activate_context
				(pack->pdp, _pdp_activate_callback, pack);
}

void
phoneui_utils_pdp_deactivate_context(void (*callback)(GError *, gpointer),
			     gpointer userdata)
{
	struct _pdp_pack *pack;

	pack = malloc(sizeof(*pack));
	pack->callback = callback;
	pack->data = userdata;
	pack->pdp = free_smartphone_gsm_get_p_d_p_proxy(_dbus(),
					FSO_FRAMEWORK_GSM_ServiceDBusName,
					FSO_FRAMEWORK_GSM_DeviceServicePath);

	free_smartphone_gsm_pdp_deactivate_context
				(pack->pdp, _pdp_deactivate_callback, pack);
}

static void
_pdp_get_credentials_cb(GObject *source, GAsyncResult *res, gpointer data)
{
	(void) source;
	GError *error = NULL;
	struct _pdp_credentials_pack *pack = data;
	char *apn, *user, *pw;

	free_smartphone_gsm_pdp_get_credentials_finish
				(pack->pdp, res, &apn, &user, &pw, &error);
	if (pack->callback) {
		pack->callback(error, apn, user, pw, pack->data);
	}
	if (error) {
		g_error_free(error);
	}
	g_object_unref(pack->pdp);
	free(pack);
}

void
phoneui_utils_pdp_get_credentials(void (*callback)(GError *, const char *,
						   const char *, const char *,
						   gpointer), gpointer data)
{
	struct _pdp_credentials_pack *pack;

	pack = malloc(sizeof(*pack));
	pack->callback = callback;
	pack->data = data;
	pack->pdp = free_smartphone_gsm_get_p_d_p_proxy(_dbus(),
					FSO_FRAMEWORK_GSM_ServiceDBusName,
					FSO_FRAMEWORK_GSM_DeviceServicePath);
	free_smartphone_gsm_pdp_get_credentials
				(pack->pdp, _pdp_get_credentials_cb, pack);
}

static void
_network_start_sharing_cb(GObject *source, GAsyncResult *res, gpointer data)
{
	(void) source;
	GError *error = NULL;
	struct _network_pack *pack = data;

	free_smartphone_network_start_connection_sharing_with_interface_finish
						(pack->network, res, &error);
	if (pack->callback) {
		pack->callback(error, pack->data);
	}
}

void
phoneui_utils_network_start_connection_sharing(const char *iface,
			void (*callback)(GError *, gpointer), gpointer data)
{
	struct _network_pack *pack;
	pack = malloc(sizeof(*pack));
	pack->callback = callback;
	pack->data = data;
	pack->network = free_smartphone_get_network_proxy(_dbus(),
					FSO_FRAMEWORK_NETWORK_ServiceDBusName,
					FSO_FRAMEWORK_NETWORK_ServicePathPrefix);
	free_smartphone_network_start_connection_sharing_with_interface
			(pack->network, iface, _network_start_sharing_cb, pack);
}

static void
_network_stop_sharing_cb(GObject *source, GAsyncResult *res, gpointer data)
{
	(void) source;
	GError *error = NULL;
	struct _network_pack *pack = data;

	free_smartphone_network_stop_connection_sharing_with_interface_finish
						(pack->network, res, &error);
	if (pack->callback) {
		pack->callback(error, pack->data);
	}
}

void
phoneui_utils_network_stop_connection_sharing(const char *iface,
			void (*callback)(GError *, gpointer), gpointer data)
{
	struct _network_pack *pack;
	pack = malloc(sizeof(*pack));
	pack->callback = callback;
	pack->data = data;
	pack->network = free_smartphone_get_network_proxy(_dbus(),
					FSO_FRAMEWORK_NETWORK_ServiceDBusName,
					FSO_FRAMEWORK_NETWORK_ServicePathPrefix);
	free_smartphone_network_stop_connection_sharing_with_interface
			(pack->network, iface, _network_stop_sharing_cb, pack);
}

static void
_get_offline_mode_callback(DBusGProxy *proxy, DBusGProxyCall *call, gpointer data)
{
	GError *error = NULL;
	struct _get_offline_mode_pack *pack = data;
	gboolean offline;

	if (!dbus_g_proxy_end_call(proxy, call, &error, G_TYPE_BOOLEAN,
			      &offline, G_TYPE_INVALID)) {
	}
	if (pack->callback) {
		pack->callback(error, offline, pack->data);
	}
	free(pack);
}

void
phoneui_utils_get_offline_mode(void (*callback)(GError *, gboolean, gpointer),
			       gpointer userdata)
{
	GError *error = NULL;
	DBusGProxy *phonefsod;
	DBusGConnection *bus;
	struct _get_offline_mode_pack *pack;

	pack = malloc(sizeof(*pack));
	pack->callback = callback;
	pack->data = userdata;

	bus = dbus_g_bus_get(DBUS_BUS_SYSTEM, &error);
	if (error) {
		callback(error, FALSE, userdata);
		return;
	}
	phonefsod = dbus_g_proxy_new_for_name(bus, "org.shr.phonefso",
			"/org/shr/phonefso/Usage", "org.shr.phonefso.Usage");
	dbus_g_proxy_begin_call(phonefsod, "GetOfflineMode",
				_get_offline_mode_callback, pack, NULL,
				G_TYPE_INVALID);
}

static void
_set_offline_mode_callback(DBusGProxy *proxy, DBusGProxyCall *call, gpointer data)
{
	GError *error = NULL;
	struct _set_offline_mode_pack *pack = data;

	if (!dbus_g_proxy_end_call(proxy, call, &error, G_TYPE_INVALID)) {
	}
	if (pack->callback) {
		pack->callback(error, pack->data);
	}
	free(pack);
}

void
phoneui_utils_set_offline_mode(gboolean offline,
			       void (*callback)(GError *, gpointer),
			       gpointer userdata)
{
	GError *error = NULL;
	DBusGProxy *phonefsod;
	DBusGConnection *bus;
	struct _set_offline_mode_pack *pack;

	pack = malloc(sizeof(*pack));
	pack->callback = callback;
	pack->data = userdata;

	bus = dbus_g_bus_get(DBUS_BUS_SYSTEM, &error);
	if (error) {
		callback(error, userdata);
		return;
	}
	phonefsod = dbus_g_proxy_new_for_name(bus, "org.shr.phonefso",
			"/org/shr/phonefso/Usage", "org.shr.phonefso.Usage");
	dbus_g_proxy_begin_call(phonefsod, "SetOfflineMode",
				_set_offline_mode_callback, pack, NULL,
				G_TYPE_BOOLEAN, offline, G_TYPE_INVALID);

}

static void
_set_pin_callback(DBusGProxy *proxy, DBusGProxyCall *call, gpointer data)
{
	GError *error = NULL;
	struct _set_pin_pack *pack = data;

	dbus_g_proxy_end_call(proxy, call, &error, G_TYPE_INVALID);
	if (pack) {
		if (pack->callback) {
			pack->callback(error, pack->data);
		}
		free(pack);
	}
}

void
phoneui_utils_set_pin(const char *pin, gboolean save,
		      void (*callback)(GError *, gpointer),
		      gpointer userdata)
{
	GError *error = NULL;
	DBusGProxy *phonefsod;
	DBusGConnection *bus;
	struct _set_pin_pack *pack;

	pack = malloc(sizeof(*pack));
	pack->callback = callback;
	pack->data = userdata;

	bus = dbus_g_bus_get(DBUS_BUS_SYSTEM, &error);
	if (error) {
		callback(error, userdata);
		return;
	}
	phonefsod = dbus_g_proxy_new_for_name(bus, "org.shr.phonefso",
			"/org/shr/phonefso/Usage", "org.shr.phonefso.Usage");
	dbus_g_proxy_begin_call(phonefsod, "SetPin", _set_pin_callback, pack,
				NULL, G_TYPE_STRING, pin, G_TYPE_BOOLEAN, save,
				G_TYPE_INVALID);
}
