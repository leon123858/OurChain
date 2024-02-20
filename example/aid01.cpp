/*
Leon Lin in Lab408, NTU CSIE, 2024/01/08

AID01 Contract Implementation

FUNCTIONS
PURE:login(name, password)
// Returns the user information of name.
PURE:verify(aid, password)
// Returns if password is correct.
registerNewUser(name, password)

COMMENTS
only use password can not used in prod, but it's ok in this example

*/
#include <ourcontract.h>
#include <iostream>
#include <json.hpp>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

using json = nlohmann::json;

enum Command
{
    get,
    login,
    verify,
    registerNewUser,
    getCoins,
    setCoin,
    removeCoin,
};

static std::unordered_map<std::string, Command> const string2Command = {{"get", Command::get}, {"login", Command::login}, {"verify", Command::verify}, {"registerNewUser", Command::registerNewUser}, {"getCoins", Command::getCoins}, {"setCoin", Command::setCoin}, {"removeCoin", Command::removeCoin}};

/**
 * data structure
 */

struct user
{
    std::string aid;
    std::string name;
    std::string password;
    std::vector<std::string> coins;
};

struct group
{
    std::vector<user> users;
};

void to_json(json &j, const user &p)
{
    j = json{{"aid", p.aid}, {"name", p.name}, {"password", p.password}, {"coins", p.coins}};
}

void from_json(const json &j, user &p)
{
    j.at("aid").get_to(p.aid);
    j.at("name").get_to(p.name);
    j.at("password").get_to(p.password);
    j.at("coins").get_to(p.coins);
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
extern "C" int contract_main(int argc, char **argv)
{
    // init state
    if (!state_exist())
    {
        group newGroup;
        state_write(newGroup);
    }
    // execute command
    if (argc == 1)
    {
        std::cerr << "argc error" << std::endl;
        return 0;
    }
    std::string command = argv[1];
    auto eCommand = string2Command.find(command);
    if (eCommand == string2Command.end())
    {
        std::cerr << "command error" << std::endl;
        return 0;
    }
    switch (eCommand->second)
    {
    case Command::get:
        if (check_runtime_can_write_db())
        {
            return 0;
        }
        {
            general_interface_write("aid", "0.0.1");
            return 0;
        }
    case Command::login:
        if (check_runtime_can_write_db())
        {
            return 0;
        }
        {
            std::string name = argv[2];
            std::string password = argv[3];
            group curGroup = state_read();
            for (auto &user : curGroup.users)
            {
                if (user.name == name && user.password == password)
                {
                    json tmp = json::object();
                    tmp["isExist"] = true;
                    tmp["aid"] = user.aid;
                    tmp["name"] = user.name;
                    state_write(tmp);
                    return 0;
                }
            }
            json tmp = json::object();
            tmp["isExist"] = false;
            state_write(tmp);
            return 0;
        }
    case Command::verify:
        if (check_runtime_can_write_db())
        {
            return 0;
        }
        {
            std::string aid = argv[2];
            std::string password = argv[3];
            group curGroup = state_read();
            for (auto &user : curGroup.users)
            {
                if (user.aid == aid)
                {
                    json tmp = json::object();
                    tmp["isExist"] = true;
                    if (user.password == password)
                    {
                        tmp["result"] = true;
                    }
                    else
                    {
                        tmp["result"] = false;
                    }
                    state_write(tmp);
                    return 0;
                }
            }
            json tmp = json::object();
            tmp["isExist"] = false;
            state_write(tmp);
            return 0;
        }
    case Command::registerNewUser:
        if (!check_runtime_can_write_db())
        {
            return 0;
        }
        {
            std::string name = argv[2];
            std::string password = argv[3];
            group curGroup = state_read();
            // create name generator
            boost::uuids::uuid baseUUID = boost::uuids::string_generator()("00000000-0000-0000-0000-000000000000");
            boost::uuids::name_generator gen(baseUUID);
            std::string newAid = to_string(gen(get_pre_txid() + name)); // "f1669cf2-1d6e-539c-9955-526fdd9f09e3"
            user newUser = user{newAid, name, password, std::vector<std::string>()};
            curGroup.users.push_back(newUser);
            state_write(curGroup);
        }
        break;
    case Command::getCoins:
        if (check_runtime_can_write_db())
        {
            return 0;
        }
        {
            std::string aid = argv[2];
            group curGroup = state_read();
            for (auto &user : curGroup.users)
            {
                if (user.aid == aid)
                {
                    json tmp = json::object();
                    tmp["isExist"] = true;
                    tmp["coins"] = user.coins;
                    state_write(tmp);
                    return 0;
                }
            }
            json tmp = json::object();
            tmp["isExist"] = false;
            state_write(tmp);
            break;
        }
    case Command::setCoin:
        if (!check_runtime_can_write_db())
        {
            return 0;
        }
        {
            std::string aid = argv[2];
            std::string coinAddress = argv[3];
            group curGroup = state_read();
            for (auto &user : curGroup.users)
            {
                if (user.aid == aid)
                {
                    user.coins.push_back(coinAddress);
                    state_write(curGroup);
                    return 0;
                }
            }
            break;
        }
    case Command::removeCoin:
        if (!check_runtime_can_write_db())
        {
            return 0;
        }
        {
            std::string aid = argv[2];
            std::string coinAddress = argv[3];
            group curGroup = state_read();
            for (auto &user : curGroup.users)
            {
                if (user.aid == aid)
                {
                    auto it = user.coins.begin();
                    while (it != user.coins.end())
                    {
                        if (*it == coinAddress)
                        {
                            it = user.coins.erase(it);
                        }
                        else
                        {
                            ++it;
                        }
                    }
                    state_write(curGroup);
                    return 0;
                }
            }
            break;
        }
    }
    return 0;
}