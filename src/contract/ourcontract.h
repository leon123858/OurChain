#ifndef BITCOIN_CONTRACT_OURCONTRACT_H
#define BITCOIN_CONTRACT_OURCONTRACT_H

#include <functional>
#include <string>

using namespace std;

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
};

#endif // BITCOIN_CONTRACT_OURCONTRACT_H
