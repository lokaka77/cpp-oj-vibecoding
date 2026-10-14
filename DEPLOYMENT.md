# CodeHunter 部署与运维手册

本文面向**部署与运维**，覆盖从零搭环境、启动、访问、加固、备份到故障排查。
项目介绍见 [README.md](README.md)，接口定义见 [API.md](API.md)，测试流程见 [web自动化测试流程.md](web自动化测试流程.md)。

> 以下命令假设项目位于 `D:\codexworks\cppojvibecoding`，换路径时请同步替换。

## 0. 快速部署（五步）

```powershell
# 1) 安装依赖到 .tools（首次；需要网络，约 400MB）
powershell -ExecutionPolicy Bypass -File scripts\install_dependencies.ps1

# 2) 配置并构建
$env:PATH = "$PWD\.tools\w64devkit\w64devkit\bin;$env:PATH"
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug `
  "-DCMAKE_TOOLCHAIN_FILE=$PWD/.tools/vcpkg/scripts/buildsystems/vcpkg.cmake"
cmake --build build

# 3) 只启动 MySQL 并初始化数据库
powershell -ExecutionPolicy Bypass -File scripts\start_local.ps1 -DatabaseOnly
Get-Content scripts\init_db.sql -Raw | .\.tools\mysql\bin\mysql.exe -u root

# 4) 启动全部服务
powershell -ExecutionPolicy Bypass -File scripts\start_local.ps1

# 5) 导入题库与测试用户
powershell -ExecutionPolicy Bypass -File scripts\seed_problems.ps1
powershell -ExecutionPolicy Bypass -File scripts\seed_users.ps1
```

完成后访问 `http://127.0.0.1:8080/`，管理员账号 `admin / admin123`。

## 1. 环境要求

| 项目 | 要求 | 说明 |
| --- | --- | --- |
| 操作系统 | Windows 10 / 11 x64 | 当前实现依赖 Windows 的 Job Object 与防火墙（Linux 迁移见第 10 节） |
| 硬件 | 4 核 / 8GB 内存 / 3GB 磁盘 | 参考验证机：i7-14650HX（24 线程）；判题并发按核数调整 |
| 端口 | 3306、8080 空闲 | 可用 `netstat -ano | findstr ":3306 :8080"` 检查 |
| 权限 | 普通用户即可 | 仅"局域网放行"（第 5 节）与"判题加固"（第 6 节）需要管理员 |

## 2. 依赖布局（.tools 便携方案）

所有依赖放在 `.tools\`，**不提交 Git**（体积大且与机器相关）。

| 组件 | 版本 | 路径 | 用途 |
| --- | --- | --- | --- |
| w64devkit（g++ / CMake / Ninja） | 2.10.0 | `.tools\w64devkit\w64devkit` | 编译后端、判题时编译用户代码 |
| vcpkg 静态库 | x64-mingw-static | `.tools\vcpkg` | cpp-httplib、nlohmann-json、libsodium（Argon2id）、libmariadb、yaml-cpp、gtest |
| MySQL Community Server | 8.0.46 | `.tools\mysql` | 数据持久化；数据目录 `.tools\mysql-data` |
| 便携 Node.js | v24.21.0 | `.tools\node` | 运行 Playwright 端到端测试 |
| npm 缓存 | — | `.tools\npm-cache` | 避免占用 C 盘 |

### 2.1 一键安装基础依赖

```powershell
powershell -ExecutionPolicy Bypass -File scripts\install_dependencies.ps1
```

脚本会：下载并解压 w64devkit、克隆并引导 vcpkg、安装 6 个静态库、下载 MySQL 便携版并初始化数据目录、生成 `my.ini`。

### 2.2 安装 Node 与 Playwright（仅跑端到端测试需要）

```powershell
curl.exe -L -o .tools\node.zip https://nodejs.org/dist/v24.21.0/node-v24.21.0-win-x64.zip
Expand-Archive .tools\node.zip -DestinationPath .tools -Force
Move-Item .tools\node-v24.21.0-win-x64 .tools\node
Remove-Item .tools\node.zip

$env:PATH = "$PWD\.tools\node;$env:PATH"
$env:PLAYWRIGHT_SKIP_BROWSER_DOWNLOAD = '1'      # 复用系统 Edge，不下载浏览器内核
$env:npm_config_cache = "$PWD\.tools\npm-cache"
Set-Location tests\e2e
npm install
```

## 3. 首次部署步骤

### 3.1 获取代码

```powershell
git clone <仓库地址> D:\codexworks\cppojvibecoding
Set-Location D:\codexworks\cppojvibecoding
```

### 3.2 安装依赖

见第 2 节。若 `.tools` 已存在（例如从旧机器拷贝），可跳过。

### 3.3 配置与构建

```powershell
$env:PATH = "$PWD\.tools\w64devkit\w64devkit\bin;$env:PATH"
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug `
  "-DCMAKE_TOOLCHAIN_FILE=$PWD/.tools/vcpkg/scripts/buildsystems/vcpkg.cmake"
cmake --build build
```

