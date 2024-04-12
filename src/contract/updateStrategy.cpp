#include "contract/updateStrategy.h"
#include "contract/processing.h"
#include <stack>

static bool processContracts(std::stack<CBlockIndex*> realBlock, ContractStateCache& cache, const Consensus::Params consensusParams)
{
    while (realBlock.size() > 0) {
        auto tmpBlock = realBlock.top();
        realBlock.pop();
        CBlock* block = new CBlock();
        if (!ReadBlockFromDisk(*block, tmpBlock, consensusParams)) {
            return false;
        }
        for (const CTransactionRef& tx : block->vtx) {
            if (!ProcessContract(tx.get()->contract, tx, &cache)) {
                LogPrintf("contract process error: %s\n", tx.get()->contract.address.ToString());
            }
        }
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
