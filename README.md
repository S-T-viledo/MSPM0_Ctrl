\# MSPM0\_Ctrl



本仓库用于 TI MSPM0 系列嵌入式控制类项目开发，开发环境为 Code Composer Studio / CCS Theia。

本项目使用 Git 进行版本管理和团队协作，方便三名队员共享工程文件、追踪修改记录、回退错误版本，并提高开发沟通效率。



\---



\## 1. 项目目录说明



当前仓库大致结构如下：



```text

MSPM0\_Ctrl/

├─ README.md

├─ .gitignore				// 用来设置哪些文件不用追踪和提交

├─ .gitattributes

├─ CCS1/

│  ├─ .ccsproject

│  ├─ .cproject

│  ├─ .project

│  ├─ .settings/

│  ├─ empty.c

│  ├─ empty.syscfg

│  ├─ targetConfigs/

│  └─ README.md

└─ hardware/

&#x20;  └─ 引脚分配.txt

```



其中：



```text

CCS1/           CCS 工程目录

hardware/       硬件相关资料，例如引脚分配、接线说明

README.md       项目说明与 Git 使用说明

.gitignore      指定哪些文件不需要提交到 Git

.gitattributes  统一文本文件换行规则，减少不同电脑之间的换行差异

```



\---



\## 2. 第一次下载项目



队友第一次参与开发时，先在电脑上安装 Git，然后选择一个合适的目录，执行：



```powershell

git clone https://github.com/S-T-viledo/MSPM0\_Ctrl.git

```



进入项目目录：



```powershell

cd MSPM0\_Ctrl

```



然后用 CCS / CCS Theia 导入工程。



一般操作路径为：



```text

File -> Import Project

```



或者：



```text

File -> Import -> Code Composer Studio Project

```



选择仓库中的 `CCS1` 目录进行导入。



注意：不要重新新建一个工程再复制代码，最好直接导入仓库里的 CCS 工程。



\---



\## 3. 每天开始写代码前必须执行



每次开始写代码前，先进入项目目录：



```powershell

cd 路径\\MSPM0\_Ctrl

```



然后执行：



```powershell

git pull

```



这个命令用于拉取队友已经上传的最新代码。



建议养成习惯：



```text

每天写代码前，先 git pull

每天写代码后，再 git add / commit / push

```



\---



\## 4. 查看当前修改状态



随时可以执行：



```powershell

git status

```



它会告诉你：



```text

哪些文件被修改了

哪些文件是新建的

哪些文件已经准备提交

当前分支是否落后于远程仓库

```



如果不知道现在 Git 处于什么状态，优先执行：



```powershell

git status

```



\---



\## 5. 提交自己的修改



写完代码并确认可以编译后，执行：



```powershell

git add .

git commit -m "这里写本次修改说明"

git push

```



例如：



```powershell

git add .

git commit -m "Add motor PWM control"

git push

```



或者：



```powershell

git add .

git commit -m "Fix encoder speed calculation"

git push

```



提交说明要尽量写清楚详细，不要写成：



```text

update

111

修改

最终版

```



建议写成：



```text

Add motor driver module

Fix PWM direction bug

Update pin assignment document

Tune PID parameters

Add OLED display test code

```



\---



\## 6. 推荐的日常协作流程



普通开发时，可以先简单使用 `main` 分支。



基本流程：



```powershell

git pull

```



然后写代码、编译、测试。



测试没问题后：



```powershell

git add .

git commit -m "说明这次改了什么"

git push

```



如果之后项目变复杂，可以增加开发分支，例如：



```text

main              稳定版本，必须能编译、能烧录

dev               日常开发版本

feature/motor     电机控制开发分支

feature/encoder   编码器开发分支

feature/pid       PID 控制开发分支

feature/oled      显示模块开发分支

```



创建新分支示例：



```powershell

git checkout -b feature/motor

```



推送新分支：



```powershell

git push -u origin feature/motor

```



切回主分支：



```powershell

git checkout main

```



\---



\## 7. 哪些文件应该提交



建议提交：



```text

\*.c

\*.h

\*.syscfg

.project

.cproject

.ccsproject

.settings/

targetConfigs/

README.md

hardware/

docs/

```



原因：



```text

.c / .h              源代码

.syscfg              MSPM0 SysConfig 外设配置

.project 等文件      CCS 工程配置

targetConfigs/       调试目标配置

hardware/            硬件接线、引脚分配等团队资料

README.md            项目说明和协作说明

```



\---



\## 8. 哪些文件不应该提交



不建议提交：



```text

Debug/

Release/

\*.out

\*.map

\*.hex

\*.bin

\*.elf

\*.obj

\*.o

\*.d

\*.log

```



