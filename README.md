# TOCTOU-Detection

### 簡介
[Clang static analyzer](https://clang-analyzer.llvm.org)是一個基於Clang的C/C++/Object-C原始碼檢測框架，而本工具則是基於Clang Static Analyzer開發的TOCTOU漏洞檢測工具


### 安裝
環境  
```
OS：ubuntu 12.04 LTS
```
必要套件
```
apt-get install build-essential zlib1g-dev python
```
編譯 & 安裝
```
git clone https://github.com/ncu-psl/TOCTOU-Detection.git llvm
mkdir build
cd build
../llvm/configure --enable-optimized
make && make install
```
### 使用
檢測 testCase.c
```
clang --analyze -Xanalyzer -analyzer-checker=alpha.toctou testCase.c
```
