# Web 自动化测试流程

本文依据 [SPEC.md](SPEC.md) 第 16 章验收标准，描述 CodeHunter 的 Web 自动化测试怎么做、按什么顺序做、结果怎么记录。

## 1. 测试分层

| 层级 | 工具 | 位置 | 覆盖内容 |
| --- | --- | --- | --- |
| 单元测试 | GoogleTest | `tests/unit/` | 配置、日志、密码、限流、编译器、比较器等内部逻辑 |
| 数据库集成测试 | GoogleTest | `tests/integration/` | MySQL 读写、题目/用例/提交队列 |
| 接口层验收 | curl + PowerShell | [基于curl命令的接口自动化测试.md](基于curl命令的接口自动化测试.md) | 状态码、权限边界、判题结论、频率限制 |
| 页面层验收 | Playwright（复用系统 Edge） | `tests/e2e/` | 交互流程、DOM 断言、跳转与可见性 |
| 性能验收 | PowerShell 并发脚本 | 见第 5 节 | 20 用户并发、500ms 目标、TLE、进程控制 |
| 安全验收 | curl + SQL 检查 | 见第 6 节 | 明文密码、SQL 注入、越权、沙箱逃逸 |

分层原则：能在接口层验证的不要放到页面层；页面层只验证"只有浏览器才能证明的事"（跳转、可见性、交互结果）。

## 2. 环境准备

### 2.1 依赖位置（全部在 D 盘，不上传 Git）

| 依赖 | 位置 | 用途 |
| --- | --- | --- |
| 便携版 Node | `.tools/node` | 运行 Playwright |
| Playwright 测试包 | `tests/e2e/node_modules` | `@playwright/test`，跳过内核下载，直接用系统 Edge |
| MySQL / w64devkit / vcpkg | `.tools/mysql`、`.tools/w64devkit`、`.tools/vcpkg` | 后端、判题、编译依赖 |

### 2.2 启动服务

```powershell
powershell -ExecutionPolicy Bypass -File scripts\start_local.ps1
curl.exe http://127.0.0.1:8080/api/health
```

健康检查应返回 `{"status":"ok"}`。

### 2.3 数据准备（可重复执行，幂等）

```powershell
powershell -ExecutionPolicy Bypass -File scripts\seed_problems.ps1
powershell -ExecutionPolicy Bypass -File scripts\seed_users.ps1
```

期望基线数据：

| 项目 | 期望值 |
| --- | --- |
| 题目 | 10 道（简单 4 / 中等 3 / 困难 3） |
| 测试用例 | 50 组（每题 5 组 = 2 组样例 + 3 组隐藏） |
| 账号 | 23 个（admin + 2 个历史账号 + student01~student20） |

## 3. 页面层自动化（Playwright）

### 3.1 工程结构

```text
tests/e2e/
├── package.json           # npm test / test:headed / report
├── playwright.config.js   # baseURL、channel=msedge、超时、报告
├── helpers.js             # login() 公共登录助手
└── specs/
    ├── guest.spec.js      # 游客验收（5 条）
    ├── student.spec.js    # 学生验收（3 条）
    ├── admin.spec.js      # 管理员验收（2 条）
    └── judge.spec.js      # 判题结论矩阵（5 条：AC/WA/TLE/RE/CE）
```

### 3.2 关键配置

| 配置 | 取值 | 原因 |
| --- | --- | --- |
| `baseURL` | `http://127.0.0.1:8080` | 用例内统一写相对路径 |
| `channel` | `msedge` | 复用系统 Edge，避免下载约 300MB 内核 |
| `workers` | `1` | 提交接口有 3 秒单人限流，串行更稳定 |
| `viewport` | `1440x900` | 桌面验收基准，与人工验收一致 |
| `trace` / `screenshot` | 仅失败时保留 | 便于失败定位且不占空间 |
| `reporter` | `list` + `html(report/)` | 控制台可读 + 可回溯报告 |

### 3.3 用例与验收条目映射

