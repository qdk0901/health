#ifndef _EMD_H_
#define _EMD_H_

#define CALCULATE_SIZE 512

typedef struct
{
	float raw[CALCULATE_SIZE];
	float asp[CALCULATE_SIZE];
	float hea[CALCULATE_SIZE];
	float asp_rate;
	float hea_rate;
}calc_result;

#ifdef __cplusplus 
extern "C" {
#endif

void emd_init();
void calculate(float* voltage, calc_result* result);

#ifdef __cplusplus
}
#endif

#endif
