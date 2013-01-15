#include <stdio.h>
#include <fcntl.h>
#include <errno.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/epoll.h>
#include <sys/time.h>
#include <signal.h>
#include <netinet/in.h>

#include <binder/Parcel.h>
#include <cutils/sockets.h>
#include <math.h>

#include "common.h"

using namespace android;
#include <private/android_filesystem_config.h>


enum
{
	DS_REQUEST_FAILED	= 0xdeaddead,
	DS_REQUEST_OK = 0x55AA55AA,
	DS_REQUEST_GET_CURRENT = 0x95270000,
	DS_REQUEST_GET_SETTINGS = 0x95270001,
	DS_REQUEST_SET_SETTINGS = 0x95270002,
	DS_REQUEST_GET_STATISTICS = 0x95270003,
	DS_REQUEST_GET_SLEEP_POINTS_BY_MONTH = 0x95270004,
};



#define DATA_SERVER_SOCKET_NAME	"socket-data-server"

#define MAX_DATA_SIZE 4096

static int send_nonblock(int s, void* buf, int len)
{
	int n;
	int remain = len;  
	while (remain > 0) {
	    n = write(s, buf + len - remain, remain);  
		if (n > 0) {
			remain -= n;	
		} else if (n == -1 ) {
			if(errno == EAGAIN)
				continue;
			else
				return STATUS_FAILED;
		} else
			break;
	}
	return len - remain;
}

static int recv_nonblock(int s, void* buf, int len)
{
	int n;
	int remain = len;
	while (remain > 0) {
		n = read(s, buf + len - remain, remain);
		if (n > 0) {
			remain -= n;	
		} else if (n == -1 ) {
			if(errno == EAGAIN)
				continue;
			else
				return STATUS_FAILED;
		} else
			break;
	}
	return len - remain;
}

static int send_parcel(int s, Parcel* p)
{
	uint32_t length;
	length = htonl(p->dataSize());
	
	if (send_nonblock(s, &length , sizeof(length)) != sizeof(length))
		return STATUS_FAILED;
	if (send_nonblock(s, (void*)p->data() , p->dataSize()) != p->dataSize())
		return STATUS_FAILED;
	return STATUS_OK;
}

static int recv_parcel(int s, Parcel* p)
{
	uint32_t length;
	unsigned char buffer[MAX_DATA_SIZE];
	
	int n = recv_nonblock(s, &length , sizeof(length));
	if (n != sizeof(length))
		return STATUS_FAILED;
		
	length = ntohl(length);
	
	n = recv_nonblock(s, buffer , length);

	if (n != length)
		return STATUS_FAILED;
	
	p->setData(buffer, length);
	return STATUS_OK;
}

static int response_status(int s, int request, int status)
{
	Parcel p;
	
	p.writeInt32(request);
	p.writeInt32(status);
	return send_parcel(s, &p);	
}

static int request_get_current(int s)
{
	Parcel p;
	current_values_t current;
	int ret = get_current(&current);
	if (ret != STATUS_OK)
		return STATUS_FAILED;
	
	p.writeInt32(DS_REQUEST_GET_CURRENT);
	p.write(&current,sizeof(current_values_t));
	
	return send_parcel(s, &p);
}


static int request_get_settings(int s)
{
	Parcel p;
	p.writeInt32(DS_REQUEST_GET_SETTINGS);
	p.writeInt32(g_default_settings.asp_high_threhold);
	p.writeInt32(g_default_settings.asp_high_threhold_warning);
	p.writeInt32(g_default_settings.asp_low_threhold);
	p.writeInt32(g_default_settings.asp_low_threhold_warning);
	p.writeInt32(g_default_settings.asp_stop_condition);
	p.writeInt32(g_default_settings.asp_stop_warning);
	p.writeInt32(g_default_settings.hea_high_threhold);
	p.writeInt32(g_default_settings.hea_high_threhold_warning);
	p.writeInt32(g_default_settings.hea_low_threhold);
	p.writeInt32(g_default_settings.hea_low_threhold_warning);
	p.writeInt32(g_default_settings.hea_stop_condition);
	p.writeInt32(g_default_settings.hea_stop_warning);
	return send_parcel(s, &p);
}

