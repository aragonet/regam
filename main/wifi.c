#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_event_loop.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "freertos/event_groups.h"
#include "moisture.c"

#include "esp_http_client.h"

#define MAX_APs 20
const int CONNECTED_BIT = BIT0;
static int postRetries = 0;
static uint16_t ap_num = MAX_APs;
static wifi_ap_record_t ap_records[MAX_APs];
#define wifiNamesMaxLength (MAX_APs * 33 + (4 * 33))
static char names[wifiNamesMaxLength];
static char *wifiNames = names;
// From auth_mode code to string
static char *getAuthModeName(wifi_auth_mode_t auth_mode)
{

	char *names[] = {"OPEN", "WEP", "WPA PSK", "WPA2 PSK", "WPA WPA2 PSK", "MAX"};
	return names[auth_mode];
}

// Empty event handler
static esp_err_t event_handler(void *ctx, system_event_t *event)
{
	return ESP_OK;
}

static void espPost(uint32_t value)
{
	ESP_LOGI("POSTS", "A");
	char *telegramUrl = (char *)malloc(150 * sizeof(char));
	sprintf(telegramUrl, "https://api.telegram.org/xxxxx/sendMessage?chat_id=xxxxx&text=Humitat:%d%%", value);
	esp_http_client_config_t config = {
		.url = telegramUrl};
	ESP_LOGI("POSTS", "B");
	esp_http_client_handle_t client = esp_http_client_init(&config);
	ESP_LOGI("POSTS", "C");
	esp_err_t err = esp_http_client_perform(client);
	ESP_LOGI("POSTS", "D");
	if (err == ESP_OK)
	{
		ESP_LOGI("POSTS", "Status = %d, content_length = %d",
				 esp_http_client_get_status_code(client),
				 esp_http_client_get_content_length(client));
	}
	ESP_LOGI("POSTS", "E");
	esp_http_client_cleanup(client);
	ESP_LOGI("POSTS", "F");

	
	nvs_handle my_handle;
	err = nvs_open("storage", NVS_READWRITE, &my_handle);
	if (err != ESP_OK)
	{
		printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
		return;
	}
	else
	{
		printf("Set reboot to 1\n");
		nvs_set_i8(my_handle, "reboot_value", 1);
		nvs_commit(my_handle);
	}
	nvs_close(my_handle);

	uint64_t sleep1Hour = 3600000000;
	//Microseconds
	esp_deep_sleep(sleep1Hour);
}

void connectWifi(char *ssid, char *password)
{
	uint8_t *ssidU32 = (uint8_t *)32;
	uint8_t *passU64 = (uint8_t *)64;
	ssidU32 = calloc(32, sizeof(char));
	passU64 = calloc(64, sizeof(char));
	strncpy((char *)ssidU32, ssid, 32);
	strncpy((char *)passU64, password, 64);

	wifi_config_t wifi_config = {
		.sta = {
			.ssid = "",
			.password = ""}};
	strcpy((char *)wifi_config.sta.ssid, ssid);
	strcpy((char *)wifi_config.sta.password, password);
	
	ESP_LOGI("SOMETHING NEW", "CONFIG INIT");
	ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
	ESP_LOGI("SOMETHING NEW", "CONFIG SET");
	int err = esp_wifi_connect();
	if (err != 0)
	{
		ESP_LOGI("SOMETHING NEW", "CONECT TO WIFI");
	}

	nvs_handle my_handle;
	err = nvs_open("storage", NVS_READWRITE, &my_handle);
	if (err != ESP_OK)
	{
		printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
	}
	else
	{
		printf("Done %s\n", ssid);
		vTaskDelay(10000 / portTICK_RATE_MS);
	
		printf("Updating restart counter in NVS ... ");
		err = nvs_set_str(my_handle, "wifi_ssid", ssid);
		err = nvs_set_str(my_handle, "wifi_passw", password);
		printf((err != ESP_OK) ? "Failed!\n" : "Done\n");

		printf("Committing updates in NVS ... ");
		err = nvs_commit(my_handle);
		printf((err != ESP_OK) ? "Failed!\n" : "Done\n");

		size_t required_size;
		nvs_get_str(my_handle, "wifi_ssid", NULL, &required_size);
		char *wifi_ssid = malloc(required_size);
		err = nvs_get_str(my_handle, "wifi_ssid", wifi_ssid, &required_size);
		char *wifi_passw = malloc(required_size);
		err = nvs_get_str(my_handle, "wifi_passw", wifi_passw, &required_size);
		switch (err)
		{
		case ESP_OK:
			printf("Done\n");
			printf("Wifi ssid = %s\n", (char *)wifi_ssid);
			printf("Wifi pass = %s\n", (char *)wifi_passw);
			break;
		case ESP_ERR_NVS_NOT_FOUND:
			printf("The value is not initialized yet!\n");
			break;
		default:
			printf("Error (%s) reading!\n", esp_err_to_name(err));
		}

		nvs_close(my_handle);
	}
}

