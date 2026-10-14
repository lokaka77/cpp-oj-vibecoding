# CodeHunter

面向 C++ 教学与训练的在线判题平台（Online Judge）：浏览器里写代码、运行样例、提交判题，拿到 `AC / WA / TLE / RE / CE` 结论。

后端 C++17 + cpp-httplib，前端原生 HTML/CSS/JS（无框架），MySQL 持久化，独立判题服务真实编译并运行用户代码。

> 想直接跑起来？见 [DEPLOYMENT.md](DEPLOYMENT.md)（部署与运维手册）。

## 功能概览

| 能力 | 说明 |
| --- | --- |
| 游客预览 | 未登录可浏览题目列表、查看题目描述与可见样例（编辑器区域显示登录入口） |
| 账号 | 注册、登录、退出；Session/Cookie 会话，口令以 Argon2id 哈希存储 |
| 在线作答 | 内置代码编辑器（行号、Tab 缩进、草稿自动保存）→ 运行样例 → 提交判题 |
| 判题结论 | `AC` 通过 / `WA` 答案错误 / `TLE` 超时 / `RE` 运行时错误 / `CE` 编译错误 |
| 难度与筛选 | 题目分简单 / 中等 / 困难，列表支持按难度筛选并与标题搜索叠加 |
| 正反馈 | 已通过的题目在列表中带金色标记（含自身 AC 记录） |
| 管理后台 | 题目新增 / 编辑 / 删除、测试用例导入（每题 ≤5 组，至少 1 组样例）、用户查看与删除 |
| 判题约束 | `g++ -std=c++17` 编译，1 秒时间限制、128MB 内存限制，单用户提交频率限制 |

## 角色与使用流程

| 角色 | 能做什么 |
| --- | --- |
| 游客 | 浏览题库、预览题目与样例；不能运行样例或提交 |
| 普通用户 | 上述全部 + 运行样例、提交判题、查看自己的通过标记 |
| 管理员 | 上述全部 + 题目与用例管理、用户管理 |

- **学生流程**：登录 → 题库筛选/搜索 → 进入题目 → 编写代码（草稿存本机浏览器）→ 运行样例 → 提交 → 查看判题结论。
- **管理员流程**：登录 → 题目管理（新增题目 / 编辑题面 / 导入测试用例 / 删除）→ 用户管理（查看、删除普通用户，管理员账号受保护）。

## 技术栈与架构

| 层 | 选型 |
| --- | --- |
| 前端 | 原生 HTML + CSS + JavaScript（Canvas 科技风背景，响应式，无构建步骤） |
| Web 服务 | C++17 + cpp-httplib，提供 REST 接口与静态页面 |
| 数据存储 | MySQL 8.0（库 `oj`） |
| 判题服务 | 独立进程 `judge_worker`：从数据库领取待判任务，编译并运行用户代码 |
| 编译器 | w64devkit 提供的 `g++ -std=c++17` |
| 测试 | GoogleTest（单元 + 数据库集成）、Playwright（端到端）、curl 脚本（接口回归） |

请求链路：

```text
浏览器 ──HTTP──> web_server(8080) ──SQL──> MySQL(3306)
                     │                        ▲
                     │  写入待判任务            │ 回写结论
                     ▼                        │
                judge_worker ──编译/运行──> 用户程序（受时限与内存约束）
```

判题结论由 `judge_worker` 写回数据库，`web_server` 在同一次提交请求内轮询结果并返回给前端。

## 判题规则

- 编译：`g++ -std=c++17 <源码> -o program.exe`，编译错误只返回友好结论，不回显编译器原始输出。
- 运行：每个测试点独立进程执行，超时（默认 1 秒）终止整个进程树，内存超过 128MB 判定失败。
- 比较：忽略每行行尾空白与文末多余换行。
- 结论：`AC` 全部测试点通过；`WA` 输出不一致；`TLE` 超时；`RE` 运行时错误或超内存；`CE` 编译失败。
- 隔离：独立子进程 + 作业对象（CPU/内存/进程数限制 + 进程树清理）；**已知限制见下节**。

