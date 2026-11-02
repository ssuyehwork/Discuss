# 时间输出工具 (Time_tool) 真·单 EXE 编译保姆级教程

这份教程专门为您准备，旨在帮助您在不具备编程背景的情况下，亲手编译出一个完全自包含、无需任何 DLL 文件的单文件 `.exe` 应用。

---

## 第一阶段：环境准备（工具必须装全）

在开始之前，请确保您的电脑安装了以下工具。这就像做饭前要准备好厨具：

1. **Visual Studio 2022 (VS2022)**
   - **下载**：[点击前往官网](https://visualstudio.microsoft.com/zh-hans/vs/)
   - **安装关键点**：安装时必须勾选 **“使用 C++ 的桌面开发”**。
2. **Python 3**
   - **下载**：[点击前往官网](https://www.python.org/downloads/)
   - **安装关键点**：安装时务必勾选 **“Add Python to PATH”**。
3. **Perl**
   - **下载**：[点击下载 Strawberry Perl](https://strawberryperl.com/)
4. **CMake**
   - **下载**：[点击前往官网](https://cmake.org/download/)
   - **安装关键点**：安装时勾选 **“Add CMake to the system PATH for all users”**。
5. **Ninja**
   - **下载**：[点击下载 Ninja.exe](https://github.com/ninja-build/ninja/releases)
   - **操作**：下载后将 `ninja.exe` 放到一个文件夹里（比如 `C:\Tools`），并将该路径添加到电脑的系统环境变量 `PATH` 中。

---

## 第二阶段：编译静态版 Qt6（最耗时的一步）

Qt 官方默认提供的是“动态链接版”，为了做单 EXE，我们需要亲手制作一份“静态版”。

1. **下载源码**：
   - [下载 Qt 6.10.2 源码包](https://download.qt.io/official_releases/qt/6.10/6.10.2/single/qt-everywhere-src-6.10.2.tar.xz)
   - 解压到磁盘，路径建议短一点，例如：`D:\QtSrc`

2. **启动编译环境**：
   - 在 Windows 开始菜单搜索：**x64 Native Tools Command Prompt for VS 2022**（注意：必须是这个，不要用普通的 CMD）。

3. **执行配置与编译**：
   在打开的黑色窗口中，依次输入以下命令：

   ```batch
   :: 1. 创建并进入构建目录
   mkdir C:\Qt_Static_Build
   cd /d C:\Qt_Static_Build

   :: 2. 配置 Qt (配置约 10 分钟)
   :: 注意修改 D:\QtSrc 为您实际解压源码的路径
   D:\QtSrc\configure.bat -static -static-runtime -release -prefix "C:\Qt\6.10.2\msvc2022_64_static" -platform win32-msvc -no-opengl -skip qtwebengine -skip qtmultimedia -- -DCMAKE_BUILD_TYPE=Release

   :: 3. 开始编译 (这步最慢，根据 CPU 性能可能需要 1-2 小时)
   cmake --build . --parallel

   :: 4. 安装
   cmake --install .
   ```

编译完成后，您的 `G:\Qt\6.10.2\msvc2022_64_static` 文件夹里就有了“静态版 Qt”。

---

## 第三阶段：编译您的 Time_tool 软件

现在我们利用刚才做好的“厨具”来做“正餐”。

1. **回到您的项目目录**（即含有 `CMakeLists.txt` 的文件夹）。
2. **非常重要：清理旧缓存**。
   - 如果您之前编译失败过，请务必**手动删除**项目文件夹下的 `build-static` 目录（或者您创建的任何构建目录）。
3. **再次打开** **x64 Native Tools Command Prompt for VS 2022**。
4. **输入以下命令进行打包编译**：

   ```batch
   :: 1. 进入项目文件夹 (假设在 G:\C++\Time_tool\Time_tool)
   cd /d G:\C++\Time_tool\Time_tool

   :: 2. 创建全新的构建目录
   mkdir build-static
   cd build-static

   :: 3. 使用静态 Qt 配置项目
   cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Release

   :: 4. 执行最后一步生成
   cmake --build . --config Release
   ```

---

## 第四阶段：成果验收

1. **寻找文件**：编译完成后，回到项目根目录。
2. **查看 EXE**：在根目录下，您会找到直接生成的 `Time_tool.exe`。
3. **验证独立性**：
   - 这个 `.exe` 的体积通常会比普通的大一些（因为它把所有库都“吞”进去了）。
   - 将它拷贝到任何一台没有安装过 Qt、没有安装过 VS 的纯净 Windows 电脑上，它都能直接双击运行。

---

## 常见问题处理

*   **报错：找不到 Qt6**：请检查 `CMakeLists.txt` 中 `CMAKE_PREFIX_PATH` 的路径是否与您实际安装静态 Qt 的路径一致（当前设置为 `G:/Qt/6.10.2/msvc2022_64_static`）。
*   **报错：无法找到平台插件**：这是因为没有正确链接 `qwindows` 插件。请确保 `CMakeLists.txt` 中包含 `qt_import_plugins` 段落（我已经为您写好了）。
*   **运行时弹窗缺少 MSVCP140.dll**：说明编译时未正确开启 `/MT` 选项。请检查 `CMakeLists.txt` 中是否包含 `add_compile_options($<$<CONFIG:Release>:/MT>)`。

恭喜！您已经完成了程序员界的高阶操作：**静态编译与发布**。