void initWifi()
{
	// initialize NVS
	ESP_ERROR_CHECK(nvs_flash_init());

	// initialize the tcp stack
	tcpip_adapter_init();

	// initialize the wifi event handler
	ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));

	// configure, initialize and start the wifi driver
	wifi_init_config_t wifi_config = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&wifi_config));
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
	ESP_ERROR_CHECK(esp_wifi_start());

	// configure and run the scan process in blocking mode
	wifi_scan_config_t scan_config = {
		.ssid = 0,
		.bssid = 0,
		.channel = 0,
		.show_hidden = true};
	printf("Start scanning...");
	ESP_ERROR_CHECK(esp_wifi_scan_start(&scan_config, true));
	printf(" completed!\n");
	printf("\n");

	// get the list of APs found in the last scan
	ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&ap_num, ap_records));

	// print the list
	printf("Found %d access points:\n", ap_num);
	printf("\n");
	printf("               SSID              | Channel | RSSI |   Auth Mode \n");
	printf("----------------------------------------------------------------\n");

	char openArrayChar[1] = {"["};
	strncat(wifiNames, openArrayChar, 1);

	for (int i = 0; i < ap_num; i++)
	{
		char *comma = "";
		if (i > 0)
		{
			comma = ",";
		}
		printf("%32s | %7d | %4d | %12s\n", (char *)ap_records[i].ssid, ap_records[i].primary, ap_records[i].rssi, getAuthModeName(ap_records[i].authmode));
		size_t needed = snprintf(NULL, 0, "%s\"%s\"", comma, (char *)ap_records[i].ssid) + 1;
		char *buffer = malloc(needed);
		sprintf(buffer, "%s\"%s\"", comma, (char *)ap_records[i].ssid);
		strncat(wifiNames, buffer, needed);
	}

	char closeArrayChar[1] = {"]"};
	strncat(wifiNames, closeArrayChar, 1);
	printf("----------------------------------------------------------------\n");

	nvs_handle my_handle;
	int err = nvs_open("storage", NVS_READWRITE, &my_handle);
	if (err != ESP_OK)
	{
		printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
		return;
	}
	else
	{
		printf("Done\n");
		size_t required_size;
		nvs_get_str(my_handle, "wifi_ssid", NULL, &required_size);
		char *wifi_ssid = malloc(required_size);
		err = nvs_get_str(my_handle, "wifi_ssid", wifi_ssid, &required_size);
		switch (err)
		{
		case ESP_OK:
			printf("Done\n");
			printf("Wifi ssid = %s\n", (char *)wifi_ssid);
			printf("Done\n");

			char *wifi_passw = malloc(required_size);
			err = nvs_get_str(my_handle, "wifi_passw", wifi_passw, &required_size);
			switch (err)
			{
			case ESP_OK:
				printf("P-Done\n");
				printf("P-Wifi passw = %s\n", (char *)wifi_passw);
				connectWifi((char *)wifi_ssid, (char *)wifi_passw);
				espPost(calculateMoisture());
				return;
			case ESP_ERR_NVS_NOT_FOUND:
				printf("P⁻ The value is not initialized yet!\n");
				break;
			default:
				printf("P- Error (%s) reading!\n", esp_err_to_name(err));
			}
			break;
		case ESP_ERR_NVS_NOT_FOUND:
			printf("The value is not initialized yet!\n");
			break;
		default:
			printf("Error (%s) reading!\n", esp_err_to_name(err));
		}
	}
	nvs_close(my_handle);
}