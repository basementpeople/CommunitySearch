# CommunitySearch Git 提交与分支规范

这份文档用于规范日常 Git 开发流程，帮助你长期保持提交记录清晰、可回溯、易协作。

## 1. 总体原则

- 后续改动建议使用 **分支 + PR** 流程，不要长期直接在 `master/main` 上开发。
- 小步提交、清晰描述，避免“攒一大坨再提交”。
- 提交信息尽量表达“这次改动的目的”，而不是只写“改了什么文件”。

## 2. 分支命名规范

格式：`类型/简短描述`

常用类型：

- `feat/xxx`：新功能
- `fix/xxx`：Bug 修复
- `docs/xxx`：文档更新
- `refactor/xxx`：重构（不改变外部功能）
- `test/xxx`：测试相关
- `chore/xxx`：工程杂项（依赖、脚本、配置等）

示例：

- `feat/search-keyword-highlight`
- `fix/empty-query-crash`
- `docs/update-quickstart`

命名建议：

- 全小写
- 单词用 `-` 连接
- 3~6 个词，能表达改动意图即可

## 3. 提交信息规范（Conventional Commits）

推荐格式：

`type: 动词 + 目的`

示例：

- `feat: add tag filter for search results`
- `fix: prevent crash when query is empty`
- `docs: clarify first-time remote setup`
- `refactor: simplify search service validation`

尽量避免：

- `update`
- `modify file`
- `fix bug`（过于笼统）

## 4. 每次开发的标准流程

```bash
# 1) 确保主分支最新
git checkout master
git pull

# 2) 创建新分支
git checkout -b feat/your-change

# 3) 开发后提交
git add .
git commit -m "feat: your concise message"

# 4) 推送分支并建立上游跟踪
git push -u origin feat/your-change
```

然后在 GitHub 上创建 PR，合并到 `master/main`。

## 5. 合并后的清理流程

```bash
git checkout master
git pull
git branch -d feat/your-change
```

## 6. 什么时候可以直接提交主分支

以下场景可酌情直接提交（仍建议谨慎）：

- 仓库仅你一个人维护
- 改动非常小且低风险（例如 README 文案修正）

除此之外，建议一律走分支流程，后期维护成本会显著更低。

## 7. 快速命令备忘

```bash
# 查看状态
git status

# 查看分支
git branch

# 切换分支
git checkout <branch-name>

# 新建并切换分支
git checkout -b <new-branch-name>

# 查看远程
git remote -v
```

---

如果后续你希望，我还可以在这个文档基础上继续补充：

- 适配你项目的提交 message 模板
- 常见报错排查（如冲突、推送被拒绝、分支落后）
- PR 描述模板（便于自己回顾或团队协作）
