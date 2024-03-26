#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include <boost/log/trivial.hpp>
#include <dlfcn.h>
#include <fcntl.h>
#include <linux/limits.h>
#include <stack>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "ourcontract.h"

void to_json(json& j, const Env& p)
{
    j = json{{"rootContract", p.rootContract}, {"isPure", p.isPure}, {"preTxid", p.preTxid}, {"contractMapStateIndex", p.contractMapStateIndex}, {"contractMapDllPath", p.contractMapDllPath}};
}
void from_json(const json& j, Env& p)
{
    j.at("rootContract").get_to(p.rootContract);
    j.at("isPure").get_to(p.isPure);
    j.at("preTxid").get_to(p.preTxid);
    j.at("contractMapStateIndex").get_to(p.contractMapStateIndex);
    j.at("contractMapDllPath").get_to(p.contractMapDllPath);
}

/* call stack */
ContractLocalState::ContractLocalState(std::string* stateStr)
{
    if (stateStr == nullptr) {
        this->state = json();
        return;
    }
    this->stateStr = stateStr;
    this->state = json::parse(*stateStr);
}

ContractLocalState::~ContractLocalState()
{
    if (this->stateStr != nullptr) {
        delete this->stateStr;
    }
}

json ContractLocalState::getState()
{
    return this->state;
}

void ContractLocalState::setState(json state)
{
    this->state = state;
}

bool ContractLocalState::isStateNull()
{
    return this->state.is_null();
}

void ContractLocalState::setPreState(json preState)
{
    this->preState = preState;
}

json ContractLocalState::getPreState()
{
    return this->preState;
}

std::string* physical_state_read(const char* contractAddress)
{
    return nullptr;
}

int physical_state_write(const std::string* buf)
{
    return 0;
}

bool check_runtime_can_write_db()
{
    return true;
}

int start_runtime(json& env, std::vector<std::string>& contractStates, json& result)
{
    std::stack<ContractLocalState*>* call_stack;
    Env e = env.get<Env>();
    if (call_contract(e.rootContract, contractStates, call_stack, e, result) != 0) {
        err_printf("start_runtime: call root contract failed\n");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int call_contract(const std::string& contract, std::vector<std::string>& contractStates, std::stack<ContractLocalState*>* call_stack, Env& env, json& result)
{
    std::string filePath = env.contractMapDllPath[contract];
    if (filePath.empty()) {
        err_printf("call_contract: contract %s not found\n", contract.c_str());
        return EXIT_FAILURE;
    }
    if (filePath.length() > PATH_MAX) {
        err_printf("call_contract: contract path too long\n");
        return EXIT_FAILURE;
    }
    void* handle = dlopen(filePath.c_str(), RTLD_LAZY);
    if (handle == NULL) {
        err_printf("call_contract: dlopen failed %s\n", dlerror());
        return EXIT_FAILURE;
    }
    int (*contract_main)(Env, json*) = reinterpret_cast<int (*)(Env, json*)>(dlsym(handle, "contract_main"));
    if (contract_main == NULL) {
        err_printf("call_contract: %s\n", dlerror());
        return EXIT_FAILURE;
    }
    auto curState = &contractStates[env.contractMapStateIndex[contract]];
    auto curStateJson = json::parse(*curState);
    int ret = contract_main(env, &curStateJson);
    if (ret != 0) {
        err_printf("call_contract: contract_main failed\n");
        return ret;
    }
    result = curStateJson;
    dlclose(handle);
    return ret;
}

int err_printf(const char* format, ...)
{
    va_list args;
    int ret;

    va_start(args, format);
    ret = vfprintf(stderr, format, args);

    va_end(args);
    return ret;
}

json state_read()
{
    return json::value_t::object;
}

void state_write(json buf)
{
}

bool state_exist()
{
    return false;
}

json pre_state_read()
{
    return json::value_t::object;
}

void general_interface_write(std::string protocol, std::string version)
{
    json j;
    j["protocol"] = protocol;
    j["version"] = version;
}

std::string get_pre_txid()
{
    return "";
}