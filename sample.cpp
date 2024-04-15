#include <iostream>
#include <json.hpp>

using json = nlohmann::json;

struct ContractAPI
{
  // read contract state
  bool (*readContractState)(std::string *state, std::string *address);
  // write contract state
  bool (*writeContractState)(std::string *state, std::string *address);
};

extern "C" int contract_main(json *arg, void *apiInstance)
{
  // cast apiInstance to struct ContractAPI
  struct ContractAPI *api = (struct ContractAPI *)apiInstance;
  // read contract state
  std::string state;
  std::string address = arg->at("address");
  if (!api->readContractState(&state, &address))
  {
    return 1;
  }
  if (state.empty())
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
    (*arg)["state"] = j;
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
    (*arg)["state"] = j;
  }
  return 0;
}
