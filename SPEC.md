# C++ 仿 LeetCode OJ 项目规格说明书

## 1. 项目概述

本项目是一个面向教学与训练场景的在线判题系统（Online Judge），功能上仿照 LeetCode 的基础做题流程。用户可以在浏览器中查看题目、在线编辑 C++ 代码、运行样例、正式提交并得到自动判题结果。管理员可以维护题目和测试用例。

项目定位：

- 使用场景：教学、训练
- 目标用户：约 1-20 人
- 支持语言：仅 C++
- 部署环境：Windows 笔记本

## 2. 业务目标与成功标准

### 2.1 业务目标

1. 为小规模教学或训练场景提供稳定、易用的 C++ 判题环境。
2. 普通用户可以浏览题目、在线编写 C++ 代码并完成提交。
3. 管理员可以新增、修改、删除题目，并手动导入测试用例。
4. 系统在低并发教学场景下具备一定并发能力，不因单次恶意或失控代码导致服务崩溃。

### 2.2 成功标准

- 系统中至少有 3 道可正常判题的 C++ 题目。
- 系统至少支持 20 名用户注册、登录和提交。
- 正常负载下，从用户提交到返回判题结果的端到端时间不超过 500ms。
- 普通用户无法访问管理员接口或修改题目。
- 用户提交的死循环、超大内存占用或异常代码不会拖垮 Web 服务。

## 3. 范围

### 3.1 本期包含

- 用户注册、登录、退出
- 预置管理员账号
- 题目列表
- 题目详情页，包含题目描述和在线代码编辑器
- 运行样例
- 正式提交与自动判题
- 管理员题目管理，包括新增、编辑、删除题目
- 管理员手动导入测试用例
- 独立判题服务与用户代码子进程隔离

### 3.2 本期不包含

- C++ 以外的语言
- 提交历史页面
- 讨论区
- 比赛系统
- 用户成绩排名
- 生产级集群部署
- 云原生或容器化部署
- 每个题目单独配置时间和内存限制

## 4. 用户角色

| 角色 | 权限 |
| --- | --- |
| 未登录用户 | 可查看题目列表，不可进入题目详情、编辑或提交 |
| 普通用户 | 查看题目列表和题目详情，运行样例，正式提交 |
| 管理员 | 拥有普通用户全部权限，并新增、修改、删除题目，导入测试用例 |

管理员账号通过配置预置，不通过公开注册流程创建。

## 5. 用户流程

### 5.1 做题流程

1. 用户打开题目列表。
2. 登录或注册。
3. 点击题目进入题目详情页。
4. 阅读题目描述和可见样例。
5. 在在线编辑器中编写 C++ 代码。
6. 点击“运行样例”，系统仅使用可见样例测试点判题，返回样例通过或未通过。
7. 点击“提交”，系统使用全部测试点判题，返回 `AC / WA / TLE / RE / CE` 等结论。

### 5.2 管理流程

1. 管理员登录。
2. 进入题目管理页面。
3. 新增题目并填写标题、描述。
4. 手动导入 1-5 组测试用例。
5. 保存题目，题目进入题目列表。

## 6. 功能需求

### 6.1 用户认证

- 支持用户名、密码注册。
- 支持登录和退出。
- 密码必须安全哈希后存储，不得明文保存。
- 登录状态使用 Session/Cookie 维持。
- 系统提供预置管理员账号。

### 6.2 题目浏览

- 题目列表展示题目编号、标题等基础信息。
- 题目详情展示题目描述和普通用户可见的样例测试点。
- 普通用户只能查看题目，不能修改题目或测试用例。

### 6.3 在线编辑与运行

- 题目详情页包含在线代码编辑器。
- 用户可编写 C++17 代码。
- “运行样例”只运行普通用户可见的样例测试点。
- “运行样例”只返回样例通过或样例未通过，不展示程序真实输出。

### 6.4 自动判题

- 仅支持 C++。
- 使用 `g++` 编译，语言标准为 C++17。
- 用户程序通过标准输入输出交互。
- 判题结果不展示用户程序真实输出。

判题结论至少包括：

- `AC`：Accepted，答案正确
- `WA`：Wrong Answer，答案错误
- `TLE`：Time Limit Exceeded，运行超时
- `RE`：Runtime Error，运行时错误
- `CE`：Compile Error，编译错误

编译错误只返回友好的错误名称，不展示 `g++` 原始报错。

### 6.5 管理员题目管理

