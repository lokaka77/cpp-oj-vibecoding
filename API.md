# OJ 后端接口文档

## 1. 通用约定

### 1.1 基础地址

默认部署地址：

```text
http://127.0.0.1:8080
```

监听地址和端口来自 `config.yaml` 的：

```yaml
server:
  listen_address: 0.0.0.0
  port: 8080
```

### 1.2 请求格式

- 请求体统一使用 JSON。
- 请求头使用：

```http
Content-Type: application/json
```

### 1.3 登录状态

- 登录成功后，服务端通过 `Set-Cookie` 返回 `session_id`。
- 后续需要登录的接口会自动携带该 Cookie。
- Cookie 属性：

```text
session_id=<token>; Path=/; HttpOnly; SameSite=Lax
```

- Session 当前保存在服务端内存中，服务重启后会失效。

### 1.4 角色

| 角色 | 说明 |
| --- | --- |
| 未登录用户 | 只能访问公开接口 |
| 普通用户 | 可以查看题目、运行样例、提交判题 |
| 管理员 | 拥有普通用户权限，并可管理题目和测试用例 |

### 1.5 统一成功响应

不同接口返回不同业务结构，但都返回 JSON。

### 1.6 统一错误响应

所有错误统一返回：

```json
{
  "error": {
    "code": "NOT_AUTHENTICATED",
    "message": "not authenticated"
  }
}
```

部分错误包含额外信息：

```json
{
  "error": {
    "code": "RATE_LIMITED",
    "message": "submission too frequent",
    "details": {
      "retry_after_ms": 1524
    }
  }
}
```

### 1.7 通用状态码

| 状态码 | 含义 |
| --- | --- |
| `200` | 请求成功 |
| `201` | 资源创建成功 |
| `400` | 请求参数错误或 JSON 格式错误 |
| `401` | 未登录、Session 无效或凭证错误 |
| `403` | 权限不足 |
| `404` | 资源不存在 |
| `409` | 资源冲突，例如用户名重复 |
| `429` | 请求过于频繁 |
| `500` | 服务端内部错误 |
| `503` | 判题服务未在等待时间内完成 |

## 2. 健康检查

### `GET /api/health`

作用：检查 HTTP 服务是否存活。

访问权限：公开。

请求参数：无。

成功响应：

```json
{
  "status": "ok"
}
```

### `GET /`

作用：访问 `web/` 目录下的静态资源。

访问权限：公开。

## 3. 认证接口

### `POST /api/auth/register`

作用：注册普通用户。

访问权限：公开。

请求体：

```json
{
  "username": "alice",
  "password": "secret123"
}
```

参数说明：

| 参数 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `username` | string | 是 | 用户名，非空 |
| `password` | string | 是 | 密码，非空 |

成功响应 `201`：

```json
{
  "id": 2,
  "username": "alice",
  "role": "user"
}
```

可能的错误：

| 状态码 | code | 说明 |
| --- | --- | --- |
| `400` | `INVALID_JSON` | JSON 格式错误 |
| `400` | `VALIDATION_ERROR` | 用户名或密码为空 |
| `409` | `USERNAME_EXISTS` | 用户名已存在 |
| `500` | `INTERNAL_ERROR` | 用户创建失败 |

### `POST /api/auth/login`

作用：用户登录并创建 Session。

访问权限：公开。

请求体：

```json
{
  "username": "alice",
  "password": "secret123"
}
```

成功响应 `200`：

```json
{
  "id": 2,
  "username": "alice",
  "role": "user"
}
```

同时返回：

```http
Set-Cookie: session_id=<token>; Path=/; HttpOnly; SameSite=Lax
```

可能的错误：

| 状态码 | code | 说明 |
| --- | --- | --- |
| `400` | `INVALID_JSON` | JSON 格式错误 |
| `401` | `INVALID_CREDENTIALS` | 用户名或密码错误 |
| `500` | `INTERNAL_ERROR` | Session 创建失败 |

初始管理员账号：

```text
username: admin
password: admin123
```

### `POST /api/auth/logout`

作用：删除当前 Session，并清除 Cookie。

访问权限：公开，可重复调用。

请求参数：无。

成功响应 `200`：

```json
{
  "status": "ok"
}
```

响应会清除 Cookie：

```http
Set-Cookie: session_id=; Path=/; HttpOnly; SameSite=Lax; Max-Age=0
```

### `GET /api/auth/me`

作用：返回当前登录用户信息。

访问权限：需要登录。

请求参数：无。

成功响应 `200`：

```json
{
  "id": 2,
  "username": "alice",
  "role": "user"
}
```

可能的错误：

| 状态码 | code | 说明 |
| --- | --- | --- |
| `401` | `NOT_AUTHENTICATED` | 未登录或 Session 无效 |
| `401` | `USER_NOT_FOUND` | Session 对应的用户不存在 |

## 4. 题目接口

### `GET /api/problems`

作用：获取题目列表。

