/*
Leon Lin in Lab408, NTU CSIE, 2024/6/3

Aid Store Contract Implementation

FUNCTIONS
// Returns the peer result list.
PURE:getResult() -> [](request)
// Returns the buyer request list.
PURE:getRequest() -> [](request)
// response of a request
allowRequest(aid, txid)
// user buy a thing
buyThing(aid, txid)
// Set a store.
setProduct(aid)
// Set a bank.
setBank(aid)

COMMENTS
this is just a demo for aid store, not a real store
*/

#include <ourcontract.h>
#include <json.hpp>
#include <string>
#include <map>

using json = nlohmann::json;

enum Command
{
  get,
  getResult,
  getRequest,
  allowRequest,
  buyThing,
  setProduct,
  setBank
};

static std::unordered_map<std::string, Command> const string2Command = {
    {"get", Command::get},
    {"getResult", Command::getResult},
    {"getRequest", Command::getRequest},
    {"allowRequest", Command::allowRequest},
    {"buyThing", Command::buyThing},
    {"setProduct", Command::setProduct},
    {"setBank", Command::setBank}};

/**
 * data structure
 */

struct request
{
  std::string aid;
  std::string txid;
};

struct store
{
  std::vector<request> requests;
  std::vector<request> allowRequests;
  std::string product;
  std::string bank;
};

void to_json(json &j, const request &p)
{
  j = json{{"aid", p.aid}, {"txid", p.txid}};
}

void from_json(const json &j, request &p)
{
  j.at("aid").get_to(p.aid);
  j.at("txid").get_to(p.txid);
}

void to_json(json &j, const store &p)
{
  j = json{{"requests", p.requests}, {"allowRequests", p.allowRequests}, {"product", p.product}, {"bank", p.bank}};
}

void from_json(const json &j, store &p)
{
  j.at("requests").get_to(p.requests);
  j.at("allowRequests").get_to(p.allowRequests);
  j.at("product").get_to(p.product);
  j.at("bank").get_to(p.bank);
}

/**
 * store
 */
extern "C" int contract_main(void *arg)
{
  // cast argument
  ContractArguments *contractArg = (ContractArguments *)arg;
  ContractAPI *api = &contractArg->api;
  // init state
  if (api->readContractState() == "null")
  {
    store s;
    // write contract state
    std::string state = json(s).dump();
    api->writeContractState(&state);
    return 0;
  }
  // execte command
  if (contractArg->parameters.size() < 1)
  {
    api->contractLog("arg count error");
    return 0;
  }
  std::string command = contractArg->parameters[0];
  auto eCommand = string2Command.find(command);
  if (eCommand == string2Command.end())
  {
    api->contractLog("command error");
    return 0;
  }
  switch (eCommand->second)
  {
  case Command::get:
    if (!contractArg->isPureCall)
    {
      return 0;
    }
    {
      api->generalContractInterfaceOutput("store02", "0.1.0");
      return 0;
    }
  case Command::getResult:
    if (!contractArg->isPureCall)
    {
      return 0;
    }
    {
      store s = json::parse(api->readContractState());
      // for each request, check if it is allowed by the store and bank
      std::vector<request> result;
      for (auto r : s.requests)
      {
        bool isAllowedByBank = false;
        bool isAllowedByStore = false;
        for (auto ar : s.allowRequests)
        {
          if (ar.txid == r.txid)
          {
            if (ar.aid == s.bank)
            {
              isAllowedByBank = true;
            }
            if (ar.aid == s.product)
            {
              isAllowedByStore = true;
            }
          }
          if (isAllowedByBank && isAllowedByStore)
          {
            result.push_back(r);
            break;
          }
        }
      }
      std::string state = json(result).dump();
      api->writeContractState(&state);
      return 0;
    }
  case Command::getRequest:
    if (!contractArg->isPureCall)
    {
      return 0;
    }
    {
      store s = json::parse(api->readContractState());
      std::string state = json(s.requests).dump();
      api->writeContractState(&state);
      return 0;
    }
  case Command::allowRequest:
    if (contractArg->isPureCall)
    {
      return 0;
    }
    {
      if (contractArg->parameters.size() < 3)
      {
        api->contractLog("arg count error");
        return 0;
      }
      store s = json::parse(api->readContractState());
      request r;
      r.aid = contractArg->parameters[1];  // bank or store's aid
      r.txid = contractArg->parameters[2]; // request's txid
      s.allowRequests.push_back(r);
      std::string state = json(s).dump();
      api->writeContractState(&state);
      break;
    }
  case Command::buyThing:
    if (contractArg->isPureCall)
    {
      return 0;
    }
    {
      if (contractArg->parameters.size() < 3)
      {
        api->contractLog("arg count error");
        return 0;
      }
      store s = json::parse(api->readContractState());
      request r;
      r.aid = contractArg->parameters[1];  // buyer's aid
      r.txid = contractArg->parameters[2]; // request's txid
      s.requests.push_back(r);
      std::string state = json(s).dump();
      api->writeContractState(&state);
      break;
    }
  case Command::setProduct:
    if (contractArg->isPureCall)
    {
      return 0;
    }
    {
      if (contractArg->parameters.size() < 2)
      {
        api->contractLog("arg count error");
        return 0;
      }
      store s = json::parse(api->readContractState());
      s.product = contractArg->parameters[1]; // product's aid
      std::string state = json(s).dump();
      api->writeContractState(&state);
      break;
    }
  case Command::setBank:
    if (contractArg->isPureCall)
    {
      return 0;
    }
    {
      if (contractArg->parameters.size() < 2)
      {
        api->contractLog("arg count error");
        return 0;
      }
      store s = json::parse(api->readContractState());
      s.bank = contractArg->parameters[1]; // bank's aid
      std::string state = json(s).dump();
      api->writeContractState(&state);
      break;
    }
  default:
    api->contractLog("command error");
    break;
  }

  return 0;
}
