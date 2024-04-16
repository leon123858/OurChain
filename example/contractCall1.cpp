#include <ourcontract.h>
#include <json.hpp>

using json = nlohmann::json;

extern "C" int contract_main(void *arg)
{
  // cast argument
  ContractArguments *contractArg = (ContractArguments *)arg;
  ContractAPI *api = &contractArg->api;
  // pure call contract
  if (contractArg->isPureCall)
  {
    api->contractLog("pure call contract1");
    auto contract2Address = contractArg->parameters[0];
    contractArg->address = contract2Address;
    // recursive call contract
    if (!api->recursiveCall(contractArg))
    {
      return 1;
    }
    auto preState = api->readPreContractState();
    api->writeContractState(&preState);
    return 0;
  }
  return 0;
}