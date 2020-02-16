# TOCTOU-Detection

### 簡介（Introduction）
[Clang static analyzer](https://clang-analyzer.llvm.org)是一個基於Clang的C/C++/Object-C原始碼檢測框架，而本工具則是基於Clang Static Analyzer開發的TOCTOU漏洞檢測工具

TOCTOU-Dection is a tool for detecting TOCTOU vulnerability. It is built on top of [Clang static analyzer](https://clang-analyzer.llvm.org), a source code analysis tool that finds bugs in the code written in C/C++/Object-C.

### 安裝
環境（Platform）
```
OS：ubuntu 12.04 LTS
```
必要套件（Dependencies）
```
apt-get install build-essential zlib1g-dev python
```
編譯 & 安裝（Build & Install）
```
git clone https://github.com/ncu-psl/TOCTOU-Detection.git llvm
mkdir build
cd build
../llvm/configure --enable-optimized
make && make install
```
### 使用（Usage）
檢測 testCase.c

Check testCase.c
```
clang --analyze -Xanalyzer -analyzer-checker=alpha.toctou testCase.c
```
