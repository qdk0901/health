#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/epoll.h>
#include <pthread.h>

#include "common.h"
#include "workers.h"

#define MAX_EVENTS 1024


void set_nonblock(int fd)
{
	int opts;
	opts = fcntl(fd,F_GETFL);
	if (opts < 0) {
		log(ERROR, "fcntl(fd,GETFL)");
	}

	opts = opts|O_NONBLOCK;
	if (fcntl(fd,F_SETFL,opts) < 0) {
		log(ERROR, "fcntl(fd,SETFL,opts)");
	}
}

static int add_event(int epfd, int fd, int ev_set, event_info_t* ptr)
{
	struct epoll_event ev;
	memset(&ev,0,sizeof(ev));
	ev.data.ptr	= ptr;
	ev.events	= ev_set;

	if(epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev) == -1)
		return STATUS_FAILED;
	return STATUS_OK;
}

static int wait_event(int epfd, struct epoll_event *e)
{
	return epoll_wait(epfd, e, MAX_EVENTS, -1);
}

static int del_event(int epfd, int fd, int ev_set)
{
	close(fd);
	if (epoll_ctl(epfd, EPOLL_CTL_DEL, fd, 0) == -1) {
		log(ERROR, "Delete event failed!!!!");
		return STATUS_FAILED;
	}
	return STATUS_OK;
}


typedef struct
{
    int epoll_fd;
}worker_info_t;

void* worker_thread(void* param)
{
	worker_info_t* wi = (worker_info_t*)param;
	struct epoll_event ev;
	
	for (;;) {
		wait_event(wi->epoll_fd, &ev);
		event_info_t* evi = (event_info_t* )ev.data.ptr;
		
		if(!evi)
		    continue;
		
		if(!evi->handler)
		    continue;
		evi->handler(evi);		
	}
}


static worker_info_t g_periodic; //do the periodic running work like data sampling
static worker_info_t g_data_server; //

static int add_worker(worker_info_t* info)
{
	info->epoll_fd = epoll_create(MAX_EVENTS);
	if (info->epoll_fd < 0) {
		log(ERROR, "cannot create epoll event");
		return STATUS_FAILED;
	}

    pthread_t tid;
    pthread_attr_t attr;
    pthread_attr_init (&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_create(&tid, NULL, worker_thread, info);
    	
	return STATUS_OK;
}

int add_handler_to_worker(int worker_id, int fd, event_handler handler)
{
	event_info_t* evi = (event_info_t*)malloc(sizeof(event_info_t)); 
	if (!evi) {
		log(ERROR, "Memory is not enough!");
		return STATUS_FAILED;
	}
	int epoll_fd = -1;
	
	evi->fd = fd;
	evi->handler = handler;
	
	switch(worker_id){
		case WORKER_PERIODIC:
			epoll_fd = g_periodic.epoll_fd;
		break;
		case WORKER_DATA_SERVER:
			epoll_fd = g_data_server.epoll_fd;
		break;
	}
	
	if (epoll_fd == -1) {
		log(ERROR, "Wrong worker id!");
		free(evi);
		return STATUS_FAILED;	
	}
	
	evi->epoll_fd = epoll_fd;
	add_event(epoll_fd, fd, EPOLLIN|EPOLLET, evi);
	return STATUS_OK;
}

int init_workers()
{
	if (add_worker(&g_periodic) != STATUS_OK)
		return STATUS_FAILED;
		
	if (add_worker(&g_data_server) != STATUS_OK)
		return STATUS_FAILED;
	
	return STATUS_OK;
}