- 管理员可新增、编辑、删除题目。
- 管理员可手动导入测试用例。
- 每道题测试用例不超过 5 组。
- 测试用例区分样例测试点和隐藏测试点。
- 普通用户只能看到样例测试点，不能看到隐藏测试点。

## 7. 判题规则

### 7.1 全局限制

- 时间限制：1 秒
- 内存限制：128 MB
- 时间和内存限制全局固定，不按题目单独配置。

### 7.2 编译

- 编译器：`g++`
- 语言标准：`-std=c++17`
- 编译成功后运行生成的可执行文件。

### 7.3 运行与隔离

- 每次编译和运行都使用新建子进程。
- 用户代码与 Web 服务和判题服务隔离。
- 超时、死循环或失控进程时，杀死整个子进程树。
- 限制用户代码的网络访问和文件系统访问范围。

### 7.4 输出比较

- 使用忽略末尾空格和换行的友好比较方式。
- 不采用严格的逐字节完全一致比较。

## 8. 技术架构

### 8.1 技术栈

| 层级 | 技术 |
| --- | --- |
| 前端 | 原生 HTML、CSS、JavaScript |
| 后端 API | C++，使用 cpp-httplib |
| 数据库 | MySQL |
| 判题服务 | 独立 C++ 服务 |
| 编译运行 | `g++`，C++17 |
| 部署 | Windows 笔记本 |

### 8.2 架构图

```text
+-------------------------------------+
|              浏览器                    |
|       原生 HTML + CSS + JS             |
+------------------+------------------+
                   | HTTP
                   v
+-------------------------------------+
|           Web / API 服务              |
|        C++ + cpp-httplib             |
|                                       |
|  - 用户认证与 Session/Cookie          |
|  - 题目列表与详情 API                 |
|  - 管理端题目 API                     |
|  - 静态资源服务                       |
+--------+-------------------+---------+
         |                   |
         |                   | SQL
         |                   v
         |          +----------------+
         |          |     MySQL       |
         |          |  users          |
         |          |  problems       |
         |          |  test_cases     |
         |          |  submissions    |
         |          +----------------+
         |
         | 写入待判题任务，轮询判题结果
         v
+-------------------------------------+
|           独立判题服务                 |
|            C++ Judge Worker          |
|                                       |
|  - 固定数量判题 worker                |
|  - 编译用户代码                        |
|  - 子进程运行与资源限制                |
|  - 判题结论回写 MySQL                  |
+------------------+------------------+
                   |
                   v
            编译 / 运行子进程
            g++ -std=c++17
```

### 8.3 服务边界

- Web/API 服务负责 HTTP、认证、静态资源、题目查询和提交入口。
- 判题服务作为独立服务运行，不直接暴露 HTTP 接口。
- Web 服务通过数据库中的提交任务状态与判题服务协作。
- 判题服务使用固定数量的 worker 消费待判题任务。

### 8.4 项目目录结构

```text
cppojvibecoding/
├── .gitignore
├── README.md
├── SPEC.md
├── DEPENDENCIES.md
├── CMakeLists.txt
├── src/
│   ├── common/
│   │   ├── config.h
│   │   ├── database.h
│   │   ├── json_utils.h
│   │   └── logging.h
│   ├── web_server/
│   │   ├── main.cpp
│   │   ├── http_service.h
│   │   ├── auth_service.h
│   │   ├── problem_service.h
│   │   └── admin_service.h
│   └── judge/
│       ├── main.cpp
│       ├── judge_worker.h
│       ├── compiler.h
│       ├── runner.h
│       └── process_guard.h
├── include/
│   └── oj/
│       ├── api.h
│       ├── models.h
│       └── verdict.h
├── web/
│   ├── index.html
│   ├── login.html
│   ├── problems.html
│   ├── problem.html
│   ├── admin.html
│   ├── css/
│   │   └── style.css
│   └── js/
│       ├── api.js
│       ├── auth.js
│       ├── problems.js
│       ├── problem.js
│       └── admin.js
├── scripts/
│   ├── install_dependencies.ps1
│   └── init_db.sql
├── tests/
│   ├── unit/
│   └── integration/
├── build/
└── .tools/
```

说明：

- `src/common`：数据库访问、配置、JSON 和日志等公共模块。
- `src/web_server`：HTTP 服务入口和认证、题目、管理端 API。
- `src/judge`：独立判题服务，负责编译、运行、资源限制和结果回写。
- `include/oj`：供各模块共享的数据模型、接口定义和判题结论定义。
- `web`：原生 HTML、CSS、JavaScript 前端页面。
- `scripts`：依赖安装和数据库初始化脚本。
- `tests`：单元测试与集成测试。
- `build`：CMake 生成的构建产物，应加入 `.gitignore`。
- `.tools`：本地便携工具链，已加入 `.gitignore`。

