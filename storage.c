
#include "common.h"

#define MAX_PATH 260
#define STATISTICS_PATH 1
#define HISTORY_PATH 2
#define HISTORY_ITEM_ARRAY_BUFFER 32


static int g_date_time = -1;
static char g_user_name[USER_NAME_LENGTH];
static int g_data_group = 0;

static char g_statistics_path[MAX_PATH];
static int g_statistics_header_offset = 0;
static statistics_header_t g_statistics_header;

static char g_history_path[MAX_PATH];
static int g_history_values_header_offset = 0;
static history_values_header_t g_history_values_header;

static history_value_item_t g_history_items_buffer[HISTORY_ITEM_ARRAY_BUFFER] = {0};
static int g_hitory_buffer_index = 0;


static int check_cross_date();
static int cross_date_switch();
static int get_today_time();
static int path_generate(int time, int type, char* path_out);

static int write_file_at(char* path, int offset, void* buffer, int len)
{
	FILE* f = fopen(path, "rw+");
	if (!f) {
		log(ERROR, "Cannot open %s!", path);
		return STATUS_FAILED;	
	}
	
	if (offset < 0)
		fseek(f, 0, SEEK_END);
	else
		fseek(f, offset, SEEK_SET);

	fwrite(buffer, 1, len, f);
	close(f);
	return STATUS_OK;
}

static int append_to_file(char* path, void* buffer, int len)
{
	return write_file_at(path, -1, buffer, len);
}

static int get_file_length(char* path)
{
	FILE* f = fopen(path, "a+");
	if (!f) {
		log(ERROR, "Cannot open %s!", path);
		return -1;	
	}
	fseek(f, 0, SEEK_END);
	return ftell(f);
}
int update_user_name(char* user_name)
{
	strncpy(g_user_name, user_name, USER_NAME_LENGTH - 1);
	
	strncpy(g_statistics_header.user_name, user_name, USER_NAME_LENGTH - 1);
	g_statistics_header_offset = get_file_length(g_statistics_path);
	if (g_statistics_header_offset < 0)
		return STATUS_FAILED;
	if (append_to_file(g_statistics_path, &g_statistics_header, sizeof(statistics_header_t)) != STATUS_OK)
		return STATUS_FAILED;

	strncpy(g_history_values_header.user_name, user_name, USER_NAME_LENGTH - 1);
	g_history_values_header_offset = get_file_length(g_history_path);
	if (g_history_values_header_offset < 0)
		return STATUS_FAILED;
	if (append_to_file(g_history_path, &g_history_values_header, sizeof(history_values_header_t)) != STATUS_OK)
		return STATUS_FAILED;
		
	return STATUS_OK;
}

int update_data_group(int data_group)
{
	g_data_group = data_group;
	
	g_statistics_header.data_group =  data_group;
	g_statistics_header_offset = get_file_length(g_statistics_path);
	if (g_statistics_header_offset < 0)
		return STATUS_FAILED;
	if (append_to_file(g_statistics_path, &g_statistics_header, sizeof(statistics_header_t)) != STATUS_OK)
		return STATUS_FAILED;

	g_history_values_header.data_group =  data_group;
	g_history_values_header_offset = get_file_length(g_history_path);
	if (g_history_values_header_offset < 0)
		return STATUS_FAILED;
	if (append_to_file(g_history_path, &g_history_values_header, sizeof(history_values_header_t)) != STATUS_OK)
		return STATUS_FAILED;
		
	return STATUS_OK;
}

static int update_statistics_header()
{
	g_statistics_header.items_count++;
	return write_file_at(g_statistics_path, g_statistics_header_offset, 
		&g_statistics_header, sizeof(statistics_header_t));
}

static int update_history_values_header(int count)
{
	g_history_values_header.items_count += count;
	return write_file_at(g_history_path, g_history_values_header_offset, 
		&g_history_values_header, sizeof(history_values_header_t));
}

int store_one_statistics_item(statistics_item_t* item)
{
	update_statistics_header();
	return append_to_file(g_statistics_path, item, sizeof(statistics_item_t));
}

