#ifndef CONTRACT_PROCESSING_H
#define CONTRACT_PROCESSING_H

#include "chain.h"
#include "contract/cache.h"
#include "contract/contract.h"
#include "contract/dbWrapper.h"
#include "contract/observer.h"
#include "primitives/transaction.h"
#include "util.h"
#include "validation.h"

#include <leveldb/db.h>
#include <queue>
#include <string>
#include <vector>

#define BYTE_READ_STATE 0
#define BYTE_WRITE_STATE 1
#define CHECK_RUNTIME_STATE 2
#define GET_PRE_TXID_STATE 3
#define CONTRACT_DAEMON 4
#define CONTRACT_DAEMON_CLIENT 5

extern std::queue<int> cont_daemon_q;

// 在真實環境中執行合約且存儲數據
bool ProcessContract(const Contract& contract, const CTransactionRef& curTx, ContractStateCache* cache);

// 在暫存快照執行合約, 存儲數據不會被寫入, 會輸出給用戶
std::string call_rt_pure(ContractDBWrapper* cache, const uint256& contract, const std::vector<std::string>& args);

// Contract daemon
void StopContractDaemon();

#endif // CONTRACT_PROCESSING_H
