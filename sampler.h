#ifndef _SAMPLER_H_
#define _SAMPLER_H_

#define GPIO_KEY	0
#define GPIO_LED_1	1
#define GPIO_LED_2	2
#define GPIO_LED_3	3
#define GPIO_RX		4
#define GPIO_TX		5

#define DATA_SAMPLE_RATE 20

#ifdef __cplusplus
extern "C" 
{
#endif

#pragma pack(1)
typedef struct
{
	unsigned char raw;
	unsigned char asp;
	unsigned char hea;
	unsigned char asp_rate;
	unsigned char hea_rate;
}current_values_t;
#pragma pack()

typedef struct
{
	int start_time;
	int interval;
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
extern int start_sampler();
extern void stop_sampler();
#ifdef __cplusplus
}
#endif

#endif