/*
Leon Lin in Lab408, NTU CSIE, 2024/01/08

AID02 Contract Implementation

FUNCTIONS
PURE:readSign(aid)
sign(aid, password, message)
registerNewUser(aid, password)

COMMENTS
only use password can not used in prod, but it's ok in this example

*/
#include <ourcontract.h>
#include <json.hpp>
#include <string>
#include <map>

using json = nlohmann::json;

enum Command
{
    get,
    readSign,
    sign,
    registerNewUser
};

static std::unordered_map<std::string, Command> const string2Command = {
    {"get", Command::get},
    {"readSign", Command::readSign},
    {"sign", Command::sign},
    {"registerNewUser", Command::registerNewUser}};

/**
 * data structure
 */

struct user
{
    std::string password;
    std::vector<std::string> signedMessages;
};

struct group
{
    std::map<std::string, user> users;
};

void to_json(json &j, const user &p)
{
    j = json{{"password", p.password}, {"signedMessages", p.signedMessages}};
}

void from_json(const json &j, user &p)
{
    j.at("password").get_to(p.password);
    j.at("signedMessages").get_to(p.signedMessages);
}

void to_json(json &j, const group &p)
{
    j = json{{"users", p.users}};
}

void from_json(const json &j, group &p)
{
    j.at("users").get_to(p.users);
}

/**
 * Main
 */
extern "C" int contract_main(void *arg)
{
    // cast argument
    ContractArguments *contractArg = (ContractArguments *)arg;
    ContractAPI *api = &contractArg->api;
    // init state
    if (api->readContractState() == "null")
    {
        group g;
        // write contract state
        std::string state = json(g).dump();
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
            api->generalContractInterfaceOutput("aid01", "0.2.0");
            return 0;
        }
    case Command::registerNewUser:
        if (contractArg->isPureCall)
        {
            return 0;
        }
        {
            // 1: aid, 2: password
            group g = json::parse(api->readContractState());
            if (g.users.find(contractArg->parameters[1]) != g.users.end())
            {
                api->contractLog("user exist");
                return 0;
            }
            user u;
            u.password = contractArg->parameters[2];
            g.users[contractArg->parameters[1]] = u;
            std::string state = json(g).dump();
            api->writeContractState(&state);
            return 0;
        }
    case Command::readSign:
        if (!contractArg->isPureCall)
        {
            return 0;
        }
        {
            // 1: aid
            group g = json::parse(api->readContractState());
            if (g.users.find(contractArg->parameters[1]) == g.users.end())
            {
                api->contractLog("user not exist");
                return 0;
            }
            user u = g.users[contractArg->parameters[1]];
            std::string state = json(u.signedMessages).dump();
            api->writeContractState(&state);
            return 0;
        }
    case Command::sign:
        if (contractArg->isPureCall)
        {
            return 0;
        }
        {
            // 1: aid, 2: password, 3: message
            group g = json::parse(api->readContractState());
            if (g.users.find(contractArg->parameters[1]) == g.users.end())
            {
                api->contractLog("user not exist");
                return 0;
            }
            user u = g.users[contractArg->parameters[1]];
            if (u.password != contractArg->parameters[2])
            {
                api->contractLog("password error");
                return 0;
            }
            u.signedMessages.push_back(contractArg->parameters[3]);
            g.users[contractArg->parameters[1]] = u;
            std::string state = json(g).dump();
            api->writeContractState(&state);
            return 0;
        }
    default:
        return 0;
    }
    return 0;
}