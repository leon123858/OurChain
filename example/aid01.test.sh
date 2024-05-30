deploycontract() {
    bitcoin-cli deploycontract ~/Desktop/ourchain/example/aid01.cpp > log.txt
    # 使用 grep 和 awk 从 log.txt 文件中提取合同地址
    contract_address=$(grep "contract address" log.txt | awk -F'"' '{print $4}')
    rm log.txt
    echo "$contract_address"
}

bitcoin-cli generate 3
contract_address=$(deploycontract)
echo "$contract_address"
bitcoin-cli generate 1
bitcoin-cli callcontract "$contract_address" "registerNewUser" "user1" "password1"
bitcoin-cli callcontract "$contract_address" "registerNewUser" "user2" "password2"
bitcoin-cli generate 1
bitcoin-cli dumpcontractmessage "$contract_address" "get"
bitcoin-cli dumpcontractmessage "$contract_address" "readSign" "user1"
bitcoin-cli callcontract "$contract_address" "sign" "user1" "password1" "new message1!!!"
bitcoin-cli callcontract "$contract_address" "sign" "user1" "password1" "new message2!!!"
bitcoin-cli callcontract "$contract_address" "sign" "user2" "password2" "new message3!!!"
bitcoin-cli generate 1
bitcoin-cli dumpcontractmessage "$contract_address" "readSign" "user1"
bitcoin-cli dumpcontractmessage "$contract_address" "readSign" "user2"