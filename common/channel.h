/*
*封装sockfd
*/
#ifndef __ALPHA_CHANNEL_H__
#define __ALPHA_CHANNEL_H__

#include <ev.h>
#include <list>
#include "util.h"
#include "message_queue.h"

namespace alpha{

class Channel{
public:
	Channel(int fd, struct ev_io* io_watcher);
	virtual ~Channel();

	int Fd(){return fd_;}

	struct ev_io* GetIoWatcher(){return io_watcher_;}
	void SetIoWatcher(struct ev_io* io_watcher){io_watcher_ = io_watcher;}

	bool empty(){return mq_.empty();}
	void push_back(SP_MessageData mq){mq_.push_back(mq);}
	SP_MessageData pop_front(){
		if (mq_.empty()){
			return nullptr;
		}
		SP_MessageData queue = mq_.front();
		mq_.pop_front();
		return queue;
	}

protected:
	int fd_;
	//int fd_type_;
private:
	struct ev_io* io_watcher_;

	std::list<SP_MessageData> mq_;
};

}

#endif