## 项目结构

| 路径 | 说明 |
| --- | --- |
| `web/` | 前端页面：落地页、登录注册、题库、题目详情、管理后台、用户管理 |
| `src/web_server/` | HTTP 服务：路由、鉴权、会话、限流、管理端接口 |
| `src/judge/` | 判题服务：编译、进程执行、输出比较、worker 池 |
| `src/common/` | 配置、日志、数据库、口令哈希、JSON 工具 |
| `scripts/` | 安装、启停、建库、种子数据、局域网放行、判题加固、性能与安全验收脚本 |
| `tests/unit/`、`tests/integration/` | GoogleTest 单元与数据库集成测试 |
| `tests/e2e/` | Playwright 端到端测试（复用系统 Edge） |
| `.tools/` | 便携工具链（MySQL、w64devkit、vcpkg、Node），不提交 Git |
| `build/` | 构建产物与运行日志，不提交 Git |

## 快速开始

```powershell
powershell -ExecutionPolicy Bypass -File scripts\install_dependencies.ps1   # 首次：安装便携依赖
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug "-DCMAKE_TOOLCHAIN_FILE=$PWD/.tools/vcpkg/scripts/buildsystems/vcpkg.cmake"
cmake --build build
powershell -ExecutionPolicy Bypass -File scripts\start_local.ps1            # 启动 MySQL / web_server / judge_worker
```

默认地址 `http://127.0.0.1:8080/`，管理员 `admin / admin123`，测试用户 `student01~student20 / student123`。
建库、导入题库、局域网访问（手机）、判题加固、备份与故障排查等完整步骤见 [DEPLOYMENT.md](DEPLOYMENT.md)。

## 当前状态（2026-10-13 阶段五验收）

| 验收项 | 结果 |
| --- | --- |
| 功能验收 | 通过：Playwright 15 条端到端用例 + 判题结论矩阵（AC/WA/TLE/RE/CE）+ 接口回归全部通过 |
| 性能验收 | 通过（口径修订后）：单次提交端到端约 1.4 秒；20 用户并发 20/20 成功、p50 3.4 秒、p95 4.9 秒 |
| 安全验收 | 9 项检查 8 项通过：口令哈希、SQL 注入、越权、相对路径读文件、外网拦截均通过 |
| 部署验收 | 通过：本地启动约 3.5 秒，重启后题目与用例保留 |

已记录的注意事项与限制：

- **500ms 口径修订**：原始目标"端到端 ≤500ms"未计入真实编译成本（`g++` 约 0.45 秒）与 Windows 首次加载新二进制的开销（约 0.5 秒），已按教学场景重订为"单用户 ≤2 秒、20 并发 p95 ≤6 秒"。
- **文件隔离限制**：Windows 下用户代码仍可用**绝对路径**读取本机文件（相对路径已封堵）；迁移 Linux 或改用 AppContainer 可解决。
- **网络隔离依赖加固脚本**：需管理员执行一次 `scripts\harden_judge.ps1`（见 DEPLOYMENT.md 第 6 节）。
- **明文 HTTP + 默认口令**：局域网访问时请注意网络可信度，并尽快修改管理员口令。

## 相关文档

| 文档 | 内容 |
| --- | --- |
| [DEPLOYMENT.md](DEPLOYMENT.md) | 部署与运维：依赖安装、建库、启停、手机访问、加固、备份、故障排查、Linux 迁移 |
| [SPEC.md](SPEC.md) | 需求、架构、数据模型、API 边界、非功能需求、TODO 与验收标准 |
| [API.md](API.md) | 全部接口的路径、参数、响应与错误码 |
| [DEPENDENCIES.md](DEPENDENCIES.md) | 依赖清单（版本与来源） |
| [web自动化测试流程.md](web自动化测试流程.md) | 测试分层、用例映射、执行流程、验收记录与缺陷记录 |
| [基于curl命令的接口自动化测试.md](基于curl命令的接口自动化测试.md) | 接口回归脚本与分步 curl 示例 |
