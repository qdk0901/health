#ifndef _SAMPLER_H_
#define _SAMPLER_H_

#ifdef __cplusplus
extern "C" 
{
#endif

#pagma pack(1)
typedef struct
{
	unsigned char raw;
	unsigned char asp;
	unsigned char hea;
	unsigned char asp_rate;
	unsigned char hea_rate;
}current_values_t;
#pagma pack()

typedef struct
{
	int start_time;
	int interval;
	int count; //count must less than 24
	int value[24];
}sleep_depth_t;

typedef struct
{
	int sleep_points;
	float sleep_depth_dis[4];
	int sleep_start;
	int sleep_end;
	sleep_depth_t sleep_depth;
}statistics_t;

typedef struct
{
	int count;
	int value[31];
}sleep_points_by_month;

extern int init_sampler();
extern int get_current(current_values_t* out);
extern int get_statistics_by_date(int timestamp, statistics_t* out);
extern int get_sleep_points_by_month(int timestamp, sleep_points_by_month* out);
extern int write_gpio(int gpio, int value);
extern int read_gpio(int gpio, int* value);
#ifdef __cplusplus
}
#endif

#endif