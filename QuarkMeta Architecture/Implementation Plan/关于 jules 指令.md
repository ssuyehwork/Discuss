在配合 **Google Jules** 协作开发时，开发者最常用到的 **高频 Git 指令**（用于与 Jules 提交的分支进行拉取、对账、本地验证和强行对齐）主要分为以下 4 类：

---

### 一、 检出与测试 Jules 生成的代码分支

Jules 在云端跑完后会推一个专有分支（并在 GitHub 上建 PR），本地拉取并测试它的指令：

```bash
# 1. 抓取远程所有最新分支（包括 Jules 新建的分支）
git fetch origin

# 2. 切换到 Jules 提交的具体分支进行本地编译测试
git checkout <jules的分支名>

# 3. 如果本地有残留临时文件，一键清理干净
git clean -fd
```

---

### 二、 放弃本地修改，强制对齐（你提到的场景）

当本地代码被改乱、想彻底以远程仓库或 Jules 最新代码为准时：

```bash
# 1. 强制放弃本地所有未提交修改，与远程主分支绝对对齐
git reset --hard origin/main

# 2. 强制把当前分支重置到 Jules 远程分支的最新状态
git reset --hard origin/<jules的分支名>

# 3. 彻底删除本地所有未跟踪的新建垃圾文件/文件夹
git clean -fdx
```

---

### 三、 验证通过后，将 Jules 的代码合并入本地主分支

如果你在本地测试 Jules 的代码没有问题，直接合并：

```bash
# 1. 切回本地主分支
git checkout main

# 2. 拉取远程主分支最新状态
git pull origin main

# 3. 合并 Jules 的远程分支
git merge origin/<jules的分支名>

# 4. 推送到远程主分支
git push origin main
```

---

### 四、 清理已完成或废弃的 Jules 分支

当 Jules 的 PR 合并或关闭后，清理本地过期的分支引用：

```bash
# 一键清理所有在远程已被删除的 Jules 分支缓存
git fetch --all --prune

# 删除本地残留的 Jules 分支
git branch -D <jules的分支名>
```

---

### 总结：
配合 Jules 最核心的本地三板斧就是：**`git fetch`（拉分支）** $\rightarrow$ **`git checkout`（去测试）** $\rightarrow$ **`git reset --hard`（对不齐就强行重置）**。

// =================== 2 ===================

**不对！千万不要误会！`git reset --hard` 绝对不是合并，而是“一键彻底抹除、完全覆盖”！**

---

### 一、 `git reset --hard origin/main` 真正干了什么？

它的本质是 **“彻底毁尸灭迹，强制倒退回原点”**：

- 你跟 Jules 聊了半天、改了半天的**所有新代码，会被瞬间全部删光、一行不剩**；
- 它的作用是强行把代码库**倒退/重置**到远程 `origin/main` 的老状态，**新代码 100% 丢失，绝对不会发生任何合并！**

---

### 二、 如果你想让“新旧版本合在一起”，应该怎么做？

1. **真正的合并指令是 `git merge`**：
   - 只有 `git merge` 才会把 Jules 刚才写的新代码和主分支的代码**融合在一起**。
2. **在 GitHub 上的正规流程**：
   - 让 Jules 把改好的代码推成一个分支；
   - 在 GitHub 上点击 **Merge Pull Request**（合并 PR），这才是把新代码正式合入主版本。

---

### 极简记法（避坑警示）：
- **`git reset --hard`** = **“格式化/放弃一切”**（写错了想彻底重来时才用）；
- **`git merge`** = **“新旧融合在一起”**。