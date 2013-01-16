#ifndef _STORAGE_H_
#define _STORAGE_H_

#define USER_NAME_LENGTH 16
#define STATISTICS_MAGIC 0x31415926
#define HISTORY_MAGIC 0x27182818

typedef struct
{
	int start_time; //
	int end_time; //
	int quality;
}statistics_item_t;

typedef struct
{
	int magic;
	int header_length;
	int data_group;
	char user_name[USER_NAME_LENGTH];
	int items_count;
	int total_sleep_time; //unit: second
	int sleep_point;
}statistics_header_t;

typedef struct
{
	// breath - bits[0:7]
	// heart_beat - bits[8:15]
	// breath_rate - bits[16:23]
	// heart_beat_rate - bits[24:31]
	int values;
}history_value_item_t;

typedef struct
{
	int magic;
	int header_length;
	int data_group;
	char user_name[USER_NAME_LENGTH];
	int items_count;
}history_values_header_t;

#ifdef __cplusplus
extern "C" 
{
#endif

extern int init_storage(char* user_name, int data_group);
extern int update_user_name(char* user_name);
extern int update_data_group(int data_group);
extern int store_one_statistics_item(statistics_item_t* item);
extern int store_one_history_item(history_value_item_t* item);
extern int get_one_day_history(int time, void* out, int len);
extern int get_one_day_statistics(int time, void* out, int len);
extern int query_one_day_statistics(int time, int* length);
extern int query_one_day_history(int time, int* length);

#ifdef __cplusplus
}
#endif

#endif