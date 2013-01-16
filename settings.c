#include "common.h"

#define DEFAULT_USER_NAME "hdm"
#define SYSTEM_SETTINGS_PATH "/sdcard/hdm/system_settings"
#define DATA_SETTINGS_PATH "/sdcard/hdm/data_settings"

system_settings_t g_default_settings = 
{
	.asp_high_threhold = 0,
	.asp_high_threhold_warning = 0,
	.asp_low_threhold = 0,
	.asp_low_threhold_warning = 0,
	.asp_stop_condition = 0,
	.asp_stop_warning = 0,
	.hea_high_threhold = 0,
	.hea_high_threhold_warning = 0,
	.hea_low_threhold = 0,
	.hea_low_threhold_warning = 0,
	.hea_stop_condition = 0,
	.hea_stop_warning = 0,
};

data_settings_t g_default_data_settings = 
{
	.user_name = DEFAULT_USER_NAME,
	.data_group = 0,
};

int load_settings()
{
	FILE* f = fopen(SYSTEM_SETTINGS_PATH, "r");
	if (!f) {
		log(ERROR, "Open % error", SYSTEM_SETTINGS_PATH);
		return STATUS_OK;
	}
	fread(&g_default_settings, 1, sizeof(system_settings_t), f);
	fclose(f);
	
	f = fopen(DATA_SETTINGS_PATH, "r");
	if (!f) {
		log(ERROR, "Open % error", DATA_SETTINGS_PATH);
		return STATUS_OK;
	}
	fread(&g_default_data_settings, 1, sizeof(data_settings_t), f);
	fclose(f);
	
	return STATUS_OK;
}
int store_settings()
{
	FILE* f = fopen(SYSTEM_SETTINGS_PATH, "w");
	if (!f) {
		log(ERROR, "Open % error", SYSTEM_SETTINGS_PATH);
		return STATUS_OK;
	}
	fwrite(&g_default_settings, 1, sizeof(system_settings_t), f);
	fclose(f);
	
	f = fopen(DATA_SETTINGS_PATH, "w");
	if (!f) {
		log(ERROR, "Open % error", DATA_SETTINGS_PATH);
		return STATUS_OK;
	}
	fwrite(&g_default_data_settings, 1, sizeof(data_settings_t), f);
	fclose(f);
	
	return STATUS_OK;		
}