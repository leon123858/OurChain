#ifndef BITCOIN_CONTRACT_OURCONTRACT_H
#define BITCOIN_CONTRACT_OURCONTRACT_H

/* Origin config
#define BYTE_READ_STATE 0
#define BYTE_SEND_TO_ADDRESS -1
#define BYTE_SEND_TO_CONTRACT -2
#define BYTE_CALL_CONTRACT -3
*/

#define BYTE_READ_STATE 0
#define BYTE_WRITE_STATE 1
#define CHECK_RUNTIME_STATE 2
#define GET_PRE_TXID_STATE 3

#include <json/json.hpp>
#include <map>
#include <stack>
#include <stdbool.h>
#include <stdint.h>
#include <string>
#include <vector>

using json = nlohmann::json;

struct Env {
    std::string rootContract;
    bool isPure;
    std::string preTxid;
    std::map<std::string, int> contractMapStateIndex;
    std::map<std::string, std::string> contractMapDllPath;
};

class ContractLocalState
{
private:
    std::string* stateStr;
    json state;
    json preState;

public:
    ContractLocalState(std::string* stateStr);
    ~ContractLocalState();
    void setState(json state);
    void setPreState(json preState);
    json getPreState();
    json getState();
    bool isStateNull();
};

void to_json(json& j, const Env& p);
void from_json(const json& j, Env& p);

/* non-reentrant entry point of runtime */
int start_runtime(json& env, std::vector<std::string>& contractStates, json& result);

/* contract call can be nested */
int call_contract(const std::string& contract, std::vector<std::string>& contractStates, std::stack<ContractLocalState*>* call_stack, Env& env, json& result);

/* print to runtime error log */
int err_printf(const char* format, ...);

/* read the state file of the calling contract */
json state_read();

/* write the state file of the calling contract */
void state_write(json buf);

/* check if state exist*/
bool state_exist();

/* check if runtime can write db*/
bool check_runtime_can_write_db();

/* read pre state*/
json pre_state_read();

/* check pre txid for verify sign*/
std::string get_pre_txid();

/* shortcut for return general interface */
void general_interface_write(std::string protocol, std::string version, std::vector<std::string> dep = {});

#endif // BITCOIN_CONTRACT_OURCONTRACT_H
