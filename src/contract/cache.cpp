#include "contract/cache.h"

SnapShot::SnapShot(std::string name)
{
    dbWrapper = new ContractDBWrapper(name);
}
SnapShot::~SnapShot()
{
    delete dbWrapper;
}

void SnapShot::setContractState(uint256 address, json state)
{
    dbWrapper->setState(address.ToString(), state.dump());
    assert(dbWrapper->isOk());
}
json SnapShot::getContractState(uint256 address)
{
    std::string state = dbWrapper->getState(address.ToString());
    if (dbWrapper->isOk() == false)
        return nullptr;
    return json::parse(state);
}
void SnapShot::clear()
{
    dbWrapper->clearAllStates();
}
void SnapShot::saveCheckPoint(std::string tipBlockHash)
{
    dbWrapper->saveCheckPoint(tipBlockHash);
}
void SnapShot::saveTmpState()
{
    dbWrapper->saveTmpState();
}
bool SnapShot::isCheckPointExist(std::string tipBlockHash)
{
    return dbWrapper->findCheckPoint(tipBlockHash);
}
ContractDBWrapper* SnapShot::getDBWrapper()
{
    return dbWrapper;
}

BlockCache::BlockCache()
{
    dbWrapper = new ContractDBWrapper(std::string("block_index"));
}
BlockCache::~BlockCache()
{
    delete dbWrapper;
}

void BlockCache::clear()
{
    dbWrapper->clearAllStates();
}
void BlockCache::setBlockIndex(uint256 blockHash, int blockHeight)
{
    dbWrapper->setState(intToKey(blockHeight), blockHash.ToString());
    assert(dbWrapper->isOk());
}
uint256 BlockCache::getBlockHash(int blockHeight)
{
    std::string blockHash = dbWrapper->getState(intToKey(blockHeight));
    if (dbWrapper->isOk() == false)
        return uint256();
    return uint256S(blockHash);
}
BlockCache::blockIndex BlockCache::getHeighestBlock()
{
    leveldb::Iterator* it = dbWrapper->getIterator();
    // 定位到数据库的最后一个条目
    it->SeekToLast();
    if (it->Valid() == false) {
        delete it;
        return blockIndex{uint256(), -1};
    }
    blockIndex result = blockIndex(uint256S(it->value().ToString()), keyToInt(it->key().ToString()));
    delete it;
    return result;
}
void BlockCache::removeBlockIndex(int blockHeight)
{
    dbWrapper->deleteState(intToKey(blockHeight));
    assert(dbWrapper->isOk());
}
ContractStateCache::ContractStateCache()
{
    blockCache = new BlockCache();
    snapShot = new SnapShot(std::string("current_cache"));
}
ContractStateCache::~ContractStateCache()
{
    delete blockCache;
    delete snapShot;
}
SnapShot* ContractStateCache::getSnapShot()
{
    return snapShot;
}
void ContractStateCache::clearSnapShot()
{
    snapShot->clear();
}
bool ContractStateCache::getFirstBlockCache(BlockCache::blockIndex& blockIndex)
{
    blockIndex = blockCache->getHeighestBlock();
    if (blockIndex.blockHeight == -1)
        return false;
    return true;
}
BlockCache* ContractStateCache::getBlockCache()
{
    return blockCache;
}
void ContractStateCache::pushBlock(BlockCache::blockIndex blockIndex)
{
    blockCache->setBlockIndex(blockIndex.blockHash, blockIndex.blockHeight);
}
void ContractStateCache::popBlock()
{
    BlockCache::blockIndex blockIndex = blockCache->getHeighestBlock();
    blockCache->removeBlockIndex(blockIndex.blockHeight);
}
void ContractStateCache::saveCheckPoint()
{
    auto blockIndex = blockCache->getHeighestBlock();
    if (snapShot->isCheckPointExist(blockIndex.blockHash.ToString()))
        return;
    if (isSaveCheckPointNow(blockIndex.blockHeight))
        snapShot->saveCheckPoint(blockIndex.blockHash.ToString());
}
void ContractStateCache::saveTmpState()
{
    if (isSaveReadReplicaNow(blockCache->getHeighestBlock().blockHeight))
        snapShot->saveTmpState();
}
bool ContractStateCache::restoreCheckPoint()
{
    auto blockIndex = blockCache->getHeighestBlock();
    if (snapShot->isCheckPointExist(blockIndex.blockHash.ToString())) {
        auto checkSnapShot = ContractDBWrapper(std::string("checkPoint/") + blockIndex.blockHash.ToString());
        return true;
    }
    return false;
}