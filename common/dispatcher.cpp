#include "dispatcher.h"
#include "message_queue.h"
#include "net_util.h"
#include "logger.h"

namespace alpha{

Dispatcher::Dispatcher():loop_(nullptr),running_(false){

}

Dispatcher::~Dispatcher(){
	if(loop_ != nullptr){
		ev_loop_destroy(loop_);
	}
}

void Dispatcher::StartServer(uint16_t port){
	int fd = netlib_socket();
	if (fd < 0){
		LogError("netlib_socket error:%d",errno);
		exit(EXIT_FAILURE);
	}

	netlib_setnonblocking(fd);

	struct linger linger;
	linger.l_onoff = 1;
	linger.l_linger = 0;
	setsockopt(fd, SOL_SOCKET, SO_LINGER, (const char *) &linger, sizeof(linger));

	netlib_reuse_port(fd, true);

	if(netlib_bind(fd, port) < 0){
		LogError("netlib_bind error:%d\n",errno);
		exit(EXIT_FAILURE);
	}
	if (netlib_listen(fd,20)){
		LogError("netlib_listen error:%d\n",errno);
		exit(EXIT_FAILURE);
	}
	
	loop_ = ev_default_loop(EVBACKEND_EPOLL | EVFLAG_NOENV);

	struct ev_io io_watcher;
	ev_init(&io_watcher, accept_cb);
	AddEvent(&io_watcher, fd, EV_READ);

	//LogInfo("EpollServer::StartServer:listen fd:%d\n",fd);
	
	//InitEventfd();
	//InitTimer();

	ev_run(loop_, 0);
}

void Dispatcher::OnAccept(int fd){
	LogDebug("EpollServer::OnAccept %d\n", fd);

	int conn_fd = netlib_accept(fd);
	if (conn_fd < 0){
		LogError("accept error\n");
	}

	netlib_setnonblocking(conn_fd);

	struct ev_io* conn_ev = (struct ev_io*) malloc(sizeof(struct ev_io));  //TODO, delete ptr
	if (conn_ev == NULL){
		LogError("malloc error in accept_cb\n");
		return;
	}

	ev_init(conn_ev, r_w_cb);
	AddEvent(conn_ev, conn_fd, EV_READ);
	//AddChannel(conn_ev, conn_fd, FD_TYPE_SERVER);

	MessageQueue::getInstance().MQ2S_Push(conn_fd, FD_TYPE_CONN, nullptr);

	//free(conn_ev);

}

void Dispatcher::OnRead(int fd){
	LogDebug("Dispatcher::OnRead:%d\n", fd);

	//LogDebug("EpollServer::OnRead: offset %d\n", in_buf.GetWriteOffset());

	//LogDebug("netlib_recv before%d\n", (int)strlen(in_buf.GetBuffer()));

	char* buffer = (char*)malloc(READ_BUF_SIZE);
	//char buffer[READ_BUF_SIZE] = {0};

	int bytes = netlib_recv(fd, buffer, READ_BUF_SIZE);
	if (bytes <= 0)
	{
		//remote close fd actively
		//RemoveEvent(fd, channel);
		//OnFdClosed(fd, fdtype);
		//LogInfo("remote close fd actively: %d\n", fd);
		return;
	}

	//LogInfo("Dispatcher::recv:%d bytes, %s \n", bytes, buffer);

	std::string str_buf(buffer);
	LogInfo("Dispatcher::recv:%d bytes, %s，%d\n", bytes, buffer, str_buf.size());
	MessageQueue::getInstance().MQ2S_Push(fd, FD_TYPE_READ, str_buf.c_str());

}

int Dispatcher::AddEvent(struct ev_io* ev, int fd, short events){
	ev_io_set(ev, fd, events);
	ev_io_start(loop_, ev);

	return 0;
}
/*
int Dispatcher::UpdateEvent(int fd, short events, Channel* channel)
{	
	//LogDebug("UpdateEvent:%d,%d\n", fd, events);

	struct ev_io* io_watcher = channel->GetIoWatcher();

	if (io_watcher == NULL)
	{
		LogError("UpdateEvent error: io_watcher null\n");
		return -1;
	}
	//TODO, how to update fd event efficiently
	ev_io_stop(loop_, io_watcher);
	ev_io_set(io_watcher, fd, events);
	ev_io_start(loop_, io_watcher);

	return 0;
}

void Dispatcher::RemoveEvent(int fd, Channel* channel)
{
	LogDebug("RemoveEvent: %d\n",fd);
	struct ev_io* io_watcher = channel->GetIoWatcher();
	if (io_watcher == NULL)
	{
		LogError("RemoveEvent error:io_watcher null\n");
		return;
	}
	ev_io_stop(loop_, io_watcher);
}
*/

void Dispatcher::accept_cb(struct ev_loop* loop, struct ev_io* watcher, int revents){
	int fd = watcher->fd;

	if (EV_ERROR & revents){
		//LogDebug("error event in accept\n");
		return;
	}
	printf("accept_cb \n");
	Dispatcher::getInstance().OnAccept(fd);
	//EpollServer::getSingleton().OnAccept(fd);
}

void Dispatcher::r_w_cb(struct ev_loop* loop, struct ev_io* watcher, int revents){
	int fd = watcher->fd;
	//LogDebug("r_w_cb %d,%d\n", fd, revents);
	if (EV_ERROR & revents){
		//LogError("error event in read or write\n");
		return;
	}

	if (EV_READ & revents){
		Dispatcher::getInstance().OnRead(fd);
	}
	if (EV_WRITE & revents){
		//Dispatcher::getInstance().OnWrite(fd, FD_TYPE_SERVER);
	}
}

}