static int request_set_settings(int s, Parcel* p)
{
	int status;
	
	status = p->readInt32(&g_default_settings.asp_high_threhold);
	if (status != NO_ERROR) return STATUS_FAILED;
	
	status = p->readInt32(&g_default_settings.asp_high_threhold_warning);
	if (status != NO_ERROR) return STATUS_FAILED;
		
	status = p->readInt32(&g_default_settings.asp_low_threhold);
	if (status != NO_ERROR) return STATUS_FAILED;
		
	status = p->readInt32(&g_default_settings.asp_low_threhold_warning);
	if (status != NO_ERROR) return STATUS_FAILED;
		
	status = p->readInt32(&g_default_settings.asp_stop_condition);
	if (status != NO_ERROR) return STATUS_FAILED;
		
	status = p->readInt32(&g_default_settings.asp_stop_warning);
	if (status != NO_ERROR) return STATUS_FAILED;
		
	status = p->readInt32(&g_default_settings.hea_high_threhold);
	if (status != NO_ERROR) return STATUS_FAILED;
		
	status = p->readInt32(&g_default_settings.hea_high_threhold_warning);
	if (status != NO_ERROR) return STATUS_FAILED;
		
	status = p->readInt32(&g_default_settings.hea_low_threhold);
	if (status != NO_ERROR) return STATUS_FAILED;
		
	status = p->readInt32(&g_default_settings.hea_low_threhold_warning);
	if (status != NO_ERROR) return STATUS_FAILED;
		
	status = p->readInt32(&g_default_settings.hea_stop_condition);
	if (status != NO_ERROR) return STATUS_FAILED;
		
	status = p->readInt32(&g_default_settings.hea_stop_warning);
	if (status != NO_ERROR) return STATUS_FAILED;
	
	response_status(s, DS_REQUEST_SET_SETTINGS, DS_REQUEST_OK);
	return STATUS_OK;
}

static int request_get_statistics(int s, Parcel* in)
{
	Parcel p;
	int timestamp;
	int status = in->readInt32(&timestamp);
	if (status != NO_ERROR)
		return STATUS_FAILED;
	
	statistics_t statistics;
	get_statistics_by_date(timestamp, &statistics);
	
	int i;
	p.writeInt32(DS_REQUEST_GET_STATISTICS);
	p.writeInt32(statistics.sleep_points);
	for (i = 0; i < 4; i++) {
		p.writeFloat(statistics.sleep_depth_dis[i]);	
	}
	p.writeInt32(statistics.sleep_start);
	p.writeInt32(statistics.sleep_end);
	
	p.writeInt32(statistics.sleep_depth.start_time);
	p.writeInt32(statistics.sleep_depth.interval);
	p.writeInt32(statistics.sleep_depth.count);
	for (i = 0; i < statistics.sleep_depth.count; i++) {
		p.writeInt32(statistics.sleep_depth.value[i]);
	}
	return send_parcel(s, &p);
}

static int request_get_sleep_point_by_month(int s, Parcel* in)
{
	sleep_points_by_month sleep_points;
	int timestamp;
	int status = in->readInt32(&timestamp);
	if (status != NO_ERROR)
		return STATUS_FAILED;
	
	get_sleep_points_by_month(timestamp, &sleep_points);
	Parcel p;
	int i;
	p.writeInt32(DS_REQUEST_GET_SLEEP_POINTS_BY_MONTH);
	p.writeInt32(sleep_points.count);
	for (i = 0; i < sleep_points.count; i++) {
		p.writeInt32(sleep_points.value[i]);	
	}
	
	return send_parcel(s, &p);
}


static int client_event_handler(void* param)
{
	event_info_t* evi = (event_info_t*)param;
	Parcel p;
	int ret = STATUS_FAILED;
	
	if (recv_parcel(evi->fd, &p) >= 0) {
		int request;
		int status = p.readInt32(&request);
		if (status == NO_ERROR) {
			switch(request)
			{
				case DS_REQUEST_GET_CURRENT:
					ret = request_get_current(evi->fd);
				break;
				case DS_REQUEST_GET_SETTINGS:
					ret = request_get_settings(evi->fd);
				break;
				case DS_REQUEST_SET_SETTINGS:
					ret = request_set_settings(evi->fd, &p);
				break;
				case DS_REQUEST_GET_STATISTICS:
					ret = request_get_statistics(evi->fd, &p);
				break;
				case DS_REQUEST_GET_SLEEP_POINTS_BY_MONTH:
					ret = request_get_sleep_point_by_month(evi->fd, &p);
				break;
			}
			
			if(ret == STATUS_FAILED)
				response_status(evi->fd, request, DS_REQUEST_FAILED);
		}
	} else {
		//client has been disconnect
		ret = STATUS_OK;
		log(INFO, "Client disconnected\n");
		close(evi->fd);
		free(evi);
	}
	
	return ret;
}
static int server_event_handler(void* param)
{
	event_info_t* evi = (event_info_t*)param;
	struct sockaddr_in remote_addr;
	socklen_t sin_size = sizeof(struct sockaddr_in);
	
	int cs = accept(evi->fd,
			(struct sockaddr *) &remote_addr, &sin_size);
	if (cs >= 0) {
		set_nonblock(cs);
		log(INFO, "One client connected, add handler for it!");
		if (add_handler_to_worker(WORKER_DATA_SERVER, cs, client_event_handler)
			!= STATUS_OK)
			return STATUS_FAILED;
	}
	
	return STATUS_OK;
}

int init_server()
{
	int server_socket = socket_local_server (DATA_SERVER_SOCKET_NAME,
            ANDROID_SOCKET_NAMESPACE_ABSTRACT, SOCK_STREAM);

	if (server_socket < 0) {
		log(ERROR, "Init data server failed\n");
		return STATUS_FAILED;	
	}
	
	if (add_handler_to_worker(WORKER_DATA_SERVER, server_socket, server_event_handler)
			!= STATUS_OK)
			return STATUS_FAILED;
			
	return STATUS_OK;
}