// update items array into file
static int store_history_item_array(history_value_item_t* items, int count)
{
	update_history_values_header(count);
	append_to_file(g_history_path, items, sizeof(history_value_item_t) * count);
	cross_date_switch();
	return STATUS_OK;
}

int store_one_history_item(history_value_item_t* item)
{
	if (g_hitory_buffer_index < HISTORY_ITEM_ARRAY_BUFFER) {
		memcpy(&g_history_items_buffer[g_hitory_buffer_index], item, sizeof(history_value_item_t));
		g_hitory_buffer_index++;	
	} else {
		g_hitory_buffer_index = 0;
		
		if (store_history_item_array(g_history_items_buffer, HISTORY_ITEM_ARRAY_BUFFER) != STATUS_OK)
			return STATUS_FAILED;
	}
	return STATUS_OK;
}


static int check_cross_date()
{
	int is_cross = 0;
	if (g_date_time != get_today_time()) {
		if (g_date_time > 0)
			is_cross = 1;
		g_date_time = 	get_today_time();
	}
	return is_cross;
}


int init_storage(char* user_name, int data_group)
{
	int time = get_today_time();
	path_generate(time, STATISTICS_PATH, g_statistics_path);
	path_generate(time, HISTORY_PATH, g_history_path);
	
	memset(&g_statistics_header, 0, sizeof(statistics_header_t));
	g_statistics_header.magic = STATISTICS_MAGIC;
	strncpy(g_statistics_header.user_name, user_name, USER_NAME_LENGTH - 1);
	g_statistics_header.data_group = data_group;
	
	memset(&g_history_values_header, 0, sizeof(history_values_header_t));
	g_history_values_header.magic = HISTORY_MAGIC;
	strncpy(g_history_values_header.user_name, user_name, USER_NAME_LENGTH - 1);
	g_history_values_header.data_group = data_group;
	
	if (update_user_name(user_name) != STATUS_OK) {
		log(ERROR, "Update user name failed!");
		return STATUS_FAILED;	
	}
	if (update_data_group(data_group) != STATUS_OK) {
		log(ERROR, "Update data group failed!");
		return STATUS_FAILED;
	}
	return STATUS_OK;
}

static int cross_date_switch()
{
	if (check_cross_date()) {
		return init_storage(g_user_name, g_data_group);
	}
	return STATUS_OK;
}

static int get_today_time()
{
	struct tm* _tm = gmtime(time(0));
	_tm->tm_sec = 0;
	_tm->tm_min = 0;
	_tm->tm_hour = 0;
	return mktime(_tm);
}

static int path_generate(int time, int type, char* path_out)
{
	char* prefix;
	if (type == STATISTICS_PATH)
		prefix = "STATISTICS";
	else if(type == HISTORY_PATH)
		prefix = "HISTORY";
	else
		return STATUS_FAILED;
		
	sprintf(path_out, "%s_%d", prefix, time);
	return STATUS_OK;
}


static int get_one_day_data(int type, int time, unsigned char* out, int len)
{
	char path[MAX_PATH];
	FILE* f;
	path_generate(time, type, path);
	f = fopen(path, "r");
	if (!f) {
		log(ERROR, "cannot open %s", path);
		return STATUS_FAILED;
	}
	
	fread(out, 1, len, f);
	fclose(f);
	
	return STATUS_OK;	
}
int get_one_day_statistics(int time, void* out, int len)
{
	return get_one_day_data(STATISTICS_PATH, time, out, len);
}

int get_one_day_history(int time, void* out, int len)
{
	return get_one_day_data(HISTORY_PATH, time, out, len);
}

static int query_one_day_data(int type, int time, int* length)
{
	char path[MAX_PATH];
	FILE* f;
	path_generate(time, type, path);
	f = fopen(path, "r");
	if (!f) {
		log(ERROR, "cannot open %s", path);
		return STATUS_FAILED;
	}
	
	fseek(f, 0, SEEK_END);
	*length = ftell(f);
	fclose(f);
	
	return STATUS_OK;	
}

int query_one_day_statistics(int time, int* length)
{
	return query_one_day_data(STATISTICS_PATH, time, length); 
}

int query_one_day_history(int time, int* length)
{
	return query_one_day_data(HISTORY_PATH, time, length); 
}
