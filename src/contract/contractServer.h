#ifndef CONTRACT_SERVER_H
#define CONTRACT_SERVER_H

#define CONTRACT_SERVER_THREADS 4

#include <thread>
#include <vector>
#include <zmq.hpp>

using namespace std;

class ContractServer
{
public:
    ContractServer();
    ~ContractServer();
    bool start();
    bool stop();
    bool interrupt();

private:
    int numThreads;
    vector<thread> threadPool;
    thread proxyThreadInstance;

    void workerThread();
    void proxyThread();
};

static ContractServer* server = nullptr;

bool contractServerInit();
bool startContractServer();
bool stopContractServer();
bool interruptContractServer();

#endif // CONTRACT_SERVER_H