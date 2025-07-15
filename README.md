# H264 软件解码器

基于 **H264/AVC 标准**（Rec. ITU-T H.264 08/2021）实现的轻量级解码器，支持主流编码特性，纯 C/C++ 编写，跨平台运行。

## ✨ 功能特性
- **完整帧处理** 
  ✅ 帧解码（I/P/B帧） 
  ✅ 场解码（交错视频支持） 
  ✅ B帧双向预测  
- **熵解码** 
  ✅ CAVLC（上下文自适应变长编码） 
  ✅ CABAC（上下文自适应二进制算术编码）  
- **跨平台** 
  ✅ Linux & macOS & Windows 已测试 （ X86 或 Arm 架构）

## 🛠 编译指南

### 依赖项
- **编译器**：GCC/Clang（C11 或 C++11 标准）
- **构建工具**：CMake ≥ 3.10（推荐）或直接使用 Makefile
- **可选工具**：`ffmpeg`（用于生成测试流）

### 编译步骤
```bash
# 使用 CMake
$ cmake -B build . #对于Linux、MacOS 平台
# $ cmake -G "MinGW Makefiles" -B build . #对于Windows 平台
# 可选的编译选项
#-DSHOW_MB_BORDER #开启宏块分割线条显示
#-DDISABLE_COUT  #关闭信息输出
#-DSKIP_MB_DECODE  #跳过宏块解码

$ make -j4 -C build
```

## 🚀 使用示例
```bash
$ ./build/a.out intput.h264
```
- `input.h264`：原始 H264 比特流文件  
- `output.yuv`：输出 YUV 格式像素数据（默认在当前目录下输出）  

## 📜 许可证
本项目采用 **MIT 许可证**，详见 [LICENSE](LICENSE) 文件。  
欢迎提交 Issue 或 PR 参与改进！

## 🙋 常见问题
**Q：如何生成测试 H264 文件？**  
```bash
ffmpeg -i test.mp4 -c:v libx264 -preset fast -an test.h264
```

**Q：解码输出 YUV 如何播放？**  
```bash
ffplay -f rawvideo -video_size 1920x1080 output.yuv
```

> 💡 提示：使用 `--help` 查看所有命令行参数。

---

### 关键设计说明
1. **分层结构**：  
   - 将 NALU 解析、熵解码、宏块重建等模块分离，便于维护。
2. **扩展性**：  
   - 预留了 SEI 和 VUI 参数解析接口（可后续扩展）。
3. **测试建议**：
   - 推荐使用 [JM 参考软件](https://iphome.hhi.de/suehring/tml/) 的测试流验证兼容性。

### 后续性能优化

实现这个项目的主要目的是为了熟悉H264的编码算法，在实现的过程中是没有对算法进行性能优化调整，所以目前的优化模块主要是通过Clang/Gcc的自动向量化实现的解码加速。目前还没有计划对项目进行一个完整的性能优化。