## 9. 数据模型

### 9.1 users

- `id`：`BIGINT UNSIGNED AUTO_INCREMENT`，主键
- `username`：`VARCHAR(50) NOT NULL`，唯一用户名
- `password_hash`：`VARCHAR(255) NOT NULL`，密码安全哈希
- `role`：`ENUM('user', 'admin') NOT NULL DEFAULT 'user'`
- `created_at`：`DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP`
- 唯一键：`UNIQUE KEY uk_users_username (username)`

### 9.2 problems

- `id`：`BIGINT UNSIGNED AUTO_INCREMENT`，主键
- `title`：`VARCHAR(200) NOT NULL`
- `description`：`MEDIUMTEXT NOT NULL`
- `difficulty`：`ENUM('easy', 'medium', 'hard') NOT NULL DEFAULT 'easy'`，难度分级（简单 / 中等 / 困难）
- `created_at`：`DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP`
- `updated_at`：`DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP`

### 9.3 test_cases

- `id`：`BIGINT UNSIGNED AUTO_INCREMENT`，主键
- `problem_id`：`BIGINT UNSIGNED NOT NULL`，所属题目
- `input_data`：`MEDIUMTEXT NOT NULL`，测试点标准输入内容
- `expected_output_data`：`MEDIUMTEXT NOT NULL`，测试点标准输出内容
- `is_sample`：`TINYINT(1) NOT NULL DEFAULT 0`，是否普通用户可见
- `order_index`：`TINYINT UNSIGNED NOT NULL`，测试点顺序
- 唯一键：`UNIQUE KEY uk_test_cases_problem_order (problem_id, order_index)`
- 索引：`KEY idx_test_cases_problem_id (problem_id)`
- 外键：`FOREIGN KEY (problem_id) REFERENCES problems(id) ON DELETE CASCADE`

### 9.4 submissions

- `id`：`BIGINT UNSIGNED AUTO_INCREMENT`，主键
- `user_id`：`BIGINT UNSIGNED NOT NULL`，提交用户
- `problem_id`：`BIGINT UNSIGNED NOT NULL`，所属题目
- `code`：`MEDIUMTEXT NOT NULL`，用户提交的 C++ 代码
- `status`：`ENUM('pending', 'judging', 'done') NOT NULL DEFAULT 'pending'`
- `verdict`：`ENUM('AC', 'WA', 'TLE', 'RE', 'CE') NULL`，判题完成后写入
- `created_at`：`DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP`
- `finished_at`：`DATETIME NULL`
- 索引：`KEY idx_submissions_status (status)`
- 索引：`KEY idx_submissions_problem_id (problem_id)`
- 索引：`KEY idx_submissions_user_id (user_id)`
- 外键：`FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE`
- 外键：`FOREIGN KEY (problem_id) REFERENCES problems(id) ON DELETE CASCADE`

### 9.5 范式与约束说明

- 每张表的字段都只保存单个原子值，不使用逗号分隔列表、JSON 数组或重复列。
- 多个测试点通过 `test_cases` 独立表存储，不重复放在 `problems` 表中，符合第一范式。
- `test_cases` 不再让同一字段同时表示“文本内容”和“文件路径”，避免一个字段承担两种语义。
- `verdict` 仅在判题完成后写入；进行中或等待中的提交保持 `NULL`，避免用空字符串充当缺失值。
- 测试点顺序通过 `problem_id + order_index` 唯一约束保证。

## 10. API 边界

所有接口统一返回 JSON，并约定统一的错误响应结构。

统一错误响应格式：

```json
{
  "error": {
    "code": "NOT_AUTHENTICATED",
    "message": "not authenticated"
  }
}
```

需要补充上下文时可增加 `details` 字段：

```json
{
  "error": {
    "code": "INTERNAL_ERROR",
    "message": "failed to import test cases",
    "details": {
      "detail": "database unavailable"
    }
  }
}
```

### 10.1 认证接口

- `POST /api/auth/register`
- `POST /api/auth/login`
- `POST /api/auth/logout`
- `GET /api/auth/me`

### 10.2 普通用户接口

- `GET /api/problems`：题目列表
- `GET /api/problems/{id}`：题目详情及可见样例
- `POST /api/problems/{id}/run`：运行样例
- `POST /api/problems/{id}/submit`：正式提交并返回判题结论

### 10.3 管理员接口

管理员接口必须单独鉴权，普通用户不可访问。

