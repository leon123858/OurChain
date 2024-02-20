#include <ourcontract.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

extern "C" int contract_main(int argc, char **argv)
{
  // pure mode
  if (!check_runtime_can_write_db())
  {
    std::cerr << "runtime is pure mode" << std::endl;
    std::string command = argv[1];
    // general interface
    if (command == "get")
    {
      general_interface_write("sample", "0.0.1");
      return 0;
    }
    // pure operation
    json j = state_read();
    std::cerr << "get state: " << j.dump() << std::endl;
    std::cerr << "pre txid: " << get_pre_txid() << std::endl;
    // some operation
    j.push_back("pure click: " + std::to_string((size_t)j.size()));
    state_write(j);
    return 0;
  }
  // call contract state
  if (state_exist())
  {
    json j = state_read();
    std::cerr << "get state: " << j.dump() << std::endl;
    std::cerr << "pre txid: " << get_pre_txid() << std::endl;
    // some operation
    j.push_back("more click: " + std::to_string((size_t)j.size()));
    state_write(j);
    return 0;
  }
  // init state
  std::cerr << "read state error" << std::endl;
  std::cerr << "pre txid: " << get_pre_txid() << std::endl;
  json j;
  j.push_back("baby cute");
  j.push_back(1);
  j.push_back(true);
  state_write(j);
  return 0;
}
