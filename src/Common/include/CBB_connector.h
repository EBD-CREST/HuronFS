#ifndef CBB_CONNECTOR_H_
#define CBB_CONNECTOR_H_

namespace CBB
{
	namespace Common
	{
		class CBB_connector
		{
			public:
				virtual ~CBB_connector();
				int _connect_to_server(const struct sockaddr_in& client_addr,
						const struct sockaddr_in& server_addr)const throw(std::runtime_error); 
		};
	}
}

#endif
