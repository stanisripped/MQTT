/*
 gcc -Wall  -shared -fPIC lern.c -o lern.so -lmosquitto
 */
#include <stdio.h>
#include <string.h>
#include <mosquitto_broker.h>
#include <mosquitto_plugin.h>
#include <mosquitto.h>

static mosquitto_plugin_id_t *mosq_pid = NULL;

static int callback_basic_auth(int event, void *event_data, void *userdata) {
	struct mosquitto_evt_basic_auth * ed = (struct mosquitto_evt_basic_auth *) event_data;
	mosquitto_log_printf(MOSQ_LOG_INFO, "MOSQ_EVT_BASIC_AUTH %s: %s / %s\n", mosquitto_client_id(ed->client), ed->username, ed->password);
	mosquitto_broker_publish_copy(mosquitto_client_id(ed->client), "hello", 3, (void *) "123", 0, 0, NULL);
	return MOSQ_ERR_SUCCESS;
}

static int callback_acl_check(int event, void *event_data, void *userdata) {
	struct mosquitto_evt_acl_check * ed = (struct mosquitto_evt_acl_check *) event_data;
	int rc = MOSQ_ERR_SUCCESS;
	mosquitto_log_printf(MOSQ_LOG_INFO, "MOSQ_EVT_ACL_CHECK client %s acc %d", mosquitto_client_id(ed->client), ed->access);
	if (ed->access == MOSQ_ACL_READ) {
		mosquitto_log_printf(MOSQ_LOG_INFO, "\tREAD"); //,rc = MOSQ_ERR_NO_SUBSCRIBERS;
			if (!strcmp(ed->topic, "secret"))
				rc = MOSQ_ERR_ACL_DENIED;
	}
	else if (ed->access == MOSQ_ACL_WRITE) {
		mosquitto_log_printf(MOSQ_LOG_INFO, "\tWRITE %s %s", ed->topic, ed->payload ); //, rc = MOSQ_ERR_NO_SUBSCRIBERS;
		if (!strcmp(ed->topic, "secret"))
			rc = MOSQ_ERR_NO_SUBSCRIBERS;
	}
	else if (ed->access == MOSQ_ACL_SUBSCRIBE) {
		mosquitto_log_printf(MOSQ_LOG_INFO, "\tSUBSCRIBE %s", mosquitto_client_id(ed->client));
		if (!strcmp(ed->topic, "secret"))
			rc = MOSQ_ERR_ACL_DENIED;
	}
	mosquitto_log_printf(MOSQ_LOG_INFO, "\trc = %d", rc);
	return rc;
}

static int callback_disconnect(int event, void *event_data, void *userdata) {
	struct mosquitto_evt_disconnect * ed = (struct mosquitto_evt_disconnect *) event_data;
	mosquitto_log_printf(MOSQ_LOG_INFO, "MOSQ_EVT_DISCONNECT %s\n", mosquitto_client_id(ed->client));
	return MOSQ_ERR_SUCCESS;
}

static int callback_message(int event, void *event_data, void *userdata)
{
	struct mosquitto_evt_message * ed = (struct mosquitto_evt_message *) event_data;
	mosquitto_log_printf(MOSQ_LOG_INFO, "MOSQ_EVT_MESSAGE client %s\n", mosquitto_client_id(ed->client));
	mosquitto_log_printf(MOSQ_LOG_INFO, "MOSQ_EVT_MESSAGE client %s user %s \n", mosquitto_client_id(ed->client), mosquitto_client_username(ed->client));
	return MOSQ_ERR_SUCCESS;
}

int mosquitto_plugin_version(int supported_version_count, const int *supported_versions)
{
	mosquitto_log_printf(MOSQ_LOG_INFO, "mosquitto_plugin_version\n");
	for(int i=0; i<supported_version_count; i++)
		if(supported_versions[i] == 5)
			return 5;
	return -1;
}

int evt[] = {MOSQ_EVT_BASIC_AUTH, MOSQ_EVT_ACL_CHECK, MOSQ_EVT_MESSAGE,MOSQ_EVT_DISCONNECT};

int mosquitto_plugin_init(mosquitto_plugin_id_t * pid, void **user_data, struct mosquitto_opt *opts, int opt_count)
{
	mosq_pid = pid;
	
	// Optionen auswerteb
	mosquitto_log_printf(MOSQ_LOG_INFO, "init %d\n", opt_count);
	for (int i = 0; i < opt_count; i++)
		mosquitto_log_printf(MOSQ_LOG_INFO, "\t%d %s %s\n", i, opts[i].key, opts[i].value);
	
	//user_data beschreiben
	//char userdata_text[] = "123";
	//printf("userdata_text %s %lu\n", userdata_text, sizeof(userdata_text));
	//*user_data = mosquitto_malloc(sizeof(userdata_text));
	//memcpy(*user_data, userdata_text, sizeof(userdata_text));

	
	// Callback-Fkt registrieren
	return mosquitto_callback_register(mosq_pid, MOSQ_EVT_MESSAGE, callback_message, NULL, *user_data)
	|| mosquitto_callback_register(mosq_pid, MOSQ_EVT_BASIC_AUTH, callback_basic_auth, NULL, *user_data)
	|| mosquitto_callback_register(mosq_pid, MOSQ_EVT_ACL_CHECK, callback_acl_check, NULL, *user_data)
	|| mosquitto_callback_register(mosq_pid, MOSQ_EVT_DISCONNECT, callback_disconnect, NULL, *user_data);
}

int mosquitto_plugin_cleanup(void *user_data, struct mosquitto_opt *opts, int opt_count)
{
	// mosq_pid aus mosquitto_plugin_init!!!
	return mosquitto_callback_unregister(mosq_pid, MOSQ_EVT_MESSAGE, callback_message, NULL)
	|| mosquitto_callback_unregister(mosq_pid, MOSQ_EVT_BASIC_AUTH, callback_message, NULL)
	|| mosquitto_callback_unregister(mosq_pid, MOSQ_EVT_ACL_CHECK, callback_acl_check, NULL)
	|| mosquitto_callback_unregister(mosq_pid, MOSQ_EVT_DISCONNECT, callback_disconnect, NULL);

}



