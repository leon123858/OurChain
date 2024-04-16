#include <iostream>
#include <json.hpp>
#include "ourcontract.h"

using json = nlohmann::json;
using namespace std;

// contract main function
extern "C" int contract_main(void *arg)
{
  // cast argument
  ContractArguments *contractArg = (ContractArguments *)arg;
  ContractAPI *api = &contractArg->api;
  // read contract state
  std::string state;
  std::string address = contractArg->address;
  if (!api->readContractState(&state, &address))
  {
    return 1;
  }
  api->contractLog("state: " + state);
  if (state == "null")
  {
    // create new state
    state = "{}";
    json j = json::parse(state);
    j["array"] = json::array();
    j["array"].push_back("init");
    state = j.dump();
    if (!api->writeContractState(&state, &address))
    {
      return 1;
    }
  }
  else
  {
    // update state
    json j = json::parse(state);
    j["array"].push_back("update" + std::to_string(j["array"].size()));
    state = j.dump();
    if (!api->writeContractState(&state, &address))
    {
      return 1;
    }
  }
  return 0;
}
