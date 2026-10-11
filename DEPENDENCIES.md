# OJ 项目依赖清单与安装命令

本文根据 `SPEC.md` 的技术栈，列出在 Windows 笔记本上开发本项目所需的依赖。

## 1. 依赖总览

| 依赖 | 用途 | 是否必需 |
| --- | --- | --- |
| MinGW-w64 `g++` | 编译用户提交的 C++17 代码 | 必需 |
| CMake | 生成项目构建文件 | 必需 |
| Ninja | CMake 构建工具 | 必需 |
| Git | 拉取 vcpkg 和依赖 | 必需 |
| vcpkg | 管理 C++ 第三方库 | 必需 |
| cpp-httplib | 后端 HTTP 服务 | 必需 |
| nlohmann/json | 后端 JSON 请求与响应处理 | 必需 |
| yaml-cpp | 解析 config.yaml 配置 | 必需 |
| GoogleTest | C++ 单元测试框架 | 必需 |
| libsodium | 用户密码安全哈希 | 必需 |
| MySQL Server | 数据持久化 | 必需 |
| MySQL 客户端库 | 后端连接 MySQL | 必需 |

前端使用原生 HTML、CSS、JavaScript，不需要 npm 或其他前端构建工具。

## 2. 推荐安装方案

### 2.1 安装 MSYS2

在 PowerShell 中执行：

```powershell
winget install --id=MSYS2.MSYS2 -e
```

安装完成后，从开始菜单打开 `MSYS2 UCRT64`，执行：

```bash
pacman -Syu
pacman -S --needed mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja mingw-w64-ucrt-x86_64-pkgconf git
```

验证安装：

```bash
g++ --version
cmake --version
ninja --version
git --version
```

### 2.2 安装 vcpkg

在 PowerShell 中执行：

```powershell
git clone https://github.com/microsoft/vcpkg.git C:\dev\vcpkg
cd C:\dev\vcpkg
.\bootstrap-vcpkg.bat
```

### 2.3 安装 C++ 第三方库

以下命令使用 MinGW 静态链接 triplet：

```powershell
cd C:\dev\vcpkg
.\vcpkg install cpp-httplib:x64-mingw-static nlohmann-json:x64-mingw-static yaml-cpp:x64-mingw-static gtest:x64-mingw-static libsodium:x64-mingw-static libmariadb:x64-mingw-static
```

说明：

- `cpp-httplib` 是文档指定的 HTTP 服务库。
- `nlohmann/json` 用于统一 JSON 响应。
- `yaml-cpp` 用于解析 `config.yaml`。
- `gtest` 用于单元测试。
- `libsodium` 提供 Argon2 等安全密码哈希能力。
- `libmariadb` 使用 MySQL 协议连接 MySQL 服务端，服务端仍安装 MySQL。

验证依赖：

```powershell
.\vcpkg list
```

### 2.4 安装 MySQL Server

在 PowerShell 中安装：

```powershell
winget search MySQL
winget install --id=Oracle.MySQL -e
```

如果 winget 中找不到对应包，也可以从 MySQL 官网下载 MySQL Installer：

```text
https://dev.mysql.com/downloads/installer/
```

安装完成后，将 MySQL 的 `bin` 目录加入 `PATH`，然后验证：

```powershell
mysql --version
```

后续需要创建数据库和业务账号，例如：

```sql
CREATE DATABASE oj CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;
```

账号和建表脚本在实现阶段再补充，不在这里安装。

## 3. 备选方案

### 3.1 使用 Chocolatey

如果机器已经安装 Chocolatey，可以合并安装部分工具：

```powershell
choco install git cmake ninja mysql -y
```

但 `g++` 和 `cpp-httplib` 仍建议通过 MSYS2 与 vcpkg 安装，以确保 C++17 和静态链接配置稳定。

### 3.2 使用 MySQL Connector/C++

如果希望使用 Oracle 官方 MySQL C++ 连接器，而不是 `libmariadb`，可以使用 vcpkg：

```powershell
.\vcpkg install mysql-connector-cpp:x64-windows-static
```

该方式更偏向 MSVC 工具链。若统一使用 MinGW，推荐继续使用 `libmariadb`，它同样兼容 MySQL 服务端。

## 4. 安装后自检清单

按顺序执行以下命令，确认关键依赖已就绪：

```bash
g++ --version
cmake --version
ninja --version
git --version
```

```powershell
mysql --version
```

```powershell
cd C:\dev\vcpkg
.\vcpkg list
```

应能看到：

- `cpp-httplib`
- `nlohmann-json`
- `yaml-cpp`
- `gtest`
- `libsodium`
- `libmariadb`

以上依赖安装完成后，即可进入项目脚手架、MySQL 建表和代码实现阶段。