- `GET /api/admin/problems`：管理端题目列表
- `POST /api/admin/problems`：新增题目
- `PUT /api/admin/problems/{id}`：修改题目
- `DELETE /api/admin/problems/{id}`：删除题目
- `POST /api/admin/problems/{id}/testcases`：导入测试用例

## 11. 非功能需求

### 11.1 性能

- 正常负载下，提交到返回判题结论的端到端时间不超过 500ms。
- 判题服务使用固定数量 worker，避免无限创建子进程。
- 限制单用户提交频率，避免短时间大量提交占满队列。

### 11.2 安全

- 密码使用安全哈希算法存储。
- SQL 查询使用参数化方式，避免 SQL 注入。
- 管理员接口执行角色鉴权。
- 用户代码通过子进程隔离执行。
- 编译和运行时不通过 shell 拼接命令，避免命令注入。
- 限制用户代码的网络、文件系统和进程创建能力。

### 11.3 可扩展性

- Web 服务与判题服务分离。
- 后续可增加判题 worker 数量。
- 后续可替换或扩展数据库任务队列。

### 11.4 成本与部署

- 在单台 Windows 笔记本上运行。
- 使用本机 MySQL 和本机 `g++`。
- 不要求额外云资源或容器平台。

## 12. 边缘案例与异常处理

- 用户提交空代码：返回编译错误，不进入判题流程。
- 用户代码编译失败：返回 `CE`，不展示原始编译器输出。
- 用户代码死循环或运行超时：杀死整个子进程树，返回 `TLE`。
- 用户代码分配大量内存：受内存限制，返回运行时错误或资源限制错误。
- 用户代码试图创建大量子进程：限制并清理进程树。
- 输出过大：截断或限制输出，避免服务端资源耗尽。
- 用户短时间重复提交：返回频率限制提示。
- 用户名重复注册：返回明确错误信息。
- 密码错误：返回登录失败，不泄露具体账号状态。
- 未登录访问受保护接口：返回未认证错误。
- 普通用户访问管理员接口：返回无权限错误。
- 测试用例格式错误或缺失：管理员导入时校验并拒绝。
- 删除已有提交的题目：明确处理策略，避免产生悬空提交记录。
- MySQL 或编译器不可用：返回服务不可用错误，不让进程崩溃。

## 13. UI/UX 边界

### 13.1 落地页

- 作为站点入口（`/index.html`），访客与登录用户均可访问。
- 首屏展示品牌、平台定位与主要入口（开始刷题、登录 / 注册）。
- 展示判题流程与核心能力说明，视觉风格与登录页、题库页保持一致。
- 已登录用户显示当前账号信息，管理员额外显示管理入口。
- 其余页面左上角的品牌标识（C++ CodeHunter）点击后返回落地页。

### 13.2 题目列表页

- 展示题目编号、标题等核心信息。
- 点击进入题目详情页。
- 未登录用户可以浏览题目列表；“已通过”标记仅在登录后展示。
- 展示难度标签（简单 / 中等 / 困难），支持按难度筛选，并可与标题搜索叠加使用。

### 13.3 题目详情页

- 左侧或上侧展示题目描述和可见样例。
- 右侧或下侧展示在线代码编辑器。
- 提供“运行样例”和“提交”两个明确操作。
- 提交后展示判题结论。
- 未登录用户可以预览题目描述与样例，此时编辑器区域展示登录入口；运行样例与提交判题需要登录。
- 题目头部展示难度标签。

### 13.4 登录注册页

- 提供用户名、密码输入。
- 提供注册、登录切换。
- 退出操作清晰可见。

### 13.5 管理员页面

- 提供题目列表管理。
- 提供新增、编辑、删除操作。
- 提供测试用例导入入口。

界面不展示用户程序真实输出、不展示隐藏测试点、不展示 `g++` 原始报错。

## 14. 潜在风险与权衡

### 14.1 Windows 隔离能力弱于 Linux

Windows 上资源隔离能力与 Linux 不同，用户代码仍可能尝试制造系统压力。通过新建子进程、进程树清理、超时和内存限制降低风险，但不能等同于容器级安全。

### 14.2 子进程编译运行带来的延迟

每次编译和运行需要创建子进程，可能影响 500ms 目标。使用独立判题服务和固定 worker 控制并发，避免 Web 服务直接执行用户代码。

### 14.3 同步等待与队列等待

并发提交集中时可能出现排队。MVP 优先保持前端交互简单，通过 worker 数量、频率限制和统一错误响应控制影响。

### 14.4 数据库任务队列

