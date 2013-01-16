#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>

#include "common.h"
#include "test_data.h"

#define I2C_SLAVE 0x703
#define I2C_SLAVE_FORCE 0x706
#define I2C_GET_GPIO 0x707
#define I2C_SET_GPIO 0x708
#define	DS_SIG_TIMER 0x31415926

#define WORKING_VOLTAGE 3333

#define REG_CONV 0
#define REG_CONF 1
#define REG_LO_THRE 2
#define REG_HI_THRE 3

#define ADC_MAX	(32752)
#define ADC_MIN	(-32752)

typedef unsigned char u8, __u8;
typedef unsigned short u16, __u16;
typedef unsigned int u32, __u32;
typedef unsigned long long __u64;

//input multiplexer
typedef enum
{
	ADC_MUX_AIN0_AIN1,
	ADC_MUX_AIN0_AIN3,
	ADC_MUX_AIN1_AIN3,
	ADC_MUX_AIN2_AIN3,
	ADC_MUX_AIN0_GND,
	ADC_MUX_AIN1_GND,
	ADC_MUX_AIN2_GND,
	ADC_MUX_AIN3_GND,

}adc_mux_sel_e;
//gain amplifier
typedef enum
{
	ADC_FS_6144,
	ADC_FS_4096,
	ADC_FS_2048,
	ADC_FS_1024,
	ADC_FS_0512,
	ADC_FS_0256,
}adc_fs_sel_e;

//data rate
typedef enum
{
	ADC_DR_128SPS,
	ADC_DR_250SPS,
	ADC_DR_490SPS,
	ADC_DR_920SPS,
	ADC_DR_1600SPS,
	ADC_DR_2400SPS,
	ADC_DR_3300SPS
	
}adc_dr_sel_e;

//device mode
typedef enum
{
	ADC_DM_CONTINUOUS,
	ADC_DM_SINGLE_SHOT,
}adc_dm_sel_e;

//comparator mode
typedef enum
{
	ADC_CM_HYSTERESIS,
	ADC_CM_WINDOW,
}adc_cm_sel_e;

//comparator polarity
typedef enum
{
	ADC_CP_ACTIVE_HIGH,
	ADC_CP_ACTIVE_LOW,
}adc_cp_sel_e;

//latching comparator
typedef enum
{
	ADC_CL_NON_LATCHING,
	ADC_CL_LATCHING,	
}adc_cl_sel_e;

//comparator queue
typedef enum
{
	ADC_CQ_ONE,
	ADC_CQ_TWO,
	ADC_CQ_FOUR,
	ADC_CQ_DISABLE,
}adc_cq_sel_e;

typedef struct
{
	u32 gpio;
	u32 value;
}gpio_setting_t;

static int g_fd = -1;
static u16 g_config = 0;
static int g_sampler_running = 0;
static int g_signal_fd = -1;

static int read_register(int fd, u8 reg, u16* out)
{
	u16 data;
	if (write(fd, &reg, 1) != 1) {
		log(ERROR, "set register address failed");
		return STATUS_FAILED;
	}
	
	if (read(fd, &data, 2) != 2) {
		log(ERROR, "read register failed");
		return STATUS_FAILED;	
	}
	
	data = (data << 8) | (data >> 8); // swap the bytes order
	*out = data;

	return STATUS_OK;
}

static int write_register(int fd, u8 reg, u16 value)
{
	u8 data[3];
	data[0] = reg;
	data[1] = value >> 8;
	data[2] = value & 0xFF;

	int ret = write(fd, data, 3);
	if (ret != 3) {
		log(ERROR, "write register failed");
		return STATUS_FAILED;	
	}
	return STATUS_OK;
}

static int set_conf_detailed(int fd, int off, int mask, int val)
{
	u16 conf;
	int ret = read_register(fd, REG_CONF, &conf);
	if (ret != STATUS_OK)
		return ret;

	conf &= ~(mask << off);
	conf |= (val & mask) << off;
	return write_register(fd, REG_CONF, conf);	
}

static int set_input_multiplexer(int fd, adc_mux_sel_e im)
{
	return set_conf_detailed(fd, 12, 7, im);
}

static int set_gain_amplifier(int fd, adc_fs_sel_e fs)
{
	return set_conf_detailed(fd, 9, 7, fs);
}

static int set_device_mode(int fd, adc_dm_sel_e dm)
{
	return set_conf_detailed(fd, 8, 1, dm);
}

static int set_data_rate(int fd, adc_dr_sel_e dr)
{
	return set_conf_detailed(fd, 5, 7, dr);
}

static int set_comp_mode(int fd, adc_cm_sel_e cm)
{
	return set_conf_detailed(fd, 4, 1, cm);
}

static int set_comp_pol(int fd, adc_cp_sel_e cp)
{
	return set_conf_detailed(fd, 3, 1, cp);
}

static int set_comp_lat(int fd, adc_cl_sel_e cl)
{
	return set_conf_detailed(fd, 2, 1, cl);
}

static int set_comp_que(int fd, adc_cq_sel_e cq)
{
	return set_conf_detailed(fd, 0, 3, cq);
}

