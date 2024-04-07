#include "contract/contractServer.h"
#include "util.h"
#include <atomic>
#include <condition_variable>
#include <sys/syscall.h>
#include <unistd.h>
#include <zmq.hpp>

using namespace std;
using namespace zmq;

atomic<bool> stopFlag(false);

ContractServer::ContractServer()
{
    numThreads = CONTRACT_SERVER_THREADS;
    threadPool.reserve(numThreads);
    proxyThreadInstance = thread(&ContractServer::proxyThread, this);
}

ContractServer::~ContractServer()
{
    if (!threadPool.empty()) {
        stop();
    }
    LogPrintf("contract server destroyed\n");
}

void ContractServer::workerThread()
{
    context_t context(1);
    zmq::socket_t puller(context, zmq::socket_type::rep);
    puller.connect("tcp://127.0.0.1:5560");
    puller.set(zmq::sockopt::rcvtimeo, 2000);
    while (true) {
        zmq::message_t message;
        zmq::recv_result_t result = puller.recv(message, zmq::recv_flags::none);
        if (result) {
            std::string recv_msg(static_cast<char*>(message.data()), message.size());
            LogPrintf("Contract Received message: %s\n", recv_msg.c_str());
            // process message
            // TODO: process message
            // send response
            zmq::message_t response(2);
            memcpy(response.data(), "OK", 2);
            puller.send(response, zmq::send_flags::none);
        } else {
            if (stopFlag.load()) {
                break;
            }
        }
    }
    puller.close();
}

void ContractServer::proxyThread()
{
    context_t context(1);
    zmq::socket_t frontend(context, zmq::socket_type::router);
    zmq::socket_t backend(context, zmq::socket_type::dealer);
    frontend.bind("tcp://*:5559");
    backend.bind("tcp://*:5560");
    zmq::proxy(frontend, backend, nullptr);
    // close sockets
    frontend.close();
    backend.close();
}

bool ContractServer::start()
{
    stopFlag.store(false);
    for (int i = 0; i < numThreads; i++) {
        threadPool.emplace_back(&ContractServer::workerThread, this);
    }
    return true;
}

bool ContractServer::stop()
{
    for (auto& thread : threadPool) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    threadPool.clear();
    LogPrintf("contract server stopped\n");
    return true;
}

bool ContractServer::interrupt()
{
    // stop the worker threads
    stopFlag.store(true);
    return true;
}

bool contractServerInit()
{
    if (server != nullptr) {
        LogPrintf("contract server is running\n");
        return false;
    }
    LogPrintf("contract server initialized\n");
    server = new ContractServer();
    return true;
}

bool startContractServer()
{
    if (server == nullptr) {
        LogPrintf("contract server is not running\n");
        return false;
    }
    return server->start();
}

bool stopContractServer()
{
    if (stopFlag.load() == false && server != nullptr) {
        interruptContractServer();
    }
    if (!server->stop()) {
        LogPrintf("failed to stop contract server\n");
        return false;
    }
    delete server;
    server = nullptr;
    return true;
}

bool interruptContractServer()
{
    return server->interrupt();
}