构建产物：`build\web_server.exe`、`build\judge_worker.exe`、`build\oj_tests.exe`、`build\database_integration_tests.exe`。

### 3.4 初始化数据库（仅首次）

```powershell
powershell -ExecutionPolicy Bypass -File scripts\start_local.ps1 -DatabaseOnly
Get-Content scripts\init_db.sql -Raw | .\.tools\mysql\bin\mysql.exe -u root
```

`init_db.sql` 会创建 `oj` 库、数据表、业务账号 `oj_app` 与预置管理员 `admin / admin123`。

> 旧版本数据库升级时，额外执行一次字段迁移（可重复执行）：
>
> ```powershell
> Get-Content scripts\migrate_add_difficulty.sql -Raw | .\.tools\mysql\bin\mysql.exe -u root
> ```

### 3.5 启动服务

```powershell
powershell -ExecutionPolicy Bypass -File scripts\start_local.ps1
```

脚本依次启动 MySQL → `web_server` → `judge_worker`，并等待 3306/8080 就绪（实测约 3.5 秒）。

### 3.6 导入题库与测试用户

```powershell
powershell -ExecutionPolicy Bypass -File scripts\seed_problems.ps1   # 10 道题（4/3/3）+ 50 组用例
powershell -ExecutionPolicy Bypass -File scripts\seed_users.ps1      # student01~student20，口令 student123
```

两个脚本都是幂等的：按标题/用户名判断，已存在则更新，不会产生重复数据。

### 3.7 验证部署

```powershell
curl.exe http://127.0.0.1:8080/api/health        # {"status":"ok"}
& .\.tools\w64devkit\w64devkit\bin\ctest.exe --test-dir build --output-on-failure
```

## 4. 启动、停止与日志

| 操作 | 命令 |
| --- | --- |
| 启动全部 | `powershell -ExecutionPolicy Bypass -File scripts\start_local.ps1` |
| 只启动 MySQL | `powershell -ExecutionPolicy Bypass -File scripts\start_local.ps1 -DatabaseOnly` |
| 停止全部 | `powershell -ExecutionPolicy Bypass -File scripts\stop_local.ps1` |

日志位于 `build\`：`web_server.log`、`web_server_error.log`、`judge_worker.log`、`judge_worker_error.log`、`mysql_error.log`。

服务不会开机自启；每次开发/演示前手动启动即可，下班可直接停止（数据落在 `.tools\mysql-data`，不会丢）。

## 5. 访问方式

### 5.1 本机

`http://127.0.0.1:8080/`（服务监听 `0.0.0.0`，同一台机器用 `localhost` 亦可）。

### 5.2 局域网 / 手机

1. 确认手机与笔记本在**同一 Wi-Fi**。
2. 以管理员身份放行端口（仅首次）：

   ```powershell
   powershell -ExecutionPolicy Bypass -File scripts\allow_lan_access.ps1
   ```

3. 查看当前可用地址并访问：

   ```powershell
   powershell -ExecutionPolicy Bypass -File scripts\allow_lan_access.ps1 -Status
   ```

   输出形如 `手机访问地址：http://192.168.1.23:8080/`。

4. 手机浏览器打开该地址。撤销放行：`allow_lan_access.ps1 -Remove`。

**打不开时的排查顺序**

| 现象 | 原因 | 处理 |
| --- | --- | --- |
| 手机提示无法连接 | 手机与电脑不在同一网段 | 确认两者连同一 Wi-Fi；电脑连有线、手机连 Wi-Fi 也可能不同网段 |
| 同一 Wi-Fi 仍不通 | 路由器开启了 AP 隔离/客户端隔离（公司、学校、酒店常见） | 用**手机开热点**，电脑连热点后按新 IP 访问；或换家用路由 |
| 换成热点仍不通 | VPN / 代理软件改写了路由 | 断开深信服等 VPN、退出代理软件后重试 |
| 换了网络后地址失效 | DHCP 重新分配了 IP | 重跑 `allow_lan_access.ps1 -Status` 取新地址 |
| 电脑浏览器显示"拒绝连接" | 服务没启动，或正处于重启窗口 | `scripts\start_local.ps1`，再访问 `/api/health` 确认 |

> 注意：站点是明文 HTTP。同一网络内的人都能打开并尝试登录，管理员默认口令是 `admin123`，在非可信网络下请先修改口令或撤销放行规则。

## 6. 判题网络加固（推荐，一次即可）

系统会让用户代码在受控环境中运行。默认情况下用户代码**可以访问外网**、也能用绝对路径读取本机文件；下面这步关闭"联网"这一项（防火墙按判题固定执行路径拦截 + 禁止判题程序创建子进程）。

以管理员身份执行：

```powershell
powershell -ExecutionPolicy Bypass -File scripts\harden_judge.ps1            # 添加规则
powershell -ExecutionPolicy Bypass -File scripts\harden_judge.ps1 -Status    # 查看规则
powershell -ExecutionPolicy Bypass -File scripts\harden_judge.ps1 -Remove    # 撤销规则
```

