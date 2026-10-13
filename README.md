# CodeHunter

C++ 在线判题平台（教学与训练场景）。后端 C++17 + cpp-httplib，前端原生 HTML/CSS/JS，MySQL 持久化，独立判题服务负责编译与运行。

## 功能概览

- 题目列表与题目详情，未登录用户可以预览描述与样例
- 题目按难度分级（简单 / 中等 / 困难），列表页支持按难度筛选与标题搜索
- 注册、登录、退出，Session/Cookie 会话，Argon2id 密码哈希
- 在线编辑器：运行样例、提交判题，返回 `AC / WA / TLE / RE / CE`
- 管理员：题目增删改、测试用例导入、用户管理
- 判题：`g++ -std=c++17` 编译，独立子进程运行，1 秒时限、128MB 内存限制，进程树清理，单用户提交频率限制

## 目录结构

| 路径 | 说明 |
| --- | --- |
| `web/` | 前端页面（落地页、登录注册、题库、题目详情、管理后台） |
| `src/web_server/` | HTTP 服务（路由、鉴权、会话、限流） |
| `src/judge/` | 判题服务（编译、运行、进程管理） |
| `src/common/` | 配置、日志、数据库、密码、JSON 公共模块 |
| `scripts/` | 部署与运维脚本（`start_local.ps1`、`stop_local.ps1`、`init_db.sql`） |
| `tests/` | gtest 单元测试与数据库集成测试 |
| `.tools/` | 便携工具链（MySQL、w64devkit、vcpkg 依赖），不提交到 Git |
| `build/` | 构建产物与运行日志，不提交到 Git |

## 本地部署（Windows 笔记本）

### 1. 依赖

依赖以绿色便携方式放在 `.tools/`（不进入 Git）：

| 依赖 | 位置 | 用途 |
| --- | --- | --- |
| MySQL 8.0 | `.tools/mysql` | 数据存储（数据目录 `.tools/mysql-data`） |
| w64devkit（g++ / cmake / ninja） | `.tools/w64devkit/w64devkit` | 编译后端与判题提交 |
| vcpkg 静态库 | `.tools/vcpkg/installed/x64-mingw-static` | cpp-httplib、nlohmann/json、argon2、gtest |

### 2. 初始化数据库（仅首次）

```powershell
powershell -ExecutionPolicy Bypass -File scripts\start_local.ps1
Get-Content scripts\init_db.sql -Raw | .\.tools\mysql\bin\mysql.exe -u root
```

初始化脚本创建 `oj` 库、表结构与预置管理员账号。

已有数据库升级时，执行一次迁移脚本补充 `difficulty` 字段（可重复执行）：

```powershell
Get-Content scripts\migrate_add_difficulty.sql -Raw | .\.tools\mysql\bin\mysql.exe -u root
```

### 3. 构建

```powershell
$env:PATH = "$PWD\.tools\w64devkit\w64devkit\bin;$env:PATH"
cmake --build build
```

### 4. 启动与停止

```powershell
powershell -ExecutionPolicy Bypass -File scripts\start_local.ps1
powershell -ExecutionPolicy Bypass -File scripts\stop_local.ps1
```

`start_local.ps1` 依次启动 MySQL、`web_server`、`judge_worker`，并等待 3306 与 8080 就绪（实测约 3.5 秒）。

### 5. 导入示例题库

```powershell
powershell -ExecutionPolicy Bypass -File scripts\seed_problems.ps1
```

脚本以题目标题为准：已存在则更新描述与难度并替换测试用例，不存在则新建。当前包含 10 道题（简单 4 / 中等 3 / 困难 3），每题 5 组测试用例（2 组样例 + 3 组隐藏）。

### 6. 准备测试用户

```powershell
powershell -ExecutionPolicy Bypass -File scripts\seed_users.ps1
```

批量创建 `student01` ~ `student20`（初始密码 `student123`）。脚本通过注册接口创建账号，密码走真实的 Argon2id 哈希流程；已存在的用户会自动跳过。管理员可在"用户"页面查看或删除这些账号。

### 7. 访问地址

| 用途 | 地址 |
| --- | --- |
| 本机访问 | `http://127.0.0.1:8080/` |
| 局域网访问 | `http://<本机IPv4>:8080/`（服务监听 `0.0.0.0`，需在防火墙放行） |
| 健康检查 | `http://127.0.0.1:8080/api/health` |

预置管理员：`admin / admin123`。

### 8. 运行测试

```powershell
& .\.tools\w64devkit\w64devkit\bin\ctest.exe --test-dir build --output-on-failure
```

- 单元测试：`oj_tests`
- 数据库集成测试：`database_integration_tests`
- 接口级回归：`基于curl命令的接口自动化测试.md`

### 9. 前端端到端测试（Playwright）

前置条件：服务已启动（MySQL、web_server、judge_worker），题库与测试用户已导入。

```powershell
$env:PATH = "$PWD\.tools\node;$env:PATH"
Set-Location tests\e2e
npm test              # 无头运行，复用系统 Edge
npm run test:headed   # 需要观察浏览器操作时
```

- Node 使用 `.tools\node` 下的便携版，Playwright 测试包位于 `tests/e2e/node_modules`，两者都不提交到 Git。
- 用例位于 `tests/e2e/specs/`：`guest.spec.js`（游客预览、难度筛选、返回落地页）、`student.spec.js`（登录、运行样例、提交判题、退出）、`admin.spec.js`（管理入口、用户目录、题目增删）。
- 首次安装依赖：在 `tests/e2e` 目录执行 `npm install`（已配置跳过浏览器内核下载，直接使用系统 Edge）。
- 运行后生成 HTML 报告：`tests/e2e/report/index.html`。

## 部署验收记录（2026-10-13）

| 项目 | 结果 |
| --- | --- |
| 本地启动 | `scripts\start_local.ps1` 3.5 秒内完成，MySQL / web_server / judge_worker 全部就绪 |
| 页面可用 | 落地页、登录注册、题库、题目详情、管理后台均返回 200 |
| 游客权限 | 题目列表与详情可预览；运行样例、提交判题、已通过列表均返回 401 |
| 端到端判题 | 注册 → 登录 → 运行样例 → 提交返回 `AC`，经过 web_server、MySQL、judge_worker、g++ 全链路 |
| 管理员功能 | 登录、用户列表、删除测试账号均正常 |
| 重启持久化 | 重启前后 用户 3 / 题目 1 / 测试用例 3 / 提交 20，数据一致 |
| 已知事项 | 单次提交端到端约 1.1 秒（主要为 g++ 编译耗时），性能目标见 `SPEC.md` 16.2 |

## 相关文档

- `SPEC.md`：需求、架构、TODO 与验收标准
- `API.md`：接口说明
- `DEPENDENCIES.md`：依赖清单与安装方式
- `基于curl命令的接口自动化测试.md`：接口回归脚本
- `web自动化测试流程.md`：自动化测试分层、用例映射与执行流程
