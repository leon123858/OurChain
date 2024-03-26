#include <ourcontract.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

extern "C" int contract_main(Env env, json *state)
{
  if (env.isPure)
  {
    std::cerr << "Pure contract\n";
    return 0;
  }
  std::cerr << "Not pure contract\n";
  return 0;
}