访问权限：公开。

请求参数：无。

成功响应 `200`：

```json
{
  "problems": [
    {
      "id": 1,
      "title": "A+B Problem"
    }
  ]
}
```

可能的错误：

| 状态码 | code | 说明 |
| --- | --- | --- |
| `500` | `INTERNAL_ERROR` | 题目列表读取失败 |

### `GET /api/problems/{id}`

作用：获取题目详情和普通用户可见的样例测试点。

访问权限：需要登录。

路径参数：

| 参数 | 类型 | 说明 |
| --- | --- | --- |
| `id` | integer | 题目 ID |

成功响应 `200`：

```json
{
  "id": 1,
  "title": "A+B Problem",
  "description": "Given two integers a and b, output a + b.",
  "samples": [
    {
      "id": 1,
      "input": "1 2\n",
      "expected_output": "3\n",
      "order_index": 1
    }
  ]
}
```

说明：

- 只返回 `is_sample = 1` 的测试点。
- 隐藏测试点不会返回给普通用户。

可能的错误：

| 状态码 | code | 说明 |
| --- | --- | --- |
| `401` | `NOT_AUTHENTICATED` | 未登录 |
| `404` | `PROBLEM_NOT_FOUND` | 题目不存在 |
| `500` | `INTERNAL_ERROR` | 数据读取失败 |

### `POST /api/problems/{id}/run`

作用：运行用户代码，只针对可见样例测试点。

访问权限：需要登录。

请求体：

```json
{
  "code": "#include <iostream>\nint main(){return 0;}"
}
```

参数说明：

| 参数 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `code` | string | 是 | C++17 源代码 |

成功响应 `200`：

```json
{
  "passed": true,
  "message": "样例通过"
}
```

可能的消息：

| message | 说明 |
| --- | --- |
| `样例通过` | 所有可见样例通过 |
| `样例未通过` | 输出与期望不一致 |
| `编译错误` | 编译失败，不返回原始 `g++` 报错 |
| `运行时错误` | 程序异常退出或内存超限 |
| `运行超时` | 超过 1 秒运行限制 |

说明：

- 不返回用户程序的真实 `stdout`。
- 不返回隐藏测试点。

可能的错误：

| 状态码 | code | 说明 |
| --- | --- | --- |
| `400` | `INVALID_JSON` | JSON 格式错误 |
| `400` | `VALIDATION_ERROR` | `code` 为空 |
| `401` | `NOT_AUTHENTICATED` | 未登录 |
| `404` | `PROBLEM_NOT_FOUND` | 题目不存在 |
| `500` | `INTERNAL_ERROR` | 样例加载失败 |

### `POST /api/problems/{id}/submit`

作用：提交代码，针对全部测试点判题，并写入提交记录。

访问权限：需要登录。

请求体：

```json
{
  "code": "#include <iostream>\nint main(){return 0;}"
}
```

成功响应 `200`：

```json
{
  "submission_id": 18,
  "verdict": "AC"
}
```

verdict 说明：

| verdict | 含义 |
| --- | --- |
| `AC` | Accepted，全部测试点通过 |
| `WA` | Wrong Answer，答案错误 |
| `TLE` | Time Limit Exceeded，运行超时 |
| `RE` | Runtime Error，运行时错误或内存超限 |
| `CE` | Compile Error，编译错误 |

说明：

- 提交先写入 `pending` 状态。
- 独立 `judge_worker` 领取任务并执行判题。
- Web 服务最多等待约 10 秒获取判题结果。
- 必须同时运行 `web_server` 和 `judge_worker`。
- 单用户默认每 3 秒最多提交一次。

可能的错误：

| 状态码 | code | 说明 |
| --- | --- | --- |
| `400` | `INVALID_JSON` | JSON 格式错误 |
| `400` | `VALIDATION_ERROR` | `code` 为空 |
| `401` | `NOT_AUTHENTICATED` | 未登录 |
| `404` | `PROBLEM_NOT_FOUND` | 题目不存在 |
| `429` | `RATE_LIMITED` | 提交过于频繁 |
| `500` | `INTERNAL_ERROR` | 提交保存失败或结果读取失败 |
| `503` | `JUDGE_TIMEOUT` | 判题未在等待时间内完成 |

提交频率限制响应：

```json
{
  "error": {
    "code": "RATE_LIMITED",
    "message": "submission too frequent",
    "details": {
      "retry_after_ms": 1524
    }
  }
}
```

## 5. 管理员题目接口

所有 `/api/admin/*` 接口都需要管理员角色。

权限错误：

| 状态码 | code | 说明 |
| --- | --- | --- |
| `401` | `NOT_AUTHENTICATED` | 未登录 |
| `403` | `ADMIN_REQUIRED` | 已登录但不是管理员 |

### `GET /api/admin/problems`

作用：获取管理端题目列表。

访问权限：管理员。

成功响应 `200`：

