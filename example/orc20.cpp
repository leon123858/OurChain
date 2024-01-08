/*
Leon Lin in Lab408, NTU CSIE, 2023/12/31

ORC20 Contract Implementation
(ERC20 Compatible)

FUNCTIONS
PURE:totalSupply()
// Returns the amount of tokens in existence.
PURE:balanceOf(account)
// Returns the amount of tokens owned by account.
transfer(recipient, amount)
// Moves amount tokens from the caller’s account to recipient.
PURE:allowance(owner, spender)
// Returns the remaining number of tokens that spender will be allowed to spend on behalf of owner through transferFrom. This is zero by default.
approve(spender, amount)
// Sets amount as the allowance of spender over the caller’s tokens.
transferFrom(sender, recipient, amount)
// Moves amount tokens from sender to recipient using the allowance mechanism. amount is then deducted from the caller’s allowance.

COMMENTS
approve, transferFrom, allowance 用來花費別人的 TOKEN

*/
#include <ourcontract.h>
#include <iostream>
#include <json.hpp>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

using json = nlohmann::json;

enum Command {
    totalSupply,
    balanceOf,
    transfer,
    allowance,
    approve,
    transferFrom
};

static std::unordered_map<std::string,Command> const string2Command = { {"totalSupply",Command::totalSupply}, {"balanceOf",Command::balanceOf}, {"transfer",Command::transfer}, {"allowance",Command::allowance}, {"approve",Command::approve}, {"transferFrom",Command::transferFrom} };
static std::string aidContractAddress = "f40ee8930eb5c30211e3691f37576c259121d673c1b55bc97e81ec518048c6da";

char* getDynamicString(const char* str){
    char* ret = (char*)malloc(sizeof(char) * strlen(str));
    strcpy(ret, str);
    return ret;
}

void removeDynamicStrings(char** argv){
    for(int i=0;i<4;i++){
        delete[] argv[i];
    }
    delete[] argv;
}

bool verifyUser(std::string aid, std::string password){
    // check if aid is valid
    char **subArgv = (char **)malloc(sizeof(char *) * 4);
    // put aidContractAddress into argv[0]
    subArgv[0] = getDynamicString(aidContractAddress.c_str());
    subArgv[1] = getDynamicString("verify");
    subArgv[2] = getDynamicString(aid.c_str());
    subArgv[3] = getDynamicString(password.c_str());
    int ret = call_contract(aidContractAddress.c_str(), 4, subArgv);
    removeDynamicStrings(subArgv);
    if(ret != 0){
        err_printf("aid contract exe error\n");
        return false;
    }
    json verifyResponse = pre_state_read();
    if(!verifyResponse["isExist"].get<bool>()){
        err_printf("aid not exist\n");
        return false;
    }
    if(!verifyResponse["result"].get<bool>()){
        err_printf("aid password error\n");
        return false;
    }
    return true;
}

extern "C" int contract_main(int argc, char **argv) {
  // init state
  if(!state_exist()){
    json j = json::array();
    std::string coinName = argv[1];
    int count = atoi(argv[2]);
    std::string aid = argv[3];
    for(int i=0;i<count;i++){
        json tmp;
        tmp["coinName"] = coinName;
        tmp["id"] = i;
        tmp["owner"] = aid;
        j.push_back(tmp);
    }
    state_write(j);
    return 0;
  }
  // execute command
  if (argc == 1) {
      std::cerr << "argc error" << std::endl;
      return 0;
  }
  std::string command = argv[1];
  auto eCommand = string2Command.find(command);
  if (eCommand == string2Command.end()) {
      std::cerr << "command error" << std::endl;
      return 0;
  }
  switch (eCommand->second)
  {
  case Command::totalSupply:
    if(check_runtime_can_write_db()){
        return 0;
    }
    state_write(state_read());
    break;
  case Command::balanceOf:
    if(check_runtime_can_write_db()){
        return 0;
    }
    {
        std::string aid = argv[2];
        json j = state_read();
        json ownList = json::array();
        for (auto &it : j) {
            if (it["owner"] == aid) {
                ownList.push_back(it);
            }
        }
        state_write(ownList);
    }
    break;
  case Command::transfer:
    if(!check_runtime_can_write_db()){
      return 0;
    }
    {
        // get argv
        std::string aid = argv[2];
        std::string targetAid = argv[3];
        int count = atoi(argv[4]);
        std::string password = argv[5];
        // check if aid is valid
        if(!verifyUser(aid, password)){
            return 0;
        }
        // start execute transfer
        json j = state_read();
        json ownList = json::array();
        for (auto &it : j) {
            if (it["owner"] == aid) {
                ownList.push_back(it);
            }
        }
        if (static_cast<int>(ownList.size()) < count) {
            err_printf("not enough token\n");
            return 0;
        }
        for(int i=0;i<count;i++){
            for(auto &it : j){
                if(it["owner"] == aid){
                    it["owner"] = targetAid;
                    break;
                }
            }
        }
        state_write(j);
    }
    break;
  case Command::allowance:
    if(!check_runtime_can_write_db()){
      return 0;
    }
    err_printf("allowance not implemented\n");
    break;
  case Command::approve:
    if(!check_runtime_can_write_db()){
      return 0;
    }
    err_printf("approve not implemented\n");
    break;
  case Command::transferFrom:
    if(!check_runtime_can_write_db()){
      return 0;
    }
    err_printf("transferFrom not implemented\n");
    break;
  default:
    break;
  }
  return 0;
}