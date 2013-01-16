#ifndef _SETTINGS_H_
#define _SETTINGS_H_

typedef struct
{
	int asp_high_threhold;
	int asp_high_threhold_warning;
	int asp_low_threhold;
	int asp_low_threhold_warning;
	int asp_stop_condition;
	int asp_stop_warning;
	int hea_high_threhold;
	int hea_high_threhold_warning;
	int hea_low_threhold;
	int hea_low_threhold_warning;
	int hea_stop_condition;
	int hea_stop_warning;
}system_settings_t;

typedef struct
{
	char user_name[USER_NAME_LENGTH];
	int data_group;
}data_settings_t;


#ifdef __cplusplus
extern "C" 
{
#endif

extern system_settings_t g_default_settings;
extern data_settings_t g_default_data_settings;
extern int load_settings();
extern int store_settings();

#ifdef __cplusplus
}
#endif

#endif