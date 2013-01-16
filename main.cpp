#include "common.h"

int main(int argc, char *argv[])
{
	load_settings();
	if (init_storage(g_default_data_settings.user_name, 
		g_default_data_settings.data_group) != STATUS_OK) {
		log(ERROR, "Init data storage failed");
		return STATUS_FAILED;
	}
	
	if (init_workers() != STATUS_OK)
		return STATUS_FAILED;
	if (init_server() != STATUS_OK)
		return STATUS_FAILED;
	if (init_sampler() != STATUS_OK)
		return STATUS_FAILED;
	
	int sampler_working_status = 0;
	
	if (argc > 1 && !strcmp(argv[1], "auto")) {
		log(INFO, "Health Monitor Auto Start!!!!");
		sampler_working_status = 1;
		start_sampler();
	}
	for (;;) {
		if (wait_for_key_change() == KEY_CHANGE) {
			if (sampler_working_status == 0) {
				sampler_working_status = 1;
				start_sampler();	
			} else {
				sampler_working_status = 0;
				stop_sampler();
			}
		}
	}
	return 0;
}