脚本会创建三条规则：

| 规则名 | 作用 |
| --- | --- |
| `CodeHunter Judge Runner (out)` | 拦截判题执行程序 `build\judge_tmp\runner\program.exe` 的出站流量 |
| `CodeHunter Judge Runner (in)` | 拦截其入站流量 |
| （程序侧）Job Object 限制 | 判题程序不能创建子进程，避免用 `curl`/`powershell` 绕过按路径拦截的规则 |

规则**只作用于判题程序那一个可执行文件**，不影响浏览器、系统更新或其它软件。撤销后"用户代码不能联网"这条安全验收会重新变成未达标。

## 7. 数据、备份与恢复

| 项目 | 位置 |
| --- | --- |
| 数据库文件 | `.tools\mysql-data\`（整个目录即完整数据） |
| 题库与用例 | 库 `oj` 的 `problems`、`test_cases` 表（也可用种子脚本重建） |
| 提交记录 | 库 `oj` 的 `submissions` 表 |
| 用户 | 库 `oj` 的 `users` 表（口令为 Argon2id 哈希） |

备份：

```powershell
& .\.tools\mysql\bin\mysqldump.exe -u root --databases oj --result-file=build\oj_backup.sql
```

恢复：

```powershell
Get-Content build\oj_backup.sql -Raw | .\.tools\mysql\bin\mysql.exe -u root --default-character-set=utf8mb4
```

迁移到新机器：拷贝代码 + `.tools`（含 `mysql-data`）即可，无需重新初始化。

## 8. 测试与验收脚本

> `ctest` 与 Playwright 会同时读写同一个库，**必须串行执行**，并行会导致集成测试误报。

| 目的 | 命令 |
| --- | --- |
| 单元 + 集成测试 | `& .\.tools\w64devkit\w64devkit\bin\ctest.exe --test-dir build --output-on-failure` |
| 端到端（15 条，复用系统 Edge） | `Set-Location tests\e2e; npm test`（先按第 2.2 节装好 Node 依赖） |
| 接口回归 | 按 `基于curl命令的接口自动化测试.md` 执行 |
| 性能验收 | `powershell -ExecutionPolicy Bypass -File scripts\perf_test.ps1` |
| 安全验收 | `powershell -ExecutionPolicy Bypass -File scripts\security_test.ps1` |

## 9. 常见故障排查

| 现象 | 原因 | 处理 |
| --- | --- | --- |
| `Can't connect to MySQL server on 'localhost:3306'` | MySQL 未启动 | `scripts\start_local.ps1`（或 `-DatabaseOnly`） |
| `unknown option '--defaults-file'` / `unknown variable` | 手工启动参数写错 | 用 `scripts\start_local.ps1`，勿重复传 `my.ini` |
| 8080 端口被占用 | 其它程序占用 | `netstat -ano | findstr :8080` 找到 PID，结束进程或改 `config.yaml` 的 `server.port` |
| 页面显示的还是旧界面 | 浏览器缓存 | 服务端已返回 `Cache-Control: no-cache` 且资源带版本号；按一次 `Ctrl+F5` |
| 提交后提示"判题服务超时" | `judge_worker` 未启动或队列积压 | 看 `build\judge_worker_error.log`；必要时调大 `config.yaml` 的 `judge.worker_count` |
| 提交被判成 `RE`（代码没问题） | 判题基础设施抖动 | 已在 `process_runner.cc` 加重试与错误码日志；查看 `judge_worker_error.log` |
| 测试结果互相干扰 | `ctest` 与 e2e 并行 | 串行执行 |
| 防火墙规则丢失（重装/换机） | 规则未持久化到新环境 | 重新执行 `harden_judge.ps1` |
| 手机无法访问 | 同网段/AP 隔离/VPN | 见第 5.2 节排查表 |

## 10. 迁移到 Linux（规划）

Windows 本地部署存在两项固有限制：无文件系统隔离（用户代码可用绝对路径读文件）、网络隔离依赖防火墙规则。迁移 Linux 后可一并解决，需要改动：

| 部分 | 现状（Windows） | Linux 方案 |
| --- | --- | --- |
| 服务启停 | `scripts\*.ps1` | systemd unit 或 bash 脚本 |
| 判题执行 | `CreateProcess` + Job Object（`src/judge/process_runner.cc`） | `fork/exec` + `setrlimit` + `unshare`/namespaces + `seccomp` |
| 网络隔离 | 防火墙按路径拦截 | netns 断网 或 nftables |
| 文件隔离 | 仅固定工作目录（相对路径） | 只读挂载 + 低权限用户 + 目录权限 |
| 数据存储 | 便携 MySQL + `.tools\mysql-data` | 系统 MySQL 或容器化 MySQL |

代码侧建议先抽出 `ProcessRunner` 的平台接口（当前 Windows 实现），再补 Linux 实现。
