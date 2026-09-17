# MiniVim

> 一个面向大一学生的 C++17 课程项目: 从零实现一个可以在终端中运行的, 具有基本 Vim 操作方式的文本编辑器.

> 对于该项目中出现的快捷键简记方式的介绍, 可以在 Vim 中执行 `:help key-notation` 或访问
> [官方文档](https://vimdoc.sourceforge.net/htmldoc/intro.html#key-notation) 来查看.

> 在 vscode 中,你可以用 `<C-S-v>` 来启动 vscode 内置的 Markdown 阅读模式, 读起来更轻松

如果你只是想学习如何使用 Vim, 请阅读 [Vim 入门笔记](./IntroForVim.md).

---

## 学习目标

完成项目后, 你应该能够:

- 熟悉 C++ 的基本 **语法** 与 STL, 如 std::filesystem, std::optional, std::vector;
- 掌握基本的 **模块化设计** 思想, 将一个复杂问题拆分成若干职责明确, 相互协作的模块;
- 理解 **封装** 的意义, 通过类的公开接口隐藏内部状态和实现细节;
- 熟悉 C++ 的基本面向对象程序设计方法;
- 建立基本的 **单元测试** 意识: 将容易独立验证的逻辑从交互式程序中分离出来, 并通过测试检查模块行为和边界情况;
- 对 Vim 之类的文本编辑器的底层设计实现有新的了解.

## 框架

对 MiniVim 的 Basic 部分, 我们准备了一个框架来方便完成:

```text
MiniVim
├── Makefile
├── src
│   ├── *.hpp
│   └── *.cpp
└── test
    └── *.cpp
```

完成 MiniVim 不一定需要使用我们提供的框架, 如果你愿意 everything starts from scratch 也是可以正常完成 MiniVim
并通过评测的; 为了简化 Basic 部分的实现流程, 我们建议使用该框架.

### 框架结构

我们在 `src/` 内的文件内留了很多注释, 它们有些是讲解设计逻辑的, 有些是讲解接口要求的. **请不要忽视这些注释**.

你需要完成的核心模块包括:

| 模块                  | 作用                               |
| --------------------- | ---------------------------------- |
| `TextLayout`          | 缓冲区列与屏幕列之间的转换         |
| `Buffer`              | 文件内容, 文本修改, 修改标记与保存 |
| `Window`              | 光标, 期望列与视口                 |
| `NormalCommandParser` | Normal 模式命令解析                |
| `Renderer`            | 从状态生成一帧 ANSI 文本           |
| `Editor`              | 事件循环, 模式切换与动作执行       |
| `main`                | 参数处理, 对象创建与顶层错误处理   |

请注意, 如果你修改了已有的 public 接口, 可能会导致 test 模块无法正常工作. 框架中提供的 test 是通过这些接口进行测试的.

### 环境安装

```sh
# 安装基本编译工具
sudo apt install build-essential
# 检查
g++ --version
make --version
```

### 运行

```sh
# 编译 MiniVim 到 code
make
# 运行 MiniVim
./code [parameters...]
```

为了能够在 ACMOJ 上进行评测, 需要把可执行文件放到 `code` 处; 如果你想修改 `make` 得到的可执行文件名 (如 `mvim`),
你需要修改 `Makefile` 中 `TARGET` 的值:

```Makefile
TARGET := code
```

### 测试

```sh
# 编译并运行测试模块
make test
```

### 清理编译中间文件

```sh
make clean
```

### Hint: 一次按键如何流过系统

![MiniVim 按键处理流程](./docs/images/key-processing-flow.png)

`Editor::processKey ()` 是模式分发的唯一入口. 只有 Normal 模式会先经过 `NormalCommandParser` 并产生 `EditorAction`;
Insert 和 Command-line 模式分别由 `Editor` 中对应的 handler 直接处理. 三条路径都在 `Editor` 中完成本次按键的语义;
如果程序继续运行, `Renderer` 再根据 `Buffer`, `Window` 和 `RenderState` 生成下一帧, 最后交给 `Terminal` 输出.

---

## 项目要求

### Basic

你必须实现 Basic 部分才能获得基础分数并继续完成 Advanced 部分与 Extra 部分. Basic 部分的最终得分即为 ACMOJ
上评测得到的分数.

#### Basic 概述

MiniVim 的哲学是模态编辑, 高效的编辑操作依托各模式间的切换完成. 简而言之, 在 Basic 部分中, 你需要为 MiniVim 实现最基础的
3 种模式:

- **Normal**: 移动光标, 执行编辑动作或进入其他模式;
- **Insert**: 插入和删除文本;
- **Command-line**: 输入保存, 退出等以 `:` 开始的命令;

并为每个模式实现一些最基础的功能.

在 Basic 部分, 你可以简单地认为屏幕由两部分构成: 屏幕最下面一行的保留部分 (用于输入命令, 显示信息等) ,
以及上面其余的编辑部分, 即缓冲区. Normal 模式与 Insert 模式主要操作缓冲区, 而 Command-line
模式则会将保留部分当成一块用于输入命令的命令缓冲区来操作.

在 Basic 部分中, 所有要求的功能在终端上打印的字符都以默认颜色与属性呈现 (具体来说, 都使用默认前景色与背景色;
没有额外终端字符属性, 如粗体). 也就是说, 你不需要操作任何颜色与属性即可通过 Basic 评测.

`<C-q>` 在该项目中是未被绑定的保留键位. 你可以将其实现为紧急退出键, 按下后在任意模式下结束程序, 以便方便调试 Basic
及之后的部分. 我们不会对该键行为进行评测.

#### Normal 模式 (Basic)

在 Normal 模式中, 光标总会停留在某一个字符上 (对于空行, 则只能停留在不存在字符的位置) .

在 Basic 部分, 你需要为 Normal 模式实现以下的功能键.

##### `h` (Basic)

光标向左移动一个字符; 如果光标在行首则不变.

##### `l` (Basic)

光标向右移动一个字符; 如果光标在行末则不变.

##### `j` (Basic)

光标向下移动一个字符; 如果光标在最后一行则不变. 若光标移动到的位置在这一行行末之后, 则向左移动到行末位置.

##### `k` (basic)

光标向上移动一个字符; 如果光标在第一行则不变. 若光标移动到的位置在这一行行末之后, 则向左移动到行末位置.

##### `i` (Basic)

在光标所选字符之前进入 Insert 模式. 若光标处在空行, 则在该空行的唯一位置进入 Insert 模式.

##### `a` (Basic)

在光标所选字符之后进入 Insert 模式. 若光标处在空行, 则在该空行的唯一位置进入 Insert 模式.

##### `:` (Basic)

进入 Command-line 模式.

---

请注意对于空行, 该行的行末位置不存在实际字符; 而对于非空行, 该行的行末位置是最后一个字符的位置.

对于以上的移动的 curswant 功能, 不要求在 Basic 部分中准确实现, 但是在 Advanced 部分中会进行评测.
具体关于该功能的说明详见 [curswant](#curswant)

#### Insert 模式 (Basic)

在 Insert 模式中, 光标总会停留在某一个字符上, 或是停留在行末位置, 即行末字符之后的位置上 (你可以假想为那里存在一个换行符
`\n`; 事实上, Vim 就是这么实现的).

在 Basic 部分, 你需要为 Insert 模式实现以下的功能键:

##### `{printable ascii}` (Basic, Insert)

这部分功能键限定为所有 ASCII 码在 `0x20~0x7e` 之间的字符. 在光标所选字符之前插入输入的字符,
并将光标自然移动到下一个位置上.

##### `<BS>` (Basic, Insert)

删除光标所选字符之前的字符, 并将光标自然移动到前一个位置上. 若光标处在行首 (或空行), 则拼接该行与上一行,
并将光标自然移动到上一行原行末位置上. 若光标处在文件首, 则无效果.

##### `<CR>` (Basic, Insert)

在光标所选字符之前位置切分该行, 换行至下一行, 并将光标自然移动到下一行的行首.

##### `<Esc>` (Basic, Insert)

进入 Normal 模式, 并将光标自然移动到前一个位置上. 若光标处在行首则不移动光标.

---

#### Command-line 模式 (Basic)

在 Command-line 模式中, 光标总会停留在窗口底部的命令输入部分, 总体操作逻辑类似于 Insert 模式中的一行, 并且此时行首必定以
`:` 开头.

进入到 Command-line 模式时, 初始输入的指令, 即命令缓冲区部分为空. 除了屏幕最下面一行, 其余部分显示内容应保持不变.

在 Basic 部分中, 你只需要考虑光标处在行末位置的情形. 也就是说, 任何停留在 Command-line
模式的操作最终都会将光标移动到行末位置.

在 Basic 部分, 你需要为 Command-line 模式实现以下的功能键:

##### `{printable ascii}` (Basic, Command-line)

这部分功能键限定为所有 ASCII 码在 `0x20~0x7e` 之间的字符. 在光标所选字符之前 (在 Basic 部分中, 即行末) 插入输入的字符,
并将光标自然移动到下一个位置上 (在 Basic 部分中, 即行末).

##### `<CR>` (Basic, Command-line)

执行当前命令缓冲区中的指令, 待执行完毕后进入 Normal 模式.

##### `<Esc>` (Basic, Command-line)

进入 Normal 模式, 并将光标移动到进入 Command-line 模式前原来的位置. 舍弃在 Command-line 模式中的所有输入.

---

在 Basic 部分, 你需要实现以下命令:

##### `:wq [file]` (Basic, Command-line)

将缓冲区内容写入到该路径所在文件. 若文件不存在, 则创建新文件并写入. 若文件已存在, 则覆盖原有内容. 在 Basic
部分中不需要考虑文件权限等问题.

##### `:q!`, `:quit!` (Basic, Command-line)

舍弃缓冲区内容并退出 MiniVim.

---

若当前输入的指令非法, 在 Basic 部分中你可以直接忽略执行它.

### Advanced

Advanced 部分包含若干可选功能. 他们之间大部分相互独立, 但有些功能存在依赖关系, 这意味着部分功能的实现需要其他功能来支持.
例如, 一般来说, 你需要实现 `e` 才能实现 `ce`.

对于 Advanced 部分, 你可以选择任意一部分功能来实现. 每个功能在 ACMOJ 中都有独立测试点计分, ACMOJ
给出的总得分在折算之后作为 Advanced 部分的实际得分. 折算公式为

$$
y = f(x)
$$

其中 $x$ 为你的 ACMOJ 计分, $y$ 为你的实际得分.

如果你使用我们提供的框架, 从本部分开始, 除 Basic 中已经给出的公共接口与 `Terminal` 外, 不再限制 `src/`
中其他模块的接口设计. 你可以添加新的类, 数据结构, public / private 接口或源文件, 但已经完成的 Basic 行为不得被破坏.

除非特别说明, 本节中的命令语义按照 Vim 的行为定义. ACMOJ 的评测不会测试未提及的 Vim 行为与特性.

若下文明确给出与 Vim 默认行为不同的要求, 则以该要求为准.

有些功能的实现与大量操作相关联 (如 `[count]{operation}`, 许多操作都支持加上计数),
则该功能的评分会在所有所关联的操作下独立评分.

#### 目录

##### 动作

| 操作/功能                  | 得分 | 依赖         | 概述                                                                                    |
| -------------------------- | ---- | ------------ | --------------------------------------------------------------------------------------- |
| `h`, `l`, `j`, `k`         | 3    |              | 将光标向左/右/下/上移动一个字符                                                         |
| `0`, `^`, `$`              | 6    |              | 将光标移动到行首/第一个非空白字符/行末                                                  |
| `gg`, `G`                  | 2    |              | 将光标移动到文件首行/末行                                                               |
| `w`, `b`, `e`, `ge`        | 9    |              | 将光标移动到下一个单词首字母/上一个单词首字母/下一个单词尾字母/上一个单词尾字母         |
| `W`, `B`, `E`, `gE`        | 5    |              | 将光标移动到下一个 WORD 首字母/上一个 WORD 首字母/下一个 WORD 尾字母/上一个 WORD 尾字母 |
| `%`                        | 3    |              | 将光标跳转到与所在括号匹配的括号                                                        |
| `m{mark}`, `` `{mark} ``   | 4    |              | 在光标位置设置标签/跳转到标签                                                           |
| `f{char}`, `F{char}`       | 4    |              | 向下/上搜索匹配的字符                                                                   |
| `;`, `,`                   | 2    | `f{char}`    | 跳转到下一个/上一个匹配上次 `f{char}`, `t{char}` 搜索的字符                             |
| `/{reg}<CR>`, `?{reg}<CR>` | 4    |              | 向后/前搜索匹配正则表达式的结果                                                         |
| `n`, `N`                   | 2    | `/{reg}<CR>` | 跳转到下一个/上一个匹配上次 `/{reg}<CR>`, `?{reg}<CR>` 搜索的结果                       |

##### 模式切换

| 操作/功能 | 得分 | 依赖 | 概述                                            |
| --------- | ---- | ---- | ----------------------------------------------- |
| `i`, `a`  | 3    |      | 从当前位置/当前位置后一个字符进入 Insert 模式   |
| `I`, `A`  | 4    |      | 从当前行第一个非空白字符前/行末进入 Insert 模式 |
| `o`, `O`  | 3    |      | 向下/上插入一个新行, 并从新行进入 Insert 模式   |
| `v`       | 10   |      | 以当前位置为起点和终点进入 Visual 模式          |
| `R`       | 4    |      | 从当前位置进入 Replace 模式                     |

##### 滚动

| 操作/功能                          | 得分 | 依赖 | 概述                                                                                 |
| ---------------------------------- | ---- | ---- | ------------------------------------------------------------------------------------ |
| `<C-e>`, `<C-y>`, `zt`, `zb`, `zz` | 7    |      | 光标位置不变, 屏幕向上滚动一行/向下滚动一行/向上滚动到顶部/向下滚动到底部/滚动到居中 |

##### 操作

| 操作/功能                      | 得分 | 依赖     | 概述                                                        |
| ------------------------------ | ---- | -------- | ----------------------------------------------------------- |
| `x`, `X`                       | 3    |          | 删除当前字符/删除当前字符前一个字符, 并将删除内容存入剪切板 |
| `r<char>`                      | 3    |          | 替换当前字符                                                |
| `p`, `P`                       | 6    | `dd`     | 从当前位置向后/前粘贴剪切板内容                             |
| `"{register}p`, `"{register}P` | 6    | `p`, `P` | 从当前位置向后/前粘贴寄存器中剪切板内容                     |
| `~`                            | 2    |          | 翻转当前字符大小写                                          |
| `J`                            | 3    |          | 连接当前行与下一行                                          |

##### 范围操作

| 操作/功能                            | 得分 | 依赖 | 概述                                                                     |
| ------------------------------------ | ---- | ---- | ------------------------------------------------------------------------ |
| `d{motion}`, `dd`, `D`               | 6    |      | 删除范围内所有字符/当前行/到行末, 并将删除内容存入剪切板                 |
| `y{motion}`, `yy`, `Y`               | 6    |      | 复制范围内所有字符/当前行/到行末, 到剪切板                               |
| `c{motion}`, `cc`, `C`               | 6    |      | 删除范围内所有字符/当前行/到行末, 到剪切板, 并从删除位置进入 Insert 模式 |
| `<{motion}`, `>{motion}`, `<<`, `>>` | 6    |      | 将范围内文本向左/右缩进, 或将当前行向左/右缩进                           |

##### 非 Normal 模式操作

| 操作/功能                          | 得分 | 依赖 | 概述                                |
| ---------------------------------- | ---- | ---- | ----------------------------------- |
| `<BS>` (Insert), `<Del>` (Insert)  | 6    |      | 删除光标所选字符之前的字符/当前字符 |
| `<Esc>` (Insert), `<Esc>` (Visual) | 6    |      | 退出当前模式到 Normal 模式          |

##### 命令操作

| 操作/功能                  | 得分 | 依赖                       | 概述                         |
| -------------------------- | ---- | -------------------------- | ---------------------------- |
| `.`                        | 6    |                            | 重放上一次操作               |
| `q{register}[operations]q` | 6    | `.`, `p`                   | 将一定操作录制到寄存器       |
| `@{register}`              | 6    | `q{register}[operations]q` | 播放寄存器中的操作           |
| `Q`                        | 6    | `@{register}`              | 播放上次录制的寄存器中的操作 |
| `u`                        | 6    |                            | 撤回上一次操作               |
| `<C-r>`                    | 6    | `u`                        | 重做上一次撤回的操作         |

---

#### 独立评分功能概述

##### `[count]{operation}`

大多数 Normal 与 Visual 模式命令可以在命令前连接一个无前导零十进制 `[count]`. 这模糊地表示该命令的重复次数,
但是对于部分命令, 其实际含义存在特殊情况或并不表示重复次数, 这会在具体命令中详细说明.

连续数字共同构成一个 `[count]`. 若命令没有显式 `[count]`, 则认为 count 为 1.

命令操作中的头部 `0` 不属于 `[count]`, 而作为 `motion` 处理; 已经开始输入 `[count]` 后, `0` 则属于 `[count]`. 因此
`[count]` 一定不包含前导零.

count 可能会很大. 对于可能造成溢出的 count 值, 你可以设置一个最大 count 值进行截断. 对于部分指令,
你需要进行优化来保证正确高效处理足够大的 count.

##### `{operator}{motion}`

在一个范围操作命令 `{operator}` 后指定一个移动命令 `{motion}`, 表示以 `motion` 指示的移动位置为终点进行该操作.
对于不同操作, 该组合命令的含义是不同的.

对于大部分行内移动或小范围移动, `{motion}` 指示的区域是从当前字符到移动到的字符的区域; 对于部分跨行移动, `{motion}`
指示的区域是从当前行到移动到的位置所在行, 且操作以行为单位进行 (linewise). 具体含义会在具体操作处阐释.

`{operator}` 后的 `{motion}` 除了可以是 Normal 模式中正常的移动之外, 也可以是表示选择一个文本对象的特殊动作. 这些 motion
以 `i` 或 `a` 加一个文本移动操作构成. 具体含义会在具体文本操作处阐释. 在 Visual 模式中也可以使用这类特殊动作.

该组合命令可以结合计数: `[count]{operator}[count]{motion}`. 若两个 `[count]` 都被显式指定, 则最终计数值为它们的乘积.

##### `<Esc>`

在 Normal 模式下, `<Esc>` 会打断当前未完整键入的任何命令, 并清空当前键入命令. 若当前未残留不完整命令, 则无效果.

`<Esc>` 会打断并清空包括 `[count]`, `{operator}` 等所有命令组成部分.

注意对于所有多键的操作, 你需要同时完成 `<Esc>` 打断的正确行为才能够通过评测.

##### curswant

`curswant` 是 Vim 内部的一个列坐标变量, 用于记录光标在垂直移动 (如按 `j`/`k`) 时的**理想列位置**.
它确保了上下移动时光标能尽可能停留在同一视觉列.

例如, 假设你在第 1 行的第 20 列, 此时 `curswant` 被设为 `20`. 你按 `j` 下移, 但第 2 行只有 10 个字符, 光标实际只能停在第
10 列. 此时 `curswant` 的值依然是 `20`, 它记住了你想去第 20 列. 你再按 `j` 下移, 如果第 3 行够长, 光标会自动跳回第 20
列, 而不是留在第 10 列.

在 Vim 中, 通过 `:echo getcurpos()` 可以查看光标位置信息, 返回格式为: `[bufnum, lnum, col, off, curswant]`.

当你使用 `$` 等操作将光标移到行尾时, `curswant` 会被设为一个极大的值, 表示 "想去最右边". 只要不进行水平移动或编辑,
之后无论上下移动到多长的行, 光标都会直接停在行尾.

垂直移动不会改变 `curswant` 的值.

---

#### Word

- 3pt

MiniVim 固定使用以下 word 分类:

- `[A-Z, a-z, 0-9, _]` 属于一类;
- 其他非空白字符属于一类;
- 空白字符(space 和 tab)作为 word 分隔符.

连续属于同一类的非空白字符组成一个 word.

简要确认: 在你的 basic 实现下,buffer 里面应该只含有 0x20~0x7E 的字符以及 tab 字符

一个对于 word 划分的例子:

在这一行 " hello += world_2; " 里面依次包含四个 word, 它们是: "hello", "+=", "world_2", ";"

你还需要特别区分**空行**与**只包含空白字符的行**. 空行本身视为一个 word 而只包含空格和\t 的行不是空行, 其中不存在任何
word.

需要实现:

| Motion | 行为                                    |
| ------ | --------------------------------------- |
| `w`    | 向文件末尾方向移动到下一个 word 的开头. |
| `b`    | 向文件开头方向移动到最近的 word 开头.   |
| `e`    | 向文件末尾方向移动到最近的 word 结尾.   |

这些 motion 可以跨越行边界并支持 count.

对于 w: 若 cursor 当前位于一个普通 word 中, 无论位于该 word 的开头还是内部, 都首先越过当前 word 的剩余部分,
再越过后续空白字符, 停留在之后第一个 word 的开头. 若 cursor 当前位于空白字符上, 则跳过当前位置之后连续的空白字符,
停留在之后第一个 word 的开头. 空行本身视为一个 word, 因此 w 可以停留在空行的唯一合法位置.
只包含空白字符的行不能停留,因为不包含 word 若 cursor 后方不存在任何新的 word, 则 cursor
向文件末尾方向移动至能够到达的最远合法位置. 若 cursor 已经位于整个文件的最后一个合法位置, w 无效果

对于 b: 若 cursor 当前位于一个普通 word 内部但不位于其开头, 则移动到当前 word 的开头. 若 cursor 当前位于空白字符上,
则跳过当前位置之前的空白字符, 停留在之前最近的 word 开头. 空行本身视为一个 word, 因此 b 可以停留在空行的唯一合法位置.
只包含空白字符的行不能停留,因为不包含 word 若 cursor 前方不存在任何 word 开头, 则 cursor
向文件开头方向移动至能够到达的最远合法位置. 若 cursor 已经位于整个文件的第一个合法位置, b 无效果.

对于 e: 若 cursor 当前位于一个普通 word 内部且尚未位于该 word 的最后一个字符, 则移动到当前 word 的最后一个字符. 若
cursor 已经位于当前 word 的最后一个字符, 则继续向文件末尾方向寻找下一个普通 word, 并移动到其最后一个字符. 若 cursor
当前位于空白字符上, 则跳过之后的空白字符, 寻找之后最近的普通 word, 并停留在其最后一个字符. 与 w 和 b 不同, e
不会停留在空行. 空行以及只包含空白字符的行都会被跳过. 若 cursor 后方不存在可以到达的 word 结尾, 则 cursor
向文件末尾方向移动至能够到达的最远合法位置. 若 cursor 已经位于整个文件的最后合法位置, e 无效果.

对于 count,[count]w,b,e 的行为均等价于执行 count 次

#### Line

- 2pt

| Motion | 行为                         |
| ------ | ---------------------------- |
| `0`    | 移动到当前行第一个字符       |
| `^`    | 移动到当前行第一个非空白字符 |
| `$`    | 移动到当前行最后一个字符     |

对于 0: 若当前行是空行,光标不动

对于^: ^ 忽略 count. 若当前行为普通非空行, ^ 移动到第一个非空白字符. 若当前行只包含空白字符, ^ 移动到该行最后一个字符.
若当前行为真正的空行, cursor 保持在该行唯一合法位置.

对于$: 若当前行是空行,光标不动

`[count]$` 首先向下移动 `count - 1` 行, 再移动到目标行的行末. 若没有足够的后续行, 则分以下情况处理: 若 count 为 1,
正常移动到当前行行末; 若 count 大于 1 且当前行已经是文件最后一行, motion 失败并保持 cursor 不变;
否则移动到能够到达的最下方文本行的行末.

#### File

- 2pt

| Motion      | 行为                                      |
| ----------- | ----------------------------------------- |
| `gg`        | 移动到文件第一行的第一个非空白字符        |
| `[count]gg` | 移动到文件第 `count` 行的第一个非空白字符 |
| `G`         | 移动到文件最后一行的第一个非空白字符      |
| `[count]G`  | 移动到文件第 `count` 行的第一个非空白字符 |

若 count 大于文件总行数, 目标行为文件最后一行.

目标行的列规则: 目标行为非空行时, cursor 移动到该行第一个非空白字符. 目标行只包含空白字符时, 移动到行末
目标行为真正的空行时, cursor 位于该行唯一合法位置.

#### Window: `zt` / `zz` / `zb`

- 2pt, 替换原来的 `H M L` 项.

本项目不自动折行, 一条 Buffer 文本行占一条屏幕正文行. 设正文高度为 `H >= 1`, 文件行数为 `N >= 1`, 当前行号为 `r` (从 1
开始), viewport 顶行号为 `top`.

| Command | 无 count 时的行为  | 新的 top                   |
| ------- | ------------------ | -------------------------- |
| `zt`    | 当前行对齐正文顶部 | `r`                        |
| `zz`    | 当前行对齐正文中间 | `max(1, r - floor(H / 2))` |
| `zb`    | 当前行对齐正文底部 | `max(1, r - H + 1)`        |

偶数高度时, `zz` 使用靠下的中间行. 文件开头不足以提供上方文本时按上表限制到 1; 文件末尾允许出现 `~` 填充行,
不为填满窗口而把 top 往回移动. 无 count 时光标与 desired column 不变. 有显式 count 时先前往第 `min(count, N)` 行, 按原
desired column 选择目标字符, 再对齐; desired column 保持不变. 这三个命令不修改文本, register 或搜索记录, 不能用于
operator. 仅要求 `zt zz zb`, 不要求 `z<CR>` 等其他 z 命令.

#### Find: `f` / `F` / `t` / `T` / `;`

- 3pt, 包含 `;`.

**依赖: Count; `;` 依赖本节字符查找记录.**

语法为 `[count]f{char}` 等, `{char}` 可以是一个可打印 ASCII 字符或 Tab. 查找只在当前行内进行, 不包含光标当前字符,
不跨行.

| Command   | 查找方向              | 成功后的光标位置     |
| --------- | --------------------- | -------------------- |
| `f{char}` | 向右找第 count 个匹配 | 匹配字符上           |
| `F{char}` | 向左找第 count 个匹配 | 匹配字符上           |
| `t{char}` | 向右找第 count 个匹配 | 匹配字符左侧一个字符 |
| `T{char}` | 向左找第 count 个匹配 | 匹配字符右侧一个字符 |

不足 count 个匹配时整个 motion 失败, 不停在最后一个已找到的匹配上. 空行同样失败. `t/T` 找到相邻字符时可以成功但不移动;
成功的零位移与失败不同. 输入完整合法参数后, 保存查找种类与字符, 即使本次没有找到; 不保存 count. `<ESC>`
或非法参数取消命令, 不覆盖旧记录.

`[count];` 沿记录的方向, 使用记录的种类和字符再次查找第 count 个匹配. 无记录时失败. 重复 `t/T` 时,
排除会使本次光标停留原位的那个相邻匹配, 再计数; 例如在 `a,b,c` 的 `a` 上执行 `t,` 后, `;` 应移动到 `b`. `;` 不改变记录,
也不沿用最初查找的 count. 字符查找记录与 `/ ?` 的搜索记录互不影响.

#### Match: `%`

- 2pt.

无 count 的 `%` 在当前行从光标处开始向右寻找第一个 `(`, `)`, `[`, `]`, `{` 或 `}` (包含当前位置), 再跳到其配对括号.
若候选为左括号, 向文件末尾查找; 若为右括号, 向文件开头查找. 可以跨行, 用同种括号的嵌套深度确定配对;
其他种类的括号不影响深度. 引号, 反斜杠, 注释中的括号仍按普通括号处理. 当前行没有候选或候选没有配对时失败, 保持原位置,
不继续尝试下一个候选.

有显式 count 时, `[count]%` 表示跳到第 `ceil(count * N / 100)` 行, 其中 N 是文件行数. count 必须在 1 到 100 之间,
否则失败; 运算不得溢出. 列位置遵循 `G` 的第一个非空白字符规则. 这是百分比跳转, 不是重复括号匹配.

#### Mark: `m{a-z}` / 精确跳转

- 3pt, 包含设置, 跳转与编辑后的维护.

`m{a-z}` 将当前 Buffer 位置保存为对应小写局部标记; 同名标记被覆盖. 反引号后输入同一个字母, 例如 `` `a ``,
跳到该标记的精确行与字符位置. 这两种命令忽略 count. 不要求大写跨文件标记, 单引号跳转, 特殊标记或 jumplist.
未设置或已失效的标记使跳转失败; 非法标记名取消命令.

标记维护采用以下课程规则, 不要求复现 Vim 全部 mark 调整机制:

- 将 Buffer 视为各行以一个 `\n` 连接的字节序列, 不额外附加文件末尾换行. 标记锚定一个字节位置, 真正空行锚定其第 0
  列对应的位置 (可能是换行或 EOF).
- 在标记位置之前或恰好该位置插入文本, 标记随原文本向后移动. 插入换行时也按此规则换算新的行列.
- 删除半开区间 `[start, end)` 时, 区间内的标记失效; 位于 end 及之后的标记向前移动被删除的字节数. EOF
  的空行锚点不会仅因删除到 EOF 而落入这个半开区间.
- `~` 的原位字符替换保留标记. 缩进只把原行首空白的替换视为删除后插入; `J` 只删除实际去掉的换行和前导空白,
  插入实际添加的分隔空格. 其他保留文本上的标记按前两条平移.
- 整行删除也删除相邻的行分隔符: 有后续行时删除该行及其后的换行; 删除到文件末尾且有前置行时删除前置分隔符及末尾行块.
  全部删除后保留一个空行. 整行删除范围内的标记统一失效, 包括位于被删除末尾空行的 EOF 锚点.
- `cc` 等 Linewise Change 按删除所选完整行块, 再插入占位空行处理; 所选行上的标记统一失效. 后续 Insert 再按插入规则处理.

一次命令内按文本编辑的先后顺序维护标记, 不因最终文本恢复原样而恢复已失效标记. 每条 Normal 编辑命令结束后, Insert
模式每次编辑按键完成后, 将存活锚点换算为行列; 若落到非空行的行末之后, 限制到该行最后一个字符, 并以此作为后续锚点;
在空行则仍为第 0 列. 标记不写入磁盘; 保存文件不清除标记. 标记维护的组合测试只使用本项声明支持的编辑功能; 基础测试用
Basic Insert, 与 A2/A3/A4 的组合需同时实现对应项.

#### Search: `/` / `?` / `n` / `N`

- 4pt, 包含搜索输入与重复搜索.

**依赖: Basic Command-line 输入, Count.**

`/` 打开向前搜索输入框, `?` 打开向后搜索输入框. 命令提示符本身不属于搜索串. 只要求区分大小写的字面串匹配,
不解释正则表达式, 转义, offset 或 option. 匹配不能跨行, Tab 按一个实际 Tab 字节匹配. `/`, `?`
和反斜杠在输入框内均作为普通搜索字符, 只有 `<CR>` 提交. 支持可打印 ASCII, Tab 和 `<BS>`; `<BS>` 删除末尾一个字节,
不删除提示符. 其他控制键忽略, `<ESC>` 除外. 输入期间不进行增量搜索, 正文与原光标位置不变. `<ESC>`
取消并回到进入前的位置, 不修改搜索记录, 同时取消尚未完成的 operator 和 count.

提交非空串后立即保存搜索串与 `/` 或 `?` 指定的方向, 即使本次找不到. 提交空串时复用旧串, 并使用这次输入的方向;
没有旧串则失败. `n` 按保存方向重复, `N` 按相反方向重复; 尚无搜索记录时两者都失败且不移动. `n/N` 不改变保存的方向;
连续两个 `N` 仍然朝同一个反方向搜索.

搜索按匹配的起始位置排序, 允许重叠匹配. 向前从当前字符之后开始, 向后从当前字符之前开始; 到文件边界后环绕.
每寻找下一个匹配最多完整绕行一周, 原位置的匹配排在一周的最后. 因此只有当前位置一个匹配时可以成功回到原位置;
整个文件无匹配时失败且不移动. `[count]/...<CR>`, `[count]?...<CR>`, `[count]n` 与 `[count]N` 都选择该方向上第 count
个匹配, 可以多次环绕; 不应为超大 count 实际循环数十亿次. 成功时光标位于匹配的第一个字符; 失败时光标与 viewport 保持不变.
不要求高亮匹配或搜索历史列表.

#### Scroll: `<C-e>` / `<C-y>` / `<C-d>` / `<C-u>`

- 2pt, 四个按键作为一个计分项.

**依赖: Count 与 Basic viewport, 不依赖 Window 的 z 命令.**

沿用 Window 的 `H, N, top`, 正文不自动折行. `top` 范围固定为 `[1, N]`, 允许末尾出现填充行. 以下是课程固定规则, 不依赖
Vim 的 scroll / scrolloff / startofline 设置.

- `[count]<C-e>` 将 top 增加 count, 限制到 N; 文本在屏幕上向上移动.
- `[count]<C-y>` 将 top 减少 count, 限制到 1; 文本在屏幕上向下移动.
- 上述两项优先保持光标 Buffer 位置. 若光标被挤出新的可见文本行区间 `[top, min(N, top + H - 1)]`, 则移到该区间最近的一行,
  按原 desired column 选择字符.
- `<C-d>` / `<C-u>` 默认步长为 `max(1, floor(H / 2))`; 显式 count 直接作为本次步长, 不修改后续命令的默认步长, 也不表示
  count 个半屏.
- 半屏滚动先将光标行按方向移动步长并限制到文件边界, 再将 top 按光标实际移动的行数同向移动并限制到 `[1, N]`.
  最后若光标仍不可见, 只将 top 调整到刚好能看见光标的位置. 列按原 desired column 选择.

四个命令都保留 desired column; 目标行过短时停在行末, 空行停在唯一合法位置. 不修改 Buffer, register, 查找记录或 Repeat
记录, 不能用于 operator.

---

#### A2. Normal / Insert 模式扩展

#### Insert Entry

- 2pt

| Command | 行为                                               |
| ------- | -------------------------------------------------- |
| `I`     | 在当前行第一个非空白字符处进入 Insert 模式         |
| `A`     | 在当前行行末进入 Insert 模式                       |
| `o`     | 在当前行下方插入一个空行, 并在新行进入 Insert 模式 |
| `O`     | 在当前行上方插入一个空行, 并在新行进入 Insert 模式 |

对于 I: 若当前行存在非空白字符, 在第一个非空白字符之前进入 Insert 模式. 若当前行只包含空格或 \t, 在第 0 列开始插入.
这是本项目固定采用的规则; Vim 在这里的行为可能随设置不同, 不作为覆盖本规则的依据. 若当前行为真正的空行,
在该行唯一合法位置进入 Insert 模式.

对于 A: 无论当前 cursor 位于该行何处, 都移动到当前行行末位置并进入 Insert 模式. 若当前行为真正的空行,
在该行唯一合法位置进入 Insert 模式.

对于 o: 始终在当前行下方创建一个新的空行, cursor 移动到新行唯一合法位置并进入 Insert 模式.
当前行为文件最后一行时仍然可以使用 o, 此时新行成为文件新的最后一行.

对于 O: 始终在当前行上方创建一个新的空行, cursor 移动到新行唯一合法位置并进入 Insert 模式.
当前行为文件第一行时仍然可以使用 O, 此时新行成为文件新的第一行.

Note: vim 对于这 4 个命令是支持 count 的,但我们不要求支持

#### Insert `<Delete>`

- 2pt

删除 Insert cursor 当前所指的一个字符.

若 cursor 当前指向普通字符(包含 space)或 \t, 删除该字符后 cursor 保持在原来的 Buffer 位置. 若后方还有字符, cursor
因此指向原字符的下一个字符; 若删除的是当前行最后一个字符, cursor 停留在新的行末位置. 若 cursor 已经位于当前行行末,
且存在下一行, 则删除当前行与下一行之间的换行, 将下一行拼接到当前行之后.此时 cursor 保持在原来的行末位置,
即拼接后下一行原来的第一个字符之前. 若当前行为一个空行且存在下一行, <Delete> 同样删除两行之间的换行并将下一行拼接上来.
若 cursor 位于整个文件的最后一个合法位置, <Delete> 无效果.

#### `x`

- `x X D C` 合计 4pt; 带 count 的行为依赖 A1 Count.

x 删除 Normal 模式 cursor 当前所指字符, 删除后仍处于 Normal 模式. [count]x 删除当前行中从 cursor 开始最多 count 个字符,
不跨越换行. 若 count 大于 cursor 到当前行行末的字符数, 只删除到当前行行末. 若当前行为真正的空行, x 无效果.

删除完成后: 若删除范围到达当前行行末, cursor 移动到删除后当前行的最后一个字符; 若当前行被删除为空行, cursor
停留在该行唯一合法位置.

#### `X`

X 删除 Normal 模式 cursor 之前的一个字符, 删除后仍处于 Normal 模式. [count]X 删除当前行中 cursor 左侧最多 count 个字符,
不跨越换行. 若 count 大于 cursor 左侧的字符数, 删除当前行中 cursor 之前的所有字符. 若 cursor 已经位于当前行第一个字符,
或当前行为真正的空行, X 无效果.

删除完成后 cursor 向左移动实际被删除的字符数, 并停留在原 cursor 所指字符上.

#### `D`

D 删除从 cursor 当前字符到当前行行末的文本, 包含 cursor 当前字符. 删除完成后仍处于 Normal 模式. 若 cursor 左侧仍有字符,
cursor 移动到删除区域左侧的最后一个字符. 若删除从当前行第一个字符开始, 删除后该行为空, cursor 停留在该行唯一合法位置.
若当前行为真正的空行, 无 count 的 D 无效果.

[count]D 表示从 cursor 当前字符开始, 一直删除到向下第 count - 1 行的行末. 若 count 大于从当前行到文件末尾的剩余行数,
删除范围一直延伸到文件末尾.

删除完成后的 cursor 规则与无 count 的 D 相同

#### `C`

删除行为和 D 完全相同, 随后从光标位置进入 Insert 模式.

若当前行为真正的空行且有效 count 为 1, C 不删除任何字符, 直接在该行唯一合法位置进入 Insert 模式. count 大于 1 时仍按 D
的跨行范围删除, 随后从删除范围起点进入 Insert.

#### Toggle Case: `~`

- 2pt.

`[count]~` 从当前字符开始, 翻转当前行中最多 count 个字符的 ASCII 大小写: `A-Z` 与 `a-z` 互换,
其他字符保持不变但同样消耗一次计数. 不跨越换行. 光标移动到处理范围之后的字符; 若已处理到行末, 停在本行最后一个字符.
空行无效果. 例如 `ab1D` 上从 `a` 执行 `3~`, 文本成为 `AB1D`, 光标位于 `D`. 该命令不是 operator, 不支持 `~{motion}`; 不写
register. 全部处理字符都不是字母时可以移动光标, 但不算文本修改.

#### Join: `J`

- 2pt.

`J` 与 `1J` 均合并当前行和下一行; `[count]J` 合并从当前行开始的 `max(2, count)` 行. 行数不足时只合并实际存在的行;
当前行已是末行则无效果. 依次把下一行接到当前累计结果后面, 每一步固定采用以下规则:

1. 删除两行间的换行, 并去掉下一行开头的全部 space / Tab.
2. 若去掉前导空白后下一行为空, 不添加分隔空格.
3. 否则, 若累计结果非空, 末尾不是 space / Tab, 且下一行第一个字符不是 `)`, 添加一个 space.
4. 拼接下一行的剩余内容. 保留累计结果原有的尾部空白, 不因句号, 问号或感叹号添加第二个空格.

完成后光标定位在最后一次拼接之前累计结果的字节长度处, 即该次新增分隔空格或新接入内容的开头;
若此处超出结果的合法字符范围, 限制到行末. 结果为空则停在唯一合法位置. 例如 `ab` 和 `cd` 合并为 `ab cd`, 光标在中间的
space 上. `J` 不写 register.

---

#### A3. Operator + Motion

**依赖: A1 Count 与对应的 motion; `c` 还依赖 Basic Insert. 扩展 motion 和 Shift 是独立可选项.**

本部分要求实现 Vim Normal 模式中最核心的 Operator-Pending 机制.

需要支持三个 operator:

| Operator | 行为   |
| -------- | ------ |
| `d`      | Delete |
| `y`      | Yank   |
| `c`      | Change |

operator 本身不会立即产生文本修改. 输入 operator 后, MiniVim 进入 Operator-Pending 状态, 等待后续 motion 确定操作范围.

#### Command Grammar

- 1pt

需要支持该语法:

```text
[count1] operator [count2] motion

其中
operator = d | y | c (实现 Shift 时另含 < | >)
motion count = count1 * count2
```

count 相乘时同样保留低 10 位

先分别按 A1 规则读取两个 count (未输入的原值为 1), 再取数学乘积的末十位; 结果为 0 时有效 count 为 1,
中间整数溢出不能改变结果. 例如 `5000000001d2l` 的有效 motion count 为 2, `9999999999d9999999999l` 的有效 motion count
为 1. 这是课程规则, 验证时不能直接依赖 Vim 的超大 count 行为.

Operator-Pending 状态下按下 `<ESC>` 应取消当前 operator 和所有 count.

若 operator 后输入不支持的 motion, 应取消当前 pending command, 不做修改.

#### Supported Operator Motions

- 每个 1pt,共 12pt

我们希望支持以下 motion 参与 operator:

h j k l w b e 0 ^ $ gg G

以上 12 种为基础 operator motion. 新增 motion 的组合属于后文 Extended Operator Motions, 单独计分; Window / Scroll
和设置标记的 `m` 始终不能用于 operator.

在 vim 中,motion 可以被分成 3 类,定义如下: **Characterwise Exclusive**: 将 motion 的起始位置与目标位置按文本顺序排列后,
操作左端位置到右端位置之前的字符, 即不包含文本顺序中较后的端点. **Characterwise Inclusive**: 将 motion
的起始位置与目标位置按文本顺序排列后, 操作两个端点以及它们之间的所有字符. **Linewise**: 操作 motion
起始位置与目标位置所在行之间的**所有**完整文本行, 包含起始行与目标行.

例如:

```
某行文本是 abcd
光标在 c,使用 dh 命令

起始位置是 c,目标位置是 b
按照文本顺序排列后[start, end] = [b, c]
由于 h 是 exclusive 的,c 是不会被包含在内的,真实区间就是[b]
我们对区间内目标执行删除.

删除后文本是 acd,光标依然在 c
```

#### Motion Type

不同 motion 与 operator 组合时产生的 range 类型如下:

| Motion | Range Type              |
| ------ | ----------------------- |
| `h`    | Characterwise Exclusive |
| `l`    | Characterwise Exclusive |
| `w`    | Characterwise Exclusive |
| `b`    | Characterwise Exclusive |
| `e`    | Characterwise Inclusive |
| `0`    | Characterwise Exclusive |
| `^`    | Characterwise Exclusive |
| `$`    | Characterwise Inclusive |
| `j`    | Linewise                |
| `k`    | Linewise                |
| `gg`   | Linewise                |
| `G`    | Linewise                |

对于 Characterwise Exclusive, Characterwise Inclusive 与 Linewise range, 除特殊说明均按照前文定义计算最终操作范围.

**Note: motion 在 Operator-Pending 状态下只用于确定操作范围. 你不应该将 motion 后的目标位置当成 operator
执行完成后的最终 cursor 位置.**

**Note: 若 motion 的 count 超过能够移动的范围, 则按照 A1 中该 motion 的边界行为确定最终范围.**

对于 `h`, `l` : 若 cursor 位于行首时, `h` 无法产生非空范围, 因此无效果 cursor 位于行尾时,`l`对最后一个字符有效;
`[count]h` 与 `[count]l` 不跨越行, count 过大时范围分别限制到当前行行首或行末.

对于 `j`, `k`: 若 count 过大, range 延伸到文件第一行或最后一行; 若 `j` 已经在文件最后一行, 或 `k` 已经在文件第一行,
则对应 operator 无效果.

对于 `gg`, `G`: `gg` 与 `G` 即使目标行就是当前行, 仍然产生包含当前整行的 Linewise range.

对于 w: `w` 与 operator 组合时存在一个特殊的行尾行为.

若最后一次 `w` 所经过的最后一个 word 位于某一行的行末, 则操作范围结束在该 word 的最后一个字符,
不继续包含换行以及下一行第一个 word.

例如:

```text
hello
world
```

cursor 位于 `hello` 的 `h` 时执行 `dw`则只删除 `hello`, 不删除两行之间的换行.

除这一规则与下面 `cw` 的特殊规则外, MiniVim 不要求实现 Vim 对跨行 Exclusive motion 的其他自动 Inclusive / Linewise 转换.

#### Extended Operator Motions

- 4pt: Find (含 `;`), Match, Mark Jump, Search (含 `n N`) 各 1pt.

**依赖: A3 Command Grammar / Range 与 A1 中对应的功能. 各项不互相依赖.**

| Motion                              | Range Type                   |
| ----------------------------------- | ---------------------------- |
| `f{char}` / `t{char}`               | Characterwise Inclusive      |
| `F{char}` / `T{char}`               | Characterwise Exclusive      |
| `;`                                 | 沿用所记录查找种类的上述类型 |
| 无显式 count 的 `%`                 | Characterwise Inclusive      |
| 有显式 count 的 `%`                 | Linewise                     |
| 标记精确跳转, 如 `` `a ``           | Characterwise Exclusive      |
| `/...<CR>` / `?...<CR>` / `n` / `N` | Characterwise Exclusive      |

以上类型均按本项目 "先按文本顺序排列端点" 的定义计算, 不额外套用 Vim 的跨行 Exclusive 转换规则. 无 count 的 `d%`
以执行前的光标为起点, 配对括号为终点; 向右寻找候选括号的过程不改变起点. `d50%` 与 `50d%` 均使用显式 count 50
的百分比跳转; operator 两侧任意一侧有 count 就视为显式输入, 有效值仍按相乘规则计算. 标记跳转忽略有效 count.
其他查找和搜索使用有效 count, `;` 不再乘上查找记录中的旧 count.

若 motion 失败, 整条 operator 命令不修改文本, 不移动光标, `c` 也不进入 Insert, register 不变.
完整查找或搜索对记录的更新仍遵循 A1. 若 motion 成功但位移为零, 按类型计算范围: Inclusive 包含当前字符 (空行则无字符),
Exclusive 为空, Linewise 包含当前整行. 因此相邻目标上的 `dt{char}` 可以删除当前字符; 跳回当前位置的搜索产生空 Exclusive
range, `c` 按前文空范围规则进入 Insert.

搜索发生环绕时, operator 只取原位置与最终目标之间按文本顺序排列的单个区间, 不取环绕路径, 不重复处理经过多次的文本.
这是课程规则. 例如从第二行搜索并环绕到第一行时, 最终操作区间仍是第一行目标到第二行原位置之间的 Exclusive 区间.

#### Doubled Operator

- 1pt

相同 operator 连续输入两次表示对完整文本行进行操作:

| Command | 行为          |
| ------- | ------------- |
| `dd`    | 删除当前行    |
| `yy`    | yank 当前行   |
| `cc`    | change 当前行 |

Doubled Operator 为 Linewise 操作.

它们同样支持:

```text
[count1] operator [count2] operator
```

#### Delete

d{motion}删除 motion 所确定的 range, 删除完成后保持 Normal 模式.

若为 Characterwise range, 删除完成后 cursor 位于被删除范围的起始位置. 若该位置在删除后仍存在字符, cursor 停留在该字符上.
若删除范围一直延伸到当前行行末, 使 range 起始位置后不存在字符, 则 cursor 移动到该行最后一个字符. 若该行删除后成为空行,
cursor 停留在该行唯一合法位置.

若为 Linewise range, 删除范围内的完整文本行. 删除后若原删除范围下方仍有文本行, cursor 移动到该行第一个非空白字符.
若删除范围一直到文件末尾, cursor 移动到新的最后一行的第一个非空白字符. 若文件中的所有行均被删除, Buffer
中仍然保留一个空行(快速检查:你在合适的地方调用 EnsureNonEmpty 了吗?), cursor 位于该行唯一合法位置.

如果同时实现 A4, 被删除的文本写入 unnamed register, 并保留本次删除的 Characterwise / Linewise 类型.

#### Yank

y{motion}将 motion 所确定的 range 按照类型复制到 unnamed register.

**Note: 你如果不实现 A4, 这里只要保持 cursor 位置正确即可**

完成 yank 后, cursor 位于操作范围在文本顺序中的起始位置.

#### Change

c{motion}首先删除 motion 所确定的 range, 随后进入 Insert 模式.

对于 Characterwise range, 删除后直接在 range 起始位置进入 Insert 模式. 若 range 为空, 不删除文本, 直接在当前位置进入
Insert 模式. 对于 Linewise range, 删除范围内的完整文本行, 在原范围位置保留一个空行, 并在该空行唯一合法位置进入 Insert
模式.

空 Characterwise range 进入 Insert 是课程规定. 例如文件开头的 `cb` 应进入 Insert, 即使默认 Vim 对该操作不进入 Insert.
这不改变前文明确规定的 motion 失败行为: 文件末行的 `cj`, 文件首行的 `ck`, 以及文件末行失败的 `c2$` 均无效果, 不进入
Insert.

#### `cw` Special Case

`cw` 是 `c{motion}` 的特殊情况.

若执行 `cw` 时 cursor 位于非空白字符上, change 范围结束在当前 word 的末尾, 而不是普通 `w` motion 所指向的下一个 word
开头.

带 count 时, 最后一个被 change 的 word 后面的空白同样不属于操作范围.

若 cursor 当前位于空白字符上, `cw` 不使用上述特殊行为, 而按照普通 `w` motion 产生的 range 执行 Change.

如果同时实现 A4, Change 删除的原文本同样写入 unnamed register, 并保留 Characterwise / Linewise 类型.

#### Shift: `<` / `>` / `<<` / `>>`

- 3pt.

**依赖: A1 Count, A3 Command Grammar / Range 与实际使用的 motion.**

`>{motion}` 与 `<{motion}` 分别将 motion 对应的文本行向右或向左缩进一级. `>>` 与 `<<` 处理当前行; `[count1]>[count2]>`
等 doubled 形式向下处理有效 count 行, 超过末行时截断. operator 两侧 count 的乘积决定 motion 的范围或 doubled 形式的行数,
**不决定缩进级数**. 例如 `3>>` 将三行各缩进一级.

先按对应 motion 的规则得到 range, 再处理该 range 中实际包含字符或换行的所有文本行. Exclusive range 若终点恰为较后行的第
0 列, 不包含该终点行; Linewise range 包含其中所有行, 包括空行. motion 失败或 Characterwise range 为空则无效果. 支持的
motion 与本次实现的 `d/y/c` 相同; 新增 motion 组合仍依赖对应扩展项.

固定每级 4 个显示列. 对每个受影响的非空行, 计算原行首连续 space / Tab 的显示宽度 W (Tab 沿用 Basic 的 4 列制表位):

- 右移后的前导空白统一替换成 `W + 4` 个 space.
- 左移后的前导空白统一替换成 `max(0, W - 4)` 个 space.
- 真正空行保持为空; 只有空白的非空行也按上述宽度规则处理.
- 非前导空白和正文完全不变. 只剩 1 到 3 列缩进时, 左移会全部移除.

例如行首一个 Tab 的 `>...` 结果为 8 个 space, `<...` 结果为 0 个 space. 不要求保留原 Tab 字节.
成功得到至少包含一行的范围后, 光标移到第一条受影响行, 列位置遵循 `^` 的规则, 即使该次没有改变文本. 不写 register.

---

#### A4. Register 与 Yank / Put

- Unnamed Register / Yank / Put 整体 8pt; Register Prefix 另计 4pt.

**依赖: A2 的 `x X D C`, A3 基础 operator 与 doubled operator. 扩展 motion 不属于本项基础测试依赖.**

基础部分要求实现 Vim 的 **unnamed register**. 命名寄存器与显式寄存器选择属于后文 Register Prefix, 单独计分.

Register 需要同时保存文本和类型

#### Register Write

没有显式 register 前缀时, 以下操作应覆盖 unnamed register; 有前缀时按 Register Prefix 的规则选择写入目标:

```text
d{motion}
dd
c{motion}
cc
y{motion}
yy
x
X
D
C
其中 x X D C 均作为 Characterwise.
```

在 A2 中即使 [count]D 或 [count]C 的删除范围跨越多行, register type 仍然为 Characterwise.若 Characterwise range
跨越行边界, register 中也需要保存这些行边界.

若一次操作没有产生任何有效的文本范围, 则 unnamed register 保持原内容不变.

此规则同样适用于空范围 yank 和 change. 例如行首的 `yh` 应保留原 register 的文本与类型, 不采用本次参考 Vim 中清空 unnamed
register 的行为. Linewise yank 一个真实存在的空行仍有有效范围, 会写入包含该空行的 Linewise register; 它与空
Characterwise range 不同.

新的 Register Write 会完全覆盖之前 unnamed register 中保存的文本和类型.

#### p / P

p 和 P 将 unnamed register 中保存的内容重新插入 Buffer.(不改变 register 内容)

`p` 在 cursor 之后 put

`P` 在 cursor 之前 put

具体 put 行为取决于 register type.

若 unnamed register 为空, p 和 P 均无效果.

#### Characterwise Put

对于 Characterwise register:

若当前行为真正的空行, p 与 P 都从该行唯一合法位置开始插入.

插入时遇到换行时需要切分当前文本行, 并按照 register 中保存的行边界恢复对应的多行文本.

put 完成后, cursor 停留在最后一个被插入的字符上. 若最后插入的位置为一个空行, 没有实际字符可以停留, 则 cursor
位于该空行唯一合法位置.

上述光标规则也适用于跨行 Characterwise register 及带 count 的 put, 是本项目统一采用的行为. Vim 跨行 `p` / `P`
的默认光标位置可能不同, 本项目仍以这里规定的插入终点为准.

#### Linewise Put

当 unnamed register type 为 Linewise 时, put 的位置按照完整文本行确定.

对于 p: 将 register 中保存的所有完整文本行插入当前行之后.

对于 P: 将 register 中保存的所有完整文本行插入当前行之前.

put 完成后, cursor 位于新插入文本的第一行. 若该行存在非空白字符, cursor 位于第一个非空白字符. 若该行只包含空白字符,
cursor 行为与前文 ^ 的规则相同. 若该行为真正的空行, cursor 位于该行唯一合法位置.

#### Put Count

p 与 P 支持 count:

[count]p [count]P

对于 Characterwise register, 将 register 中保存的完整文本内容连续插入 count 次.

对于 Linewise register, 将 register 中保存的完整文本行块连续插入 count 次.

count 只重复 register 中保存的内容, 不会在每次重复之间重新计算新的 put 位置.

整个 [count]p 或 [count]P 视为一次完整的编辑操作.

Characterwise Put 完成后, cursor 仍然停留在最后一个被插入的字符上.

Linewise Put 完成后, cursor 仍然位于所有新插入文本中的第一行, 列位置按照前述 Linewise Put 规则确定.

#### Register Prefix: `"{register}`

- 4pt.

**依赖: 本节 Unnamed Register / Yank / Put.**

`"` 在 Normal 模式下开始寄存器前缀, 后面紧跟一个寄存器名. 支持 `a-z`, `"` (unnamed) 与 `_` (black hole), 不要求大写追加,
数字, 系统剪贴板或表达式寄存器. 例如 `"ayy` 把当前行复制到 a, `"ap` 从 a put, `"_dd` 删除当前行但不保存删除内容.

要求支持以下语法, register 前缀必须位于整条命令的 count 之前:

```text
["{register}] [count1] operator [count2] motion
["{register}] [count1] operator [count2] operator
["{register}] [count] x | X | D | C | p | P
```

最后一行的可选前缀与 count 均作用于所选择的一条命令. 此处 operator 仅为 `d y c`, 不包含 Shift.
其他前缀顺序不在要求范围内; 在 count 或 operator 后遇到 `"` 视为非法后续按键并取消整条命令. `"` 后的非法名称, 重复的
register 前缀, 前缀后不支持的命令或 `<ESC>` 均取消本条命令, 清空所有待处理前缀, 不改变文本或 register.

各命名 register 独立保存文本和 Characterwise / Linewise 类型, 初始为空. 每次完整覆盖, 不追加. 没有显式前缀或使用 `""`
时, 写入与读取均使用 unnamed register. 向 `a-z` 写入时同时以同样文本和类型覆盖 unnamed register. 这两个 register
保存独立副本, 以后覆盖 unnamed 不改变命名 register. 向 `_` 写入时丢弃文本, unnamed 与所有命名 register 均不变; 从 `_`
put 永远无效果. 空 Characterwise range 不覆盖任何 register; Linewise 空行仍是有效内容, 与前文规则一致.

`p/P` 从指定 register 读取, 插入范围, count 与光标规则和 unnamed put 完全相同; 不改变任何 register. 读取尚未写入的
register 无效果. 前缀只作用于当前一条命令. Change 只把删除阶段的旧文本写入目标 register, 后续 Insert 不写入该 register.
`~`, `J`, Shift 与普通 Insert 编辑不写 register.

---

#### A5. Repeat: `.`

- 该部分整体 8pt.

**依赖: Basic Insert, A1 Count, A2, A3 基础 operator 与 A4 Unnamed Register / Put.**

本部分要求记录最近一次可重复的完整文本修改. 不要求撤销历史, `.` 也不是恢复某份旧 Buffer 快照:
它应从当前位置重新执行上一次编辑的意图.

#### Repeat Unit

下列完整命令可以成为一条 Repeat 记录:

- Basic 的 `i/a` 及 A2 的 `I/A/o/O` 所开始的完整 Insert session.
- A2 的 `x X D C ~ J`, A3 的 `d/c` 与 doubled operator, A4 的 `p/P`.
- 若同时实现 A3 Shift, 扩展 operator motion 或 A4 Register Prefix, 对应编辑同样可以重复.

从进入 Insert 到对应 `<ESC>` 返回 Normal, 中间普通字符, Tab, `<CR>`, `<BS>` 与 `<Delete>` 都属于同一 session.
记录进入方式和后续输入动作; 重复时在新位置重放这些动作, 不能只记录最后新增的字符串. 通过 `c` 或 `C` 进入 Insert 时,
删除范围的命令与整个 Insert session 合为一条记录. 单独 `o/O` 即使随后立即 `<ESC>`, 只要新增了空行仍是文本修改. 普通
Insert 与 Insert Entry 本身仍不要求 count; 对这些记录执行 `[count].` 时忽略 count, 只重放一次 session.

只有命令完成后完整 Buffer 内容与命令开始前不同, 才替换 Repeat 记录. Change 的比较起点在删除之前. 因此失败命令, 空范围
delete, 纯光标移动, 处理非字母的 `~`, 以及输入后全部删回的净零 Insert session 都不覆盖旧记录. 净零 session 中的标记,
查找或 register 副作用按各自规则保留, 不回滚. 普通 motion, 搜索, `m`, yank, 窗口命令和保存都不覆盖 Repeat 记录.
文件刚打开时记录为空.

#### Repeat Count

`.` 在当前位置执行记录, 默认使用记录中上一次成功修改的有效 count. 没有记录时无效果. `[count].` 用新的有效 count
**替换** 原 count, 不表示把整条旧命令运行 count 次. 对于 operator, 替换的是两侧 count 相乘后的有效值; 原来是否显式 count
的信息在无新 count 时保留, 有新 count 时置为显式.

例如 `2d3w` 记录有效 count 6; 之后 `.` 删除 6 个 word, `4.` 删除 4 个 word, 不是 24 个. 对于 `c` / `C`, 新 count
只控制删除范围, 随后的 Insert session 重放一次. 对于 `J`, 新 count 仍表示合并的总行数, 最少两行; 对于 `p/P`,
表示内容重复插入的次数. 对于 Shift, 新 count 控制 motion 或 doubled 形式的行数, 不控制缩进级数. 忽略 count 的 motion
(例如标记跳转) 重复时仍忽略 count.

重复产生实际文本修改后, 记录保留本次采用的有效 count, 因而 `4.` 成功后下一次 `.` 继续采用 4. 若重复失败或净修改为零,
原记录 (含旧 count) 保持不变. 重复仍是一条编辑命令, 不能把 `.` 本身记录成递归调用 `.` 的记录.

#### 重放时的状态

- 普通 motion 在新位置重新计算范围; 不记住旧绝对范围或旧删除文本. 重放 Insert session 时 `<BS>` / `<Delete>`
  也按新位置的边界规则执行.
- `p/P` 读取执行 `.` 当时指定 register 的内容, 不冻结原 put 的文本. 无前缀时读取当前 unnamed; 有前缀时记住 register
  名称.
- 带 register 前缀的 delete / change 重复时仍写同一目标, 默认命令仍写 unnamed. 不支持直接给 `.` 再加 register 前缀.
- 对 `f/F/t/T` 或 `;` 构成的编辑, 记录当时实际使用的查找种类和字符, 以及是否来自 `;` (以保留 `t/T`
  重复时排除相邻匹配的规则). 对 `/ ? n N` 构成的编辑, 记录当时实际使用的字面串和方向.
  之后修改查找或搜索记录不影响这条编辑的重复含义.
- 上述冻结的查找参数只供重放使用; `.` 不覆盖用户当前的字符查找或搜索记录. 标记跳转则记录标记名,
  重复时读取该标记的当前位置; 标记失效时失败.
- 其他边界, 空范围, register 写入, 标记维护和光标位置均遵守被重放命令本身的规则.

例如先 `dw`, 再移动, `yy`, 最后 `.`, 应再次执行 `dw`; yank 不会替换 Repeat 记录, 但它可以改变当前 unnamed register.

#### Saved State

保存不清除 Repeat 记录. 成功保存后, 当前 Buffer 内容成为新的 saved state. 任何编辑 (包括 `.`) 完成后, modified
应表示当前 Buffer 内容是否与最近一次成功保存的内容不同; 初始 saved state 为刚打开的 Buffer 内容. 若最终文本恢复为 saved
state, modified 为 false; 否则为 true. 不得仅因执行过某条编辑命令就永远保持 modified 为 true.

---

### Extra

这部分完全是可选的,完成任意部分均可得分,我们会在 CR 时人工检查你的实现

Advanced 范围之外的 search and jump, 例如正则搜索或 jumplist(Easy), 或 C-o,C-i 列表跳转

Visual/Replace mode(Moderate, 记得考虑颜色问题)

Syntax highlighting(Hard)

## 测试策略

### 单元测试

```
make test
```

make test 会自动构建并运行我们提供的模块测试,它只是用来帮助你 debug.模块测试通过与否和行为测试通过与否没有任何关系.
我们最终会依据 ACMOJ 上的行为测试给分

### 集成测试

在 ACMOJ 上的测试
我们会下发 Basic 部分的测试点并给出使用说明,但不会下发 Advanced 部分的测试点.

## 调试建议

**(重要) 终端异常时先恢复** 如果进程被强制终止导致 shell 显示异常, 可以在 shell 中运行 `reset` 或 `stty sane`.
正常控制流应依靠 Terminal 的 RAII 自动恢复.

**(重要) VSCode 终端行为**
你大概率需要在.vscode 文件夹的 settings.json 里面加 "terminal.integrated.allowChords": false 从而让 vscode
里面的终端不优先把按键截获并识别为 vscode 的内置快捷键

## 评分标准

basic: 60

advanced: 30

CR: 10

extra(bonus): 10

最后项目你一共可以获得的分数是 110 分,该项目 100 分即为满分,超出 100 的部分可以用来填补其他不满 100 分的大作业的得分
