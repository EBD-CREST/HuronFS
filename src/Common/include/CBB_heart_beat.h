#ifndef CBB_HEART_BEAT_H_
#define CBB_HEART_BEAT_H_

#include "CBB_task_parallel.h"
#include "CBB_communication_thread.h"

namespace CBB
{
	namespace Common
	{
		class CBB_heart_beat
		{
			public:
				//socket, IOnode_id
				typedef std::map<int, ssize_t> socket_map_t;
				CBB_heart_beat();
				CBB_heart_beat(communication_queue_t* input_queue,
						communication_queue_t* output_queue);	
				virtual ~CBB_heart_beat() = default;	

				void set_task_parallel_queue(communication_queue_t* input_queue,
						communication_queue_t* output_queue);
				int heart_beat_check();

				void heart_beat_func();

				int send_heart_beat_check(int socket);
				virtual int get_IOnode_socket_map(socket_map_t& map)=0;
			private:
				CBB_heart_beat(const CBB_heart_beat&) = delete;	
				CBB_heart_beat& operator=(const CBB_heart_beat&) = delete;

			private:
				int keepAlive;
				communication_queue_t* input_queue;
				communication_queue_t* output_queue;
		};
	}
}

#endif
