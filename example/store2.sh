deploycontract() {
    bitcoin-cli deploycontract ~/Desktop/ourchain/example/store2.cpp > log.txt
    # 使用 grep 和 awk 从 log.txt 文件中提取合同地址
    contract_address=$(grep "contract address" log.txt | awk -F'"' '{print $4}')
    rm log.txt
    echo "$contract_address"
}

bitcoind --regtest --daemon -txindex
sleep 5
bitcoin-cli generate 3
contract_address=$(deploycontract)
echo "$contract_address"
bitcoin-cli generate 1
bitcoin-cli dumpcontractmessage "$contract_address" "get"
bitcoin-cli callcontract "$contract_address" "setProduct" "product-aid-demo"
bitcoin-cli callcontract "$contract_address" "setBank" "bank-aid-demo"
bitcoin-cli callcontract "$contract_address" "buyThing" "user-aid-demo" "demo-txid"
bitcoin-cli generate 1
echo "getRequest"
bitcoin-cli dumpcontractmessage "$contract_address" "getRequest"
echo "getResult"
bitcoin-cli dumpcontractmessage "$contract_address" "getResult"
echo "allow tx"
bitcoin-cli callcontract "$contract_address" "allowRequest" "bank-aid-demo" "demo-txid"
bitcoin-cli generate 1
echo "getRequest"
bitcoin-cli dumpcontractmessage "$contract_address" "getRequest"
echo "getResult"
bitcoin-cli dumpcontractmessage "$contract_address" "getResult"
echo "allow tx"
bitcoin-cli callcontract "$contract_address" "allowRequest" "product-aid-demo" "demo-txid"
bitcoin-cli generate 1
echo "getResult"
bitcoin-cli dumpcontractmessage "$contract_address" "getResult"
bitcoin-cli stop