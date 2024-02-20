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
#define CONTRACT_DAEMON 4
#define CONTRACT_DAEMON_CLIENT 5

#include <json.hpp>
#include <stdbool.h>
#include <stdint.h>
#include <string>
#include <unistd.h>


using json = nlohmann::json;

/* non-reentrant entry point of runtime */
int start_runtime(int argc, char** argv);

/* contract call can be nested */
int call_contract(const char* contract, int argc, char** argv);

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

/* Contract daemon */
std::string contract_daemon();
std::string daemon_client(std::string cmd);
void daemon_log(FILE* f, const char* format, ...);

/* shortcut for return general interface */
void general_interface_write(std::string protocol, std::string version);

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

#endif // BITCOIN_CONTRACT_OURCONTRACT_H
