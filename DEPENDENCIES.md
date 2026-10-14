# CodeHunter 依赖清单

本项目采用**便携依赖**方案：所有第三方组件放在 `.tools\`，不提交 Git，换机器可整体拷贝。
安装步骤见 [DEPLOYMENT.md](DEPLOYMENT.md) 第 2 节；本文只列版本、来源与用途。

## 1. 依赖总览

| 组件 | 版本 | 来源 | 用途 | 必需 |
| --- | --- | --- | --- | --- |
| w64devkit | 2.10.0 | GitHub `skeeto/w64devkit` releases | 提供 `g++`（C++17）、CMake、Ninja | 是 |
| vcpkg | 最新主线（clone） | GitHub `microsoft/vcpkg` | 安装下列 C++ 静态库 | 是 |
| cpp-httplib | vcpkg 版本 | vcpkg `cpp-httplib:x64-mingw-static` | HTTP 服务与路由 | 是 |
| nlohmann-json | vcpkg 版本 | vcpkg `nlohmann-json:x64-mingw-static` | JSON 序列化 | 是 |
| libsodium | vcpkg 版本 | vcpkg `libsodium:x64-mingw-static` | 口令哈希（Argon2id） | 是 |
| libmariadb | vcpkg 版本 | vcpkg `libmariadb:x64-mingw-static` | 连接 MySQL 服务端 | 是 |
| yaml-cpp | vcpkg 版本 | vcpkg `yaml-cpp:x64-mingw-static` | 解析 `config.yaml` | 是 |
| GoogleTest | vcpkg 版本 | vcpkg `gtest:x64-mingw-static` | 单元测试与集成测试 | 是（测试） |
| MySQL Community Server | 8.0.46（no-install ZIP） | dev.mysql.com | 数据持久化 | 是 |
| Node.js | v24.21.0（便携 ZIP） | nodejs.org | 运行 Playwright 端到端测试 | 否（仅测试） |
| Playwright | 随 `npm install` 安装 | npm registry | 浏览器端到端测试（复用系统 Edge，不下载内核） | 否（仅测试） |

## 2. 一键安装

```powershell
powershell -ExecutionPolicy Bypass -File scripts\install_dependencies.ps1
```

脚本行为：

1. 下载并解压 w64devkit 2.10.0 到 `.tools\w64devkit`；
2. 克隆并引导 vcpkg 到 `.tools\vcpkg`；
3. 安装 6 个静态库（cpp-httplib、nlohmann-json、libsodium、libmariadb、yaml-cpp、gtest）；
4. 下载 MySQL 8.0.46 便携版到 `.tools\mysql`，生成 `my.ini`，初始化数据目录 `.tools\mysql-data`。

脚本**不含** Node 与 Playwright（只有跑端到端测试才需要），安装命令见 [DEPLOYMENT.md](DEPLOYMENT.md) 第 2.2 节。

## 3. 安装后自检

```powershell
& .\.tools\w64devkit\w64devkit\bin\g++.exe --version
& .\.tools\w64devkit\w64devkit\bin\cmake.exe --version
& .\.tools\w64devkit\w64devkit\bin\ninja.exe --version
& .\.tools\vcpkg\vcpkg.exe list | Select-String 'x64-mingw-static'
& .\.tools\mysql\bin\mysqld.exe --version
& .\.tools\node\node.exe --version          # 可选：已装 Node 时
```

## 4. 备选安装方式（不使用 .tools 时）

如果偏好系统级安装（会让项目依赖机器环境，不推荐用于本项目的复现）：

| 组件 | 安装方式 |
| --- | --- |
| 编译工具链 | MSYS2（`pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake mingw-w64-x86_64-ninja`）或 winget |
| C++ 库 | vcpkg（`vcpkg install cpp-httplib nlohmann-json libsodium libmariadb yaml-cpp gtest`），或 MSYS2 对应 `mingw-w64-x86_64-*` 包 |
| MySQL | `winget install --id=Oracle.MySQL -e`，或 MySQL Installer |
| Node.js | `winget install --id=OpenJS.NodeJS.LTS -e`，或官网安装包 |

此方式下构建时需自行指定 `CMAKE_TOOLCHAIN_FILE`（vcpkg）与编译器路径。
