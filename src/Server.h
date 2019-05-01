#ifndef SERVER_H_INCLUDED
#define SERVER_H_INCLUDED

#include <memory>

#include "NetworkManagerServer.h"

#include "GameInstance.h"

class Server : public GameInstance {
	friend class NetworkManagerServer;

  public:
	//todo: pass world settings to constructor
	Server(bool _allowConnections);
	~Server();

	void receivePackets();

	void update(int _timeslice);

	bool connectionsAllowed();

  public:
	NetworkManagerServer m_networkManager;
	bool                 m_allowConnections;
};

#endif // SERVER_H_INCLUDED
