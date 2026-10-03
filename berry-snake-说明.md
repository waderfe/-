# 莓莓蛇 · C++ 卡通贪吃蛇

这是 C++ 实现的 Windows 窗口游戏。图形由 Windows 自带的 GDI+ 绘制，不需要 HTML、浏览器或额外图片素材。

## 直接游玩

在 Windows 10 / 11 的 64 位电脑上，双击 `berry-snake.exe`，点击「开始吃莓」。不需要安装编译器。

- 方向键 / WASD：移动。也可以点击右侧的方向按钮。
- 空格 / Enter：开始、暂停或继续。
- Esc：暂停。
- 点击「重新开始」：开启新的一局。
- 切换到其他窗口时，游戏会自动暂停。
- 每颗草莓得 10 分，每吃 5 颗升一级，速度逐渐增加。
- 撞到边界或自己的身体会结束游戏；填满 20 × 20 棋盘则获胜。
- 三档难度的选择在下一局生效。

最高纪录保存在当前 Windows 用户的 `HKEY_CURRENT_USER\Software\BerrySnake` 注册表项中。

## 编译源代码

源码：`berry-snake.cpp`。使用 Windows 下的 MinGW-w64 / MSYS2 g++，在源码所在目录执行：

```powershell
g++ berry-snake.cpp -std=c++17 -O2 -municode -mwindows -static -o berry-snake.exe -lgdiplus -lgdi32 -luser32 -ladvapi32
```

程序采用静态链接 C++ 运行库，图形和窗口部分只依赖 Windows 系统组件。图形版使用 Windows API，不能直接作为 macOS / Linux 图形程序运行。

## 游戏逻辑检查

同一个源文件内提供无窗口的逻辑检查模式：

```powershell
g++ berry-snake.cpp -std=c++17 -O2 -DBERRY_SNAKE_SELF_TEST -static -o snake-tests.exe
./snake-tests.exe
```

已经通过移动、禁止反向、连续转弯、食物生成、成长升级、边界及身体碰撞、移动到刚腾出的尾格、暂停、胜利、重新开始和难度检查。

`berry-snake-preview.png` 由同一个 C++ 绘图程序实际输出；原生绘制和 PNG 导出已成功运行。