这些通常是编译生成文件，每个人电脑上都会重新生成。

如果提交这些文件，容易造成仓库变大、冲突变多，而且没有必要。



\---



\## 9. 修改引脚时的注意事项



如果修改了 MSPM0 的引脚配置，需要同步检查和提交：



```text

\*.syscfg

ti\_msp\_dl\_config.c

ti\_msp\_dl\_config.h

hardware/引脚分配.txt

```



如果只改了代码，但没有更新引脚说明，其他队友很容易接错线。



建议每次改引脚后，都在 `hardware/引脚分配.txt` 中写清楚：



```text

功能          引脚          说明

左电机 PWM   PAxx       TIMx 输出

右电机 PWM   PAxx       TIMx 输出

编码器 A     PAxx       外部中断/定时器捕获

编码器 B     PAxx       外部中断/定时器捕获

OLED SCL     PAxx       I2C

OLED SDA     PAxx       I2C

```



\---



\## 10. 减少冲突的开发习惯



不要三个人同时频繁修改同一个文件，尤其是：



```text

main.c

\*.syscfg

```



推荐把功能拆成模块：



```text

main.c              只负责初始化和主循环

motor.c / motor.h   电机控制

encoder.c / .h      编码器测速

pid.c / pid.h       PID 算法

oled.c / oled.h     显示模块

key.c / key.h       按键

app.c / app.h       上层控制逻辑

```



这样每个人主要修改自己的模块，冲突会少很多。



\---



\## 11. 如果出现冲突怎么办



如果执行 `git pull` 或合并分支时出现：



```text

CONFLICT

```



不要慌。打开冲突文件，会看到类似内容：



```c

<<<<<<< HEAD

Motor\_Init();

=======

Encoder\_Init();

>>>>>>> feature/encoder

```



含义是：



```text

<<<<<<< HEAD 到 ======= 之间是你当前版本

======= 到 >>>>>>> 之间是另一个版本

```



需要手动改成正确代码，例如：



```c

Motor\_Init();

Encoder\_Init();

```



然后执行：



```powershell

git add 冲突文件名

git commit -m "Resolve merge conflict"

git push

```



如果不确定怎么解决冲突，先不要乱删，可以在群里说明冲突文件，并让相关队友一起确认。



\---



\## 12. 回退到上一个稳定版本



查看提交历史：



```powershell

git log --oneline

```



如果只是想撤销工作区还没提交的修改：



```powershell

git restore 文件名

```



如果想撤销所有还没提交的修改：



```powershell

git restore .

```



注意：这个操作会丢弃本地未提交修改，执行前要确认。



\---



\## 13. 打稳定版本标签



当某个版本确认可以正常编译、烧录、运行时，可以打标签：



```powershell

git tag v1.0-basic-car

git push origin v1.0-basic-car

```



例如：



```text

v1.0-basic-car       小车基础运动可用

v1.1-encoder         编码器测速可用

v1.2-pid             PID 闭环控制可用

v2.0-contest-final   比赛最终稳定版本

```



这样以后如果代码改坏了，可以很方便地找回之前的稳定版本。



\---



\## 14. 推荐队伍约定



1\. 写代码前必须 `git pull`。

2\. 编译不通过的代码不要直接 push 到主分支。

3\. 不要提交 `Debug/`、`Release/` 和各种编译产物。

4\. 修改引脚后必须同步更新 `hardware/引脚分配.txt`。

5\. 不要三个人同时大量修改 `main.c`。

6\. 每次 commit 信息要写清楚，不要只写 `update`。

7\. 重要稳定版本要打 tag。

8\. 出现冲突时不要慌，先看冲突文件，再和相关队友确认。



\---



\## 15. 常用命令速查



```powershell

git clone 仓库地址

```



下载仓库。



```powershell

git pull

```



拉取队友最新代码。



```powershell

git status

```



查看当前修改状态。



```powershell

git add .

```



把所有修改加入暂存区。



```powershell

git commit -m "修改说明"

```



提交一个版本。



```powershell

git push

```



上传到 GitHub。



```powershell

git log --oneline

```



查看提交历史。



```powershell

git checkout -b 分支名

```



创建并切换到新分支。



```powershell

git checkout 分支名

```



切换分支。



```powershell

git restore 文件名

```



撤销某个文件未提交的修改。



```powershell

git restore .

```



撤销所有未提交修改。



\---



\## 16. 推荐最简单工作流



对目前三人小队来说，先按下面流程做就够了：



```powershell

git pull

```



写代码，编译，烧录，测试。



```powershell

git status

git add .

git commit -m "说明本次修改"

git push

```



只要坚持这个流程，就能避免大部分“代码版本混乱”和“互相覆盖文件”的问题。