| SPEC 16.1 验收条目 | 对应用例 | 关键断言 |
| --- | --- | --- |
| 用户可以注册、登录、退出 | `student.spec.js` 登录 / 退出两条；注册由 `scripts/seed_users.ps1` 走注册接口完成 | `#user-name` 显示用户名；退出后 URL 回到 `login.html` |
| 管理员账号可以正常登录 | `admin.spec.js` 管理入口用例 | `#user-role` 为"管理员"；管理/用户导航可见 |
| 未登录用户可以查看题目列表并预览题目详情、样例 | `guest.spec.js` 题库 / 详情两条 | 题库停在 `problems.html` 不跳登录；`#problem-title`、样例可见 |
| 登录用户可以查看题目详情并提交代码 | `student.spec.js` 提交用例 | 运行样例返回 `PASS`，提交返回 `AC` |
| 未登录用户在详情页无法运行样例或提交判题 | `guest.spec.js` 详情用例 + 接口层 401 | `#workspace-editor` 隐藏、`#workspace-guest` 可见 |
| 至少 3 道题可以正常编译、运行和判题 | 接口层判题矩阵（第 4 节） | 多题返回 `AC` |
| `AC` / `WA` / `TLE` / `RE` / `CE` 均能正确反馈 | `judge.spec.js` 五条用例（student05~student09 各提交一次） | `#result-verdict` 依次为 AC / WA / TLE / RE / CE，`#result-message` 为对应中文提示 |
| 编译错误不展示 `g++` 原始报错 | `judge.spec.js` CE 用例 + 第 4.2 节的接口响应检查 | 接口返回体只有 `{"submission_id":..,"verdict":"CE"}`，不含编译日志 |
| 管理员可以新增、编辑、删除题目 | `admin.spec.js` 增删用例；编辑由种子脚本的更新接口覆盖 | 创建后列表 +1、删除后回到 10 条 |
| 管理员可以手动导入测试用例 | `scripts/seed_problems.ps1`（走导入接口） | 每题用例数 = 5，样例 2 组 |
| 每道题测试用例不超过 5 组 | 数据库核对（第 4.3 节） | 每题 `COUNT(test_cases) <= 5` |
| 普通用户只能看到样例测试点 | 接口层：`GET /api/problems/{id}` 未登录返回 | 响应只含 `is_sample = 1` 的用例 |
| 普通用户不能访问管理员接口 | 接口层：普通用户请求 `/api/admin/*` | 返回 `403 ADMIN_REQUIRED` |

额外补充的页面用例（不在 SPEC 逐条列出，但属于回归重点）：

- 难度筛选：选"困难"显示 3 道、选"简单"显示 4 道、选"全部"恢复 10 道。
- 左上角品牌可返回落地页（对应早前"未登录点预览被强制跳登录"的回归点）。

### 3.4 执行流程

1. 启动服务（第 2.2 节）并确认健康检查通过。
2. 准备基线数据（第 2.3 节）。
3. 运行用例：

```powershell
$env:PATH = "$PWD\.tools\node;$env:PATH"
Set-Location tests\e2e
npm test                # 无头运行，默认复用系统 Edge
npm run test:headed     # 需要肉眼观察操作过程时
```

4. 预期输出：`15 passed`（当前实测约 27 秒，有头模式）。
5. 查看报告：`npm run report`，或直接打开 `tests/e2e/report/index.html`。
6. 记录结论：按第 7、8 节更新 SPEC 勾选与本文件执行记录。

### 3.5 失败排查

| 现象 | 可能原因 | 处理 |
| --- | --- | --- |
| 全部用例网络错误 | 服务未启动 | 执行 `scripts\start_local.ps1` |
| 断言拿到旧界面 | 浏览器缓存了旧资源 | 页面已加 `Cache-Control: no-cache` + 资源版本号；必要时 `Ctrl+F5` |
| 提交用例返回 429 | 触发 3 秒单人限流 | 让不同用例使用不同 `studentNN`，或增加等待 |
| 启动报找不到浏览器 | 系统无 Edge | 安装 Edge，或在 `playwright.config.js` 中改用其他 `channel` |
| 题目/账号数量断言失败 | 数据被手工改动 | 重新执行种子脚本恢复基线 |
| 管理员用例残留临时题目 | 用例中途失败 | 手动在管理端删除，或重跑用例（脚本会自清理） |

