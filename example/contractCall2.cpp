#include <ourcontract.h>
#include <iostream>
#include <json.hpp>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

using json = nlohmann::json;

extern "C" int contract_main(void *arg)
{
  // cast argument
  ContractArguments *contractArg = (ContractArguments *)arg;
  ContractAPI *api = &contractArg->api;
  // pure call contract
  if (contractArg->isPureCall)
  {
    api->contractLog("pure call contract2");
    json j;
    j["key"] = "contract2 state";
    auto state = j.dump();
    if (!api->writeContractState(&state))
    {
      return 1;
    }
    return 0;
  }
  return 0;
}