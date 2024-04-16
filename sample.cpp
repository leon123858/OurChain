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
  // pure call contract
  if (contractArg->isPureCall)
  {
    string command = contractArg->parameters[0];
    api->contractLog("command: " + command);
    if (command == "get")
    {
      json j;
      j["name"] = "example";
      j["version"] = "0.1.0";
      // write contract state
      auto state = j.dump();
      if (!api->writeContractState(&state, &contractArg->address))
      {
        return 1;
      }
      return 0;
    }
    else
    {
      // pure operation
      string state = "";
      if (!api->readContractState(&state, &contractArg->address))
      {
        return 1;
      }
      json j = json::parse(state);
      j.push_back("pure click: " + std::to_string((size_t)j.size()));
      // pure output
      state = j.dump();
      if (!api->writeContractState(&state, &contractArg->address))
      {
        return 1;
      }
      return 0;
    }
  }
  // non-pure call contract
  string state = "";
  if (!api->readContractState(&state, &contractArg->address))
  {
    return 1;
  }
  if (state == "null")
  {
    json j;
    j.push_back("baby cute");
    j.push_back(1);
    j.push_back(true);
    // write contract state
    state = j.dump();
    if (!api->writeContractState(&state, &contractArg->address))
    {
      return 1;
    }
    return 0;
  }
  json j = json::parse(state);
  j.push_back("more click: " + std::to_string((size_t)j.size()));
  // write contract state
  state = j.dump();
  if (!api->writeContractState(&state, &contractArg->address))
  {
    return 1;
  }
  return 0;
}