失败时可在 `tests/e2e/test-results/` 找到 trace，用 `npx playwright show-trace <trace.zip>` 回放。

### 3.6 用例编写规范

1. 选择器优先用 `id`（如 `#code-editor`），其次用稳定类名（如 `.problem-item`），避免用中文文案定位。
2. 用 `expect(...).toHaveText / toHaveCount / toHaveURL` 自动等待，不要写固定 `sleep`。
3. 每条用例使用独立浏览器上下文，不复用登录态，避免相互污染。
4. 涉及提交判题的用例每人只提交一次（3 秒限流），多人并发时按 `studentNN` 分配账号。
5. 会写数据的用例必须自清理（新增题目后立即删除），保证可重复执行。
6. 新增用例时同步更新 `3.3` 映射表与 SPEC 勾选状态。

## 4. 接口层流程

### 4.1 执行现有回归脚本

按 [基于curl命令的接口自动化测试.md](基于curl命令的接口自动化测试.md) 的"一键测试脚本"执行，覆盖健康检查、注册登录、题目、运行、提交、限流、权限边界、管理端增删改与用例导入。

### 4.2 判题结论矩阵（页面层已自动化，本节用于接口层复核）

对同一道题依次提交下面五段代码，逐条核对结论：

| 结论 | 提交内容 | 预期返回 | 额外断言 |
| --- | --- | --- | --- |
| `AC` | A+B 正确解 | `verdict = AC` | 消息为"全部测试点通过" |
| `WA` | 固定输出 `1` | `verdict = WA` | 消息提到输出不一致 |
| `TLE` | `while (true) {}` | `verdict = TLE` | 1 秒时限内被杀，进程无残留 |
| `RE` | `int a = 1 / 0;` | `verdict = RE` | 不返回崩溃堆栈细节 |
| `CE` | 缺少分号的代码 | `verdict = CE` | 提示不包含 `g++`、`main.cpp` 等原始报错 |

注意：同一账号提交间隔需大于 3 秒，建议五个结论使用五个不同 `studentNN` 账号。

页面层已由 `tests/e2e/specs/judge.spec.js` 覆盖（2026-10-13 通过）；本节用于复核接口原始响应，确认结论之外不夹带编译日志等内部信息。

### 4.3 数据核对

```powershell
& .\.tools\mysql\bin\mysql.exe -u root -e "SELECT p.id, p.title, COUNT(t.id) AS cases, SUM(t.is_sample) AS samples FROM oj.problems p LEFT JOIN oj.test_cases t ON t.problem_id = p.id GROUP BY p.id, p.title ORDER BY p.id;"
```

核对点：每题用例数 ≤ 5、样例数 ≥ 1、题目总数 10。

## 5. 性能验收流程（SPEC 16.2，待执行）

1. 目标：20 名用户按教学频率并发提交，提交到返回结论的端到端时间 ≤ 500ms。
2. 方法：PowerShell 并发（`ForEach-Object -Parallel` 或 `Start-Job`）驱动 20 个 `studentNN` 账号同时提交同一道题的正确解，避免命中单人限流。
3. 记录指标：p50 / p95 端到端耗时、失败率、`judge_worker` 进程与系统负载。
4. 已知基线：单用户单次提交约 1.1 秒，其中 g++ 编译占主要部分；若并发结果超出 500ms，需要在验收结论中说明口径（是否含编译）并给出优化方向（编译缓存、预热、并行度调整）。
5. 稳定性用例：
   - 提交死循环代码 → 应在 1 秒左右返回 `TLE`，且判题进程无残留；
   - 连续快速提交（不同账号）→ 服务不崩溃、不出现进程无限增长。

