#include "contract/updateStrategy.h"
#include "contract/ourcontract.h"
#include "contract/processing.h"
#include <boost/asio.hpp>
#include <future>
#include <stack>
#include <sys/wait.h>

static fs::path contracts_dir;

const static fs::path& GetContractsDir()
{
    if (!contracts_dir.empty()) return contracts_dir;

    contracts_dir = GetDataDir() / "contracts";
    fs::create_directories(contracts_dir);

    return contracts_dir;
}

static bool processContracts(std::stack<CBlockIndex*> realBlock, ContractStateCache& cache, const Consensus::Params consensusParams)
{
    while (realBlock.size() > 0) {
        auto tmpBlock = realBlock.top();
        realBlock.pop();
        CBlock* block = new CBlock();
        if (!ReadBlockFromDisk(*block, tmpBlock, consensusParams)) {
            return false;
        }
        // init tp
        boost::asio::thread_pool pool(4);
        // put task to thread pool
        for (const CTransactionRef& tx : block->vtx) {
            if (tx.get()->contract.action == contract_action::ACTION_NONE) {
                continue;
            }
            if (tx.get()->contract.action == contract_action::ACTION_NEW) {
                boost::asio::post(pool, [tx, &cache]() {
                    auto contract = tx.get()->contract;
                    // contract address
                    fs::path new_dir = GetContractsDir() / contract.address.GetHex();
                    fs::create_directories(new_dir);
                    std::ofstream contract_code(new_dir.string() + "/code.cpp");
                    contract_code.write(contract.code.c_str(), contract.code.size());
                    contract_code.close();
                    // compile contract
                    int pid, status;
                    pid = fork();
                    if (pid == 0) {
                        int fd = open((GetContractsDir().string() + "/err").c_str(),
                            O_WRONLY | O_APPEND | O_CREAT,
                            0664);
                        dup2(fd, STDERR_FILENO);
                        close(fd);
                        execlp("ourcontract-mkdll",
                            "ourcontract-mkdll",
                            GetContractsDir().string().c_str(),
                            contract.address.GetHex().c_str(),
                            NULL);
                        exit(EXIT_FAILURE);
                    }
                    waitpid(pid, &status, 0);
                    if (WIFEXITED(status) == false || WEXITSTATUS(status) != 0) {
                        LogPrintf("compile contract %s error\n", contract.address.GetHex());
                    }
                    // execute contract
                    int fd_error = open((GetContractsDir().string() + "/err").c_str(),
                        O_WRONLY | O_APPEND | O_CREAT,
                        0664);
                    dup2(fd_error, STDERR_FILENO);
                    close(fd_error);
                    LogPrintf("execute contract %s\n", contract.address.GetHex());
                    Env env;
                    env.rootContract = contract.address.GetHex();
                    env.isPure = false;
                    env.preTxid = tx.get()->GetHash().GetHex();
                    env.contractMapStateIndex[contract.address.GetHex()] = 0;
                    env.contractMapDllPath[contract.address.GetHex()] = GetContractsDir().string() + "/" + contract.address.GetHex() + "/code.so";
                    json jsonEnv = env;
                    std::vector<std::string> contractStates;
                    contractStates.push_back("{}");
                    json result;
                    if (start_runtime(jsonEnv, contractStates, result) != 0) {
                        LogPrintf("execute contract %s error\n", contract.address.GetHex());
                    }
                });
                continue;
            }
            assert(tx.get()->contract.action == contract_action::ACTION_CALL);
            boost::asio::post(pool, [tx, &cache]() {
                auto contract = tx.get()->contract;
                // exe contract
                int fd_error = open((GetContractsDir().string() + "/err").c_str(),
                    O_WRONLY | O_APPEND | O_CREAT,
                    0664);
                dup2(fd_error, STDERR_FILENO);
                close(fd_error);
                LogPrintf("execute contract %s\n", contract.address.GetHex());
                Env env;
                env.rootContract = contract.address.GetHex();
                env.isPure = false;
                env.preTxid = tx.get()->GetHash().GetHex();
                env.contractMapStateIndex[contract.address.GetHex()] = 0;
                env.contractMapDllPath[contract.address.GetHex()] = GetContractsDir().string() + "/" + contract.address.GetHex() + "/code.so";
                json jsonEnv = env;
                std::vector<std::string> contractStates;
                contractStates.push_back("{}");
                json result;
                if (start_runtime(jsonEnv, contractStates, result) != 0) {
                    LogPrintf("execute contract %s error\n", contract.address.GetHex());
                }
            });
        }
        pool.join();
        // release memory
        delete block;
    }
    return true;
}

UpdateStrategy* UpdateStrategyFactory::createUpdateStrategy(CChain& chainActive, ContractStateCache* cache)
{
    int curHeight = chainActive.Height();
    uint256 curHash = chainActive.Tip()->GetBlockHash();
    BlockCache::blockIndex firstBlock;
    if (!cache->getFirstBlockCache(firstBlock)) {
        return new UpdateStrategyRebuild();
    }
    int cacheHeight = firstBlock.blockHeight;
    uint256 cacheHash = firstBlock.blockHash;
    if (curHash == cacheHash && curHeight == cacheHeight) {
        return new UpdateStrategyUnDo();
    }
    if (curHash != cacheHash) {
        if (curHeight <= cacheHeight) {
            return new UpdateStrategyRollback();
        }
        // curHeight > cacheHeight
        return new UpdateStrategyContinue();
    }
    // curHash == cacheHash && curHeight != cacheHeight
    return new UpdateStrategyRollback();
}

