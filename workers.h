#ifndef _WORKERS_H_
#define _WORKERS_H_

typedef enum
{
	WORKER_PERIODIC,
	WORKER_DATA_SERVER,	
};
typedef int (* event_handler)(void* param);

typedef struct
{
	int fd;
	int epoll_fd;
	event_handler handler;
}event_info_t;

#ifdef __cplusplus
extern "C" 
{
#endif

extern int init_workers();	
extern int add_handler_to_worker(int worker_id, int fd, event_handler handler);
extern void set_nonblock(int fd);

#ifdef __cplusplus
}
#endif

#endif