static int adc_calibration(int fd)
{
	// ideal adc convertion should be 
	// 0 < conv < 0x7ff0 ||  0x8010 < conv < 0xfff0 
	
	short conv;

	set_gain_amplifier(fd, 2);
	return STATUS_OK;
	
	adc_fs_sel_e fs = 5;
	do
	{
		set_gain_amplifier(fd, fs);
		usleep(100*1000); //wait for output stable
		
		read_register(fd, REG_CONV, &conv);
		fs--;
		
	}while((conv <= ADC_MIN || conv >= ADC_MAX) && fs >= 0);

	if (fs < 0) {
		log(ERROR, "adc calibration failed");
		return STATUS_FAILED;
	}
	return STATUS_OK;
}

static int calc_voltage(int adc, int config, int ref)
{
	int fs = (config >> 9) & 7;
	if (fs >= 5)
		fs = 256;
	else if (fs == 0)
		fs = 6144;
	else if (fs == 1)
		fs = 4096;
	else if (fs == 2)
		fs = 2048;
	else if (fs == 3)
		fs = 1024;
	else if (fs == 4)
		fs = 512;

	//adc = FS(input) - FS(ref) = FS(input - ref) = (input - ref) / (fs * 2 / 65536)
	//input = adc * fs * 2 / 65536 + ref ;

	return ref + adc * fs * 2 / 65536;
}

static int adc_sanity_check(int fd)
{
	u16 test;
	return read_register(fd, REG_CONV, &test);
}

int write_gpio(int gpio, int value)
{
	gpio_setting_t g;
	g.gpio = gpio;
	g.value = value;
	return ioctl(g_fd,I2C_GET_GPIO, &gpio);
}

int read_gpio(int gpio, int* value)
{
	int ret;
	gpio_setting_t g;
	g.gpio = gpio;
	ret = ioctl(g_fd,I2C_GET_GPIO, &gpio);
	*value = g.value;
	return ret;
}

int start_sampler()
{
	if (adc_calibration(g_fd) != STATUS_OK)
		return STATUS_FAILED;
	if (read_register(g_fd, REG_CONF, &g_config) != STATUS_OK)
		return STATUS_FAILED;
	
	g_sampler_running = 1;
	
	log(INFO, "Data sampler started");
	return STATUS_OK;
}

void stop_sampler()
{
	g_sampler_running = 0;
	log(INFO, "Data sampler stopped");
}

static int init_adc()
{
	int fd = open("/dev/io_emul_i2c",O_RDWR);

	if ( fd < 0 ) {
		log(ERROR, "cannot open device /dev/i2c-3");
		return STATUS_FAILED;	
	}

	int ret = ioctl(fd,I2C_SLAVE_FORCE, 0x48);
	if (ret != STATUS_OK) {
		log(ERROR, "ioctl error: %s", strerror(errno));
		return STATUS_FAILED;
	}

	if (adc_sanity_check(fd) != STATUS_OK) {
		log(ERROR, "adc does not work well!!");
		return STATUS_FAILED;
	}

	set_input_multiplexer(fd, ADC_MUX_AIN0_AIN1);
	set_comp_que(fd, ADC_CQ_DISABLE);
	set_data_rate(fd, ADC_DR_250SPS);
	set_device_mode(fd, ADC_DM_CONTINUOUS);

	g_fd = fd;
	return STATUS_OK;
}

static void push_raw_data(float vol)
{
	// do the calculation here
}

static void signal_handler(int s)
{
	int i;
	int sig;
	if (s == SIGALRM) {
		sig = DS_SIG_TIMER;
	}
	
	if (g_sampler_running) {
		write(g_signal_fd, &sig, 4);
	}
}


static int start_timer()
{
	struct itimerval itv, oitv;
	
	itv.it_value.tv_sec = 1;
	itv.it_value.tv_usec = 0;

	itv.it_interval.tv_sec = 0;
	itv.it_interval.tv_usec = 1000000 / DATA_SAMPLE_RATE;


	setitimer(ITIMER_REAL, &itv, &oitv);
	signal(SIGALRM,signal_handler);
	
	return STATUS_OK;
}

static int timer_event_handler(void* param)
{
	int sig;
	event_info_t* evi = (event_info_t*)param;
	read(evi->fd, &sig, 4);
	if (sig == DS_SIG_TIMER) {
#if TESTING_MODE
		//push_raw_data(test_data[g_test_index]);
#else
		short raw;
		int ret = read_register(g_fd, REG_CONV, &raw);
		if (ret != STATUS_OK)
			return ret;
		
		raw = calc_voltage(raw, g_config, WORKING_VOLTAGE / 2);
		push_raw_data(raw / 1000.0f);
#endif
	}
	return STATUS_OK;
}

int init_sampler()
{
	int pfd[2];
	
#if !TESTING_MODE
	if (init_adc()) {
		return STATUS_FAILED;	
	}
#endif
	
	pipe(pfd);
	if (pfd[0] < 0 || pfd[1] < 0) {
		log(ERROR, "cannot create pipe fd");
		return STATUS_FAILED;
	}
	set_nonblock(pfd[0]);
	set_nonblock(pfd[1]);

	g_signal_fd = pfd[1];
	
	if (add_handler_to_worker(WORKER_PERIODIC, pfd[0], timer_event_handler) != STATUS_OK)
		return STATUS_FAILED;

	start_timer();
	return STATUS_OK;
}

int get_current(current_values_t* out)
{
	out->raw = 0;
	out->asp = 0;
	out->hea = 0;
	out->asp_rate = 0;
	out->hea_rate = 0;
	return STATUS_OK;
}