## 6. 安全验收流程（SPEC 16.3，待执行）

| 检查项 | 执行方法 | 通过标准 |
| --- | --- | --- |
| 密码不以明文存储 | SQL 查询 `password_hash` | 全部为 `$argon2id$` 开头的哈希 |
| SQL 注入 | 登录与参数接口传入 `' OR 1=1 --` 等载荷 | 返回 401/400，数据表未被篡改 |
| 普通用户越权 | 普通用户调用管理端写接口 | 返回 `403 ADMIN_REQUIRED` |
| 读取服务端文件 | 提交 `ifstream` 读取 `C:\Windows\win.ini` 的代码 | 文件内容不出现在结论中 |
| 建立外网连接 | 提交 socket 连接外部地址的代码 | 连接失败或超时，不产生外部流量 |
| 命令注入 | 提交 `system("whoami")` 之类代码 | 不产生额外进程与输出，结论为 `RE`/`WA` |

## 7. 结论记录规范

1. 每通过一组验收，更新 [SPEC.md](SPEC.md) 第 16 章对应条目的勾选。
2. 在本文件第 8 节追加执行记录（日期、范围、结果、耗时、执行人）。
3. 保留证据：接口响应 JSON、Playwright 报告 `tests/e2e/report/`、关键界面截图。

## 8. 执行记录

| 日期 | 范围 | 结果 | 备注 |
| --- | --- | --- | --- |
| 2026-10-13 | 部署验收（16.4） | 通过 | 启动 3.5 秒；重启后数据一致 |
| 2026-10-13 | 功能验收（16.1，Playwright 15 条，有头模式） | 通过 | `15 passed / 26.9 秒`，覆盖游客、学生、管理员与判题结论矩阵，报告见 `tests/e2e/report/` |
| 2026-10-13 | 功能验收（16.1，接口层复核 CE / RE 响应） | 通过 | 响应体仅含 `submission_id` 与 `verdict`，无编译日志 |
| 2026-10-13 | 性能验收（16.2） | 待执行 | 单用户基线约 1.1 秒 |
| 2026-10-13 | 安全验收（16.3） | 待执行 | — |

### 8.1 缺陷记录

| 日期 | 现象 | 根因 | 处理 |
| --- | --- | --- | --- |
| 2026-10-13 | 提交空指针崩溃的代码被判为 `TLE`，应为 `RE` | Windows 错误报告（WerFault）会挂住崩溃进程，判题侧等满 1 秒时间限制后按超时处理 | 在 `src/judge/process_runner.cc` 创建子进程前调用 `SetErrorMode`（`SEM_FAILCRITICALERRORS` 与 `SEM_NOGPFAULTERRORBOX`），子进程继承后立即以非 0 退出码结束，判定为 `RE`；重跑用例 2.3 秒返回并通过 |

## 9. 命令速查

| 目的 | 命令 |
| --- | --- |
| 启动服务 | `powershell -ExecutionPolicy Bypass -File scripts\start_local.ps1` |
| 停止服务 | `powershell -ExecutionPolicy Bypass -File scripts\stop_local.ps1` |
| 健康检查 | `curl.exe http://127.0.0.1:8080/api/health` |
| 导入题库 | `powershell -ExecutionPolicy Bypass -File scripts\seed_problems.ps1` |
| 导入测试用户 | `powershell -ExecutionPolicy Bypass -File scripts\seed_users.ps1` |
| 运行页面测试 | `Set-Location tests\e2e; npm test` |
| 查看测试报告 | `Set-Location tests\e2e; npm run report` |
| 单元 + 集成测试 | `& .\.tools\w64devkit\w64devkit\bin\ctest.exe --test-dir build --output-on-failure` |

运行页面测试前记得把便携 Node 加入 PATH：

```powershell
$env:PATH = "$PWD\.tools\node;$env:PATH"
```
