#ifndef _DEBUG_H_
#define _DEBUG_H_

#define DEBUG 	0
#define INFO	1
#define ERROR 	2

#define MSG_LEVEL 0

#define log(l, ...) \
	do\
	{\
		if(l >= MSG_LEVEL) {\
			printf(__VA_ARGS__);\
			printf("\n");\
		}\
	}while(0);

#endif