bool UpdateStrategyRebuild::UpdateSnapShot(ContractStateCache& cache, SnapShot& snapShot, CChain& chainActive, const Consensus::Params consensusParams)
{
    cache.getSnapShot()->clear();
    cache.getBlockCache()->clear();
    auto realBlock = std::stack<CBlockIndex*>();
    for (CBlockIndex* pindex = chainActive.Tip(); pindex != nullptr; pindex = pindex->pprev) {
        int height = pindex->nHeight;
        uint256 hash = pindex->GetBlockHash();
        cache.pushBlock(BlockCache::blockIndex(hash, height));
        realBlock.push(pindex);
    }
    // process all contract in blocks
    return processContracts(realBlock, cache, consensusParams);
}

bool UpdateStrategyContinue::UpdateSnapShot(ContractStateCache& cache, SnapShot& snapShot, CChain& chainActive, const Consensus::Params consensusParams)
{
    auto realBlock = std::stack<CBlockIndex*>();
    auto tmpBlockIndex = std::stack<BlockCache::blockIndex>();
    BlockCache::blockIndex firstBlock;
    if (!cache.getFirstBlockCache(firstBlock)) {
        return false;
    }
    for (CBlockIndex* pindex = chainActive.Tip(); pindex != nullptr; pindex = pindex->pprev) {
        // push block index to cache
        int height = pindex->nHeight;
        uint256 hash = pindex->GetBlockHash();
        if (height == firstBlock.blockHeight && hash == firstBlock.blockHash) {
            // find pre chain state
            while (tmpBlockIndex.size() > 0) {
                BlockCache::blockIndex tmpBlock = tmpBlockIndex.top();
                tmpBlockIndex.pop();
                cache.pushBlock(tmpBlock);
            }
            break;
        }
        if (height < firstBlock.blockHeight) {
            // release memory
            while (realBlock.size() > 0) {
                realBlock.pop();
            }
            // can not find pre chain state, rollback
            UpdateStrategyRollback algo;
            return algo.UpdateSnapShot(cache, snapShot, chainActive, consensusParams);
        }
        tmpBlockIndex.push(BlockCache::blockIndex(hash, height));
        realBlock.push(pindex);
    }
    return processContracts(realBlock, cache, consensusParams);
}

bool UpdateStrategyRollback::UpdateSnapShot(ContractStateCache& cache, SnapShot& snapShot, CChain& chainActive, const Consensus::Params consensusParams)
{
    BlockCache::blockIndex firstBlock;
    if (!cache.getFirstBlockCache(firstBlock)) {
        return false;
    }
    auto checkPointInfoList = cache.getCheckPointList();
    std::vector<std::string> checkPointList;
    for (auto it = checkPointInfoList.begin(); it != checkPointInfoList.end(); it++) {
        checkPointList.push_back(it->tipBlockHash);
    }
    for (CBlockIndex* pindex = chainActive.Tip(); pindex != nullptr; pindex = pindex->pprev) {
        int height = pindex->nHeight;
        uint256 hash = pindex->GetBlockHash();
        // try to match reacent same block
        if (height > firstBlock.blockHeight) {
            continue;
        } else if (height == firstBlock.blockHeight) {
            if (hash == firstBlock.blockHash) {
                // check point exist
                if (std::find(checkPointList.begin(), checkPointList.end(), hash.ToString()) != checkPointList.end()) {
                    // restore check point
                    if (!cache.restoreCheckPoint(hash.ToString(), checkPointInfoList)) {
                        LogPrintf("rollback error can not continue in checkPoint \n");
                        assert(false);
                    }
                    UpdateStrategyContinue algo;
                    return algo.UpdateSnapShot(cache, snapShot, chainActive, consensusParams);
                }
            }
            cache.popBlock();
            if (!cache.getFirstBlockCache(firstBlock)) {
                // block is empty now
                UpdateStrategyRebuild algo;
                return algo.UpdateSnapShot(cache, snapShot, chainActive, consensusParams);
            }
            continue;
        } else {
            // cache index should not bigger than chain index
            while (firstBlock.blockHeight >= height) {
                cache.popBlock();
                if (!cache.getFirstBlockCache(firstBlock)) {
                    // block is empty now
                    UpdateStrategyRebuild algo;
                    return algo.UpdateSnapShot(cache, snapShot, chainActive, consensusParams);
                }
            }
        }
    }
    UpdateStrategyRebuild algo;
    return algo.UpdateSnapShot(cache, snapShot, chainActive, consensusParams);
}

bool UpdateStrategyUnDo::UpdateSnapShot(ContractStateCache& cache, SnapShot& snapShot, CChain& chainActive, const Consensus::Params consensusParams)
{
    // default is todo nothing
    return true;
}
