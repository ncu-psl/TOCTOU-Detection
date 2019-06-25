# TOCTOU-Detection

### 簡介
[Clang static analyzer](https://clang-analyzer.llvm.org)是一個基於clang的c/c++/object-c原始碼檢測框架，而本工具則是基於clang static analyzer開發的漏洞檢測工具


### 安裝  
```
安裝環境：ubuntu 12.04 LTS
```
安裝必要工具
```
待補...
```
下載編譯
```
cd ~
mkdir llvm
cd llvm
git clone https://github.com/ncu-psl/TOCTOU-Detection.git ./llvm
mkdir build
cd build
cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release ../llvm
make
cd ..
```
### 使用
檢測testCase.c
```
clang --analyze -Xanalyzer -analyzer-checker=alpha.toctou testCase.c
```
