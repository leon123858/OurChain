#!/bin/bash
# Leon Lin for Ourchain

mining() {
    ./src/bitcoin-cli getnewaddress > address.txt
    ./src/bitcoin-cli generatetoaddress $1 $(cat address.txt)
    sleep 5
    rm address.txt
}

deploycontract() {
    ./src/bitcoin-cli deploycontract ~/Desktop/ourchain/sample.cpp > log.txt
    # 使用 grep 和 awk 从 log.txt 文件中提取合同地址
    contract_address=$(grep "contract address" log.txt | awk -F'"' '{print $4}')
    rm log.txt
    echo "$contract_address"
}

make -j8 && make install && ldconfig
./src/bitcoind --regtest --daemon -txindex
sleep 5
mining 11
contract_address=$(deploycontract)
echo "contract: $contract_address"
mining 1
./src/bitcoin-cli callcontract "$contract_address" ""
mining 2
./src/bitcoin-cli dumpcontractmessage "$contract_address" ""
# 一般化合約介面
./src/bitcoin-cli dumpcontractmessage "$contract_address" "get"
./src/bitcoin-cli stop