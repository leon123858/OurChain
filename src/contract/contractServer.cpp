#include "contract/contractServer.h"
#include "util.h"
#include "validation.h"
#include <atomic>
#include <condition_variable>
#include <dlfcn.h>
#include <json.hpp>
#include <sys/syscall.h>
#include <sys/sysinfo.h>
#include <unistd.h>
#include <zmq.hpp>

using namespace std;
using namespace zmq;
using namespace nlohmann;

atomic<bool> stopFlag(false);

thread_local struct ContractAPI apiInstance = {
    .readContractState = nullptr,
    .writeContractState = nullptr,
};

static bool readContractCache(string* state, string* hex_ctid)
{
    json j = contractStateCache.getSnapShot()->getContractState(*hex_ctid);
    *state = j.dump();
    return true;
}

static bool writeContractCache(string* state, string* hex_ctid)
{
    json j = json::parse(*state);
    contractStateCache.getSnapShot()->setContractState(uint256S(*hex_ctid), j);
    return true;
}

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

static void responseZmqMessage(zmq::socket_t& socket, const string& response)
{
    zmq::message_t message(response.size());
    memcpy(message.data(), response.c_str(), response.size());
    socket.send(message, zmq::send_flags::none);
}

void ContractServer::workerThread()
{
    RenameThread("contract-worker");
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
            auto address = recv_msg.c_str();
            auto contractPath = contractStateCache.getContractPath(address) / "code.so";
            if (!fs::exists(contractPath)) {
                LogPrintf("Contract not found: %s\n", contractPath.string());
                responseZmqMessage(puller, "FAILED");
                continue;
            }
            // execute contract
            void* handle = dlopen(contractPath.c_str(), RTLD_LAZY);
            if (!handle) {
                LogPrintf("Failed to load contract: %s\n", dlerror());
                responseZmqMessage(puller, "FAILED");
                continue;
            }
            int (*contract_main)(json*, ContractAPI*) = (int (*)(json*, ContractAPI*))dlsym(handle, "contract_main");
            if (!contract_main) {
                LogPrintf("Failed to load contract_main: %s\n", dlerror());
                responseZmqMessage(puller, "FAILED");
                dlclose(handle);
                continue;
            }
            apiInstance.readContractState = readContractCache;
            apiInstance.writeContractState = writeContractCache;
            json arg = json::object();
            arg["address"] = address;
            arg["state"] = json::object();
            try {
                int ret = contract_main(&arg, &apiInstance);
                if (ret != 0) {
                    throw runtime_error("Contract execution failed");
                }
            } catch (const std::exception& e) {
                LogPrintf("Failed to get contract state: %s\n", e.what());
                responseZmqMessage(puller, "FAILED");
                dlclose(handle);
                continue;
            }
            // send state
            responseZmqMessage(puller, arg["state"].dump());
            dlclose(handle);
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
    RenameThread("contract-proxy");
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