为保持 MVP 简单，判题任务通过数据库状态协作，不引入额外消息队列。低并发下足够使用。

### 14.5 全局统一限制

时间和内存限制全局固定，不做按题配置。实现简单，但牺牲了不同题目的灵活性。

## 15. TODO 清单

### 阶段一：基础环境

- [x] 初始化项目目录结构
- [x] 配置 MySQL 数据库
- [x] 建立用户、题目、测试用例、提交记录表
- [x] 配置预置管理员账号
- [x] 确认本机 `g++` 和 C++17 编译环境
- [x] 实现配置管理
- [x] 实现日志封装

### 阶段二：后端 Web/API

- [x] 实现 cpp-httplib HTTP 服务
- [x] 实现注册、登录、退出、当前用户接口
- [x] 实现 Session/Cookie 会话管理
- [x] 实现密码哈希
- [x] 实现题目列表和题目详情接口
- [x] 实现运行样例接口
- [x] 实现提交判题接口
- [x] 实现管理员题目增删改接口
- [x] 实现管理员测试用例导入接口
- [x] 实现普通用户与管理员接口鉴权
- [x] 实现统一 JSON 错误响应

### 阶段三：独立判题服务

- [x] 实现判题 worker 与固定并发数
- [x] 实现待判题任务读取
- [x] 实现 `g++ -std=c++17` 编译
- [x] 实现子进程创建与进程树清理
- [x] 实现 1 秒时间限制
- [x] 实现 128 MB 内存限制
- [x] 实现忽略末尾空格和换行的输出比较
- [x] 实现 `AC / WA / TLE / RE / CE` 结论回写
- [x] 实现编译错误友好处理
- [x] 实现单用户提交频率限制

### 阶段四：前端页面

- [x] 实现落地页
- [x] 实现登录注册页
- [x] 实现题目列表页
- [x] 实现题目详情页
- [x] 实现在线代码编辑器
- [x] 实现运行样例交互
- [x] 实现提交与结果展示
- [x] 实现管理员题目管理页
- [x] 实现管理员测试用例导入
- [x] 实现管理员用户管理页（查看、删除普通用户）

### 阶段五：验收与部署

- [x] 在 Windows 笔记本本地部署并运行
- [x] 准备至少 3 道 C++ 测试题目（已导入 10 道，覆盖简单 / 中等 / 困难）
- [x] 准备 20 个测试用户（student01 ~ student20，初始密码 student123）
- [ ] 执行功能验收
- [ ] 执行性能验收
- [ ] 执行安全验收

## 16. 验收标准

页面级功能验收由 `tests/e2e` 下的 Playwright 用例自动执行（复用系统 Edge，运行方式见 README）：

```powershell
Set-Location tests\e2e
npm test
```

完整的自动化测试流程（分层、用例映射、执行顺序、结论记录）见 `web自动化测试流程.md`。

### 16.1 功能验收

- [x] 用户可以注册、登录、退出
- [x] 管理员账号可以正常登录
- [x] 未登录用户可以查看题目列表并预览题目详情、样例
- [x] 登录用户可以查看题目详情并提交代码
- [x] 未登录用户在详情页无法运行样例或提交判题
- [x] 至少 3 道题可以正常编译、运行和判题
- [ ] `AC`、`WA`、`TLE`、`RE`、`CE` 均能正确反馈
- [ ] 编译错误不展示 `g++` 原始报错
- [x] 管理员可以新增、编辑、删除题目
- [x] 管理员可以手动导入测试用例
- [x] 每道题测试用例不超过 5 组
- [x] 普通用户只能看到样例测试点
- [x] 普通用户不能访问管理员接口

### 16.2 性能验收

- [ ] 在 20 名用户、正常教学提交频率下，提交到返回判题结论的端到端时间不超过 500ms
- [ ] 用户提交死循环代码时，判题服务能及时终止进程并返回 `TLE`
- [ ] 连续提交不会导致系统崩溃或无限创建进程

### 16.3 安全验收

- [ ] 密码不以明文存储
- [ ] SQL 查询不存在明显的 SQL 注入
- [ ] 普通用户无法新增、修改或删除题目
- [ ] 用户代码不能访问服务端敏感文件
- [ ] 用户代码不能建立外部网络连接
- [ ] 用户代码无法通过命令注入调用 shell

### 16.4 部署验收

- [x] 系统可以在 Windows 笔记本本地启动
- [x] 前端页面、后端 API、MySQL 和独立判题服务可以协同工作
- [x] 重启服务后已导入的题目和测试用例仍然存在
