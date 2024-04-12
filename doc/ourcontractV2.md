# OurContract

OurContract 是在 OurChain 上的智能合约模塊。以下說明其概念和如何使用。

## 核心概念

### 狀態的讀取與存入

![image](https://github.com/leon123858/OurChain/assets/56253942/f29f5eef-297a-4574-8275-cc3c1ae455ef)

參考上圖, 合約和合約可以遞迴呼叫, 但是只有最外層的合約可以存入狀態或輸出狀態。每個合約被呼叫時都可以讀取屬於自己的當前狀態。

- 輸出狀態到資料庫被稱為 `callcontract` 操作, 這個操作會產生一筆交易, 並且會被區塊鏈記錄下來。
- 輸出狀態到用戶端被稱為 `dumpcontractmessage` 操作, 這個操作不會產生交易, 但是會讀取區塊鏈上的交易記錄。

### 合約狀態變化策略

描述合約模塊如何依據區塊鏈的變化改變狀態。合約模塊會比較當前鏈的狀態和合約當前維護狀態取用不同的更新策略。
詳見 [updateStrategy](../src/contract/updateStrategy.h)

## 合約撰寫

### 快速開始

請看 [sample.cpp](../sample.cpp)

### 進階範例

請看 [example folder](../example/)

### 所有方法

請看 [ourcontract.h](../src/contract/ourcontract.h)

### 實際調用方法

請看 [mytest.sh](../mytest.sh)

### 線下合約編譯測試

須在已安裝, 且可順利運行 OurChain 的容器內, 假設測試合約名稱為 `aid.cpp`

```sh
g++ -fPIC -g -c -Wall -o "./aid.o" "./aid.cpp"
g++ -shared -Wl,-soname,"aid.so" -o "./aid.so" "./aid.o" -lssl -lcrypto
rm -f "./aid.o"
```

### 外部 API 串接

請看 [our contract agent](https://github.com/leon123858/go-aid)