```json
{
  "problems": [
    {
      "id": 1,
      "title": "A+B Problem"
    }
  ]
}
```

### `POST /api/admin/problems`

作用：新增题目。

访问权限：管理员。

请求体：

```json
{
  "title": "New Problem",
  "description": "Problem description"
}
```

参数说明：

| 参数 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `title` | string | 是 | 题目标题，非空 |
| `description` | string | 否 | 题目描述 |

成功响应 `201`：

```json
{
  "id": 3,
  "title": "New Problem"
}
```

可能的错误：

| 状态码 | code | 说明 |
| --- | --- | --- |
| `400` | `INVALID_JSON` | JSON 格式错误 |
| `400` | `VALIDATION_ERROR` | 标题为空 |
| `500` | `INTERNAL_ERROR` | 题目创建失败 |

### `PUT /api/admin/problems/{id}`

作用：修改题目。

访问权限：管理员。

路径参数：

| 参数 | 类型 | 说明 |
| --- | --- | --- |
| `id` | integer | 题目 ID |

请求体：

```json
{
  "title": "Updated Problem",
  "description": "Updated description"
}
```

成功响应 `200`：

```json
{
  "status": "ok"
}
```

可能的错误：

| 状态码 | code | 说明 |
| --- | --- | --- |
| `400` | `INVALID_JSON` | JSON 格式错误 |
| `400` | `VALIDATION_ERROR` | 标题为空 |
| `403` | `ADMIN_REQUIRED` | 不是管理员 |
| `404` | `PROBLEM_NOT_FOUND` | 题目不存在 |
| `500` | `INTERNAL_ERROR` | 更新失败 |

### `DELETE /api/admin/problems/{id}`

作用：删除题目。

访问权限：管理员。

路径参数：

| 参数 | 类型 | 说明 |
| --- | --- | --- |
| `id` | integer | 题目 ID |

成功响应 `200`：

```json
{
  "status": "ok"
}
```

说明：

- 删除题目会级联删除该题目的测试用例和提交记录。

可能的错误：

| 状态码 | code | 说明 |
| --- | --- | --- |
| `403` | `ADMIN_REQUIRED` | 不是管理员 |
| `404` | `PROBLEM_NOT_FOUND` | 题目不存在 |
| `500` | `INTERNAL_ERROR` | 删除失败 |

### `POST /api/admin/problems/{id}/testcases`

作用：导入或替换题目的测试用例。

访问权限：管理员。

路径参数：

| 参数 | 类型 | 说明 |
| --- | --- | --- |
| `id` | integer | 题目 ID |

请求体：

```json
{
  "test_cases": [
    {
      "input": "1 2\n",
      "expected_output": "3\n",
      "is_sample": true
    },
    {
      "input": "10 20\n",
      "expected_output": "30\n",
      "is_sample": false
    }
  ]
}
```

参数说明：

| 参数 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `test_cases` | array | 是 | 测试用例列表，数量 1-5 |
| `test_cases[].input` | string | 是 | 标准输入 |
| `test_cases[].expected_output` | string | 是 | 期望输出 |
| `test_cases[].is_sample` | boolean | 否 | 是否为可见样例，默认 `false` |

成功响应 `200`：

```json
{
  "status": "ok",
  "count": 2
}
```

说明：

- 导入采用整题替换语义：先删除该题旧测试用例，再写入新的测试用例。
- `order_index` 按数组顺序自动生成。
- 至少要有一组 `is_sample = true` 的测试用例。
- 最多 5 组测试用例。

可能的错误：

| 状态码 | code | 说明 |
| --- | --- | --- |
| `400` | `INVALID_JSON` | JSON 格式错误 |
| `400` | `VALIDATION_ERROR` | 测试用例格式或数量不合法 |
| `403` | `ADMIN_REQUIRED` | 不是管理员 |
| `404` | `PROBLEM_NOT_FOUND` | 题目不存在 |
| `500` | `INTERNAL_ERROR` | 测试用例导入失败 |

## 6. 判题限制与流程

### 6.1 限制

| 项目 | 默认值 | 配置项 |
| --- | --- | --- |
| 用户程序时间限制 | `1000ms` | `judge.time_limit_ms` |
| 用户程序内存限制 | `128MB` | `judge.memory_limit_mb` |
| 单用户提交间隔 | `3000ms` | `judge.submit_interval_ms` |
| 判题 worker 数 | `2` | `judge.worker_count` |

### 6.2 判题流程

```text
POST /api/problems/{id}/submit
        |
        v
写入 submissions 表，status=pending
        |
        v
judge_worker 领取任务，status=judging
        |
        v
编译 + 运行全部测试点
        |
        v
写入 status=done、verdict、finished_at
        |
        v
Web 服务读取结果并返回 verdict
```

### 6.3 编译错误处理

- 编译失败返回 `CE`。
- 用户只能看到友好消息 `编译错误`。
- `g++` 原始报错只写入服务端日志，不通过 API 返回。
