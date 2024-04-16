#ifndef CONTRACT_SERVER_H
#define CONTRACT_SERVER_H

#define CONTRACT_SERVER_THREADS 4

#include <json/json.hpp>
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

extern ContractServer* server;

struct zmqMsg {
    string address;
    vector<string> parameters;
    bool isPure;
};

bool contractServerInit();
bool startContractServer();
bool stopContractServer();
bool interruptContractServer();

struct ContractAPI {
    // read contract state
    std::function<bool(string*, string*)> readContractState;
    // write contract state
    std::function<bool(string*, string*)> writeContractState;
    // print log
    std::function<void(string)> contractLog;
};

struct ContractArguments {
    // contract api interface
    ContractAPI api;
    // contract address
    string address;
    // is pure call contract or not
    bool isPureCall = false;
    // parameters
    vector<string> parameters = {};
};

#endif // CONTRACT_SERVER_H