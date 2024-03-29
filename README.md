# mind-vision-example

<img src="test\1.png" alt="微信截图_20220402145808" style="zoom:50%;" />
<img src="test\微信截图_20220402145816.png" alt="微信截图_20220402145816" style="zoom:50%;" />
<img src="test\微信截图_20220402145821.png" alt="微信截图_20220402145821" style="zoom:50%;" />
<img src="test\微信截图_20220402145826.png" alt="微信截图_20220402145826" style="zoom:50%;" />
<img src="test\微信截图_20220402145831.png" alt="微信截图_20220402145831" style="zoom:50%;" />
<img src="test\微信截图_20220402145839.png" alt="微信截图_20220402145839" style="zoom:50%;" />
<img src="test\微信截图_20220402145847.png" alt="微信截图_20220402145847" style="zoom:50%;" />
<img src="test\微信截图_20220402145852.png" alt="微信截图_20220402145852" style="zoom:50%;" />
<img src="test\微信截图_20220402145856.png" alt="微信截图_20220402145856" style="zoom:50%;" />
<img src="test\微信截图_20220402150630.png" alt="微信截图_20220402150630" style="zoom:50%;" />

## 工作计划

| 任务                 | 问题                                                         | 时间 | 测试 |
| -------------------- | ------------------------------------------------------------ | ---- | ---- |
| 设备显示栏           | ~~设备列表以及分类显示~~<br />~~扫描设备-置顶/取消置顶~~<br />~~打开/连接设备~~ |      |      |
| 多相机同时连接与现实 | 多种画面布局显示-单画面 4画面 9画面 自定义布局<br />不同的画面窗口与相机绑定<br />~~选定画面相机时参数、设备信息、状态信息显示跟随选定相机~~ |      |      |
| 相机参数设置         | ~~曝光控制-模式选择~~<br />~~曝光控制-自动曝光~~<br />~~曝光控制-手动曝光~~<br /><br />~~颜色调整-白平衡~~<br />~~颜色调整-颜色强化~~ <br />~~颜色调整-其他特效~~ <br />~~颜色调整-Raw2Rgb算法选择~~ <br /><br />~~查表变换-模式选择~~ <br />~~查表变换-调整参数动态生成LUT表~~ <br />~~查表变换-相机预设LUT表~~ <br />~~查表变换-自定义LUT表~~ <br />~~查表变换-LUT曲线~~<br /><br />~~图形变换-软件镜像、硬件镜像~~<br />~~图形变换-锐化~~<br />~~图形变换-降噪~~<br />~~图形变换-旋转~~<br />~~图形变换-镜头失真校正~~<br />~~图形变换-彩点~~<br />~~图形变换-平场校正~~<br /><br />~~视频参数-帧率~~ <br />~~视频参数-相机输出图像格式~~ <br />~~视频参数-RAW输出范围~~<br /><br />~~分辨率-相机模式 （预设、自动以（ROI））~~ <br />~~分辨率-预设选择~~ <br />~~分辨率-当前分辨率详细信息~~<br /><br />~~IO控制-in1 in2 in3配置~~ <br />~~IO控制-out1 out2 out3 out4配置~~<br /><br />~~相机输出模式-触发模式~~ <br />~~相机输出模式-触发参数~~ <br />~~相机输出模式-外触发参数~~ <br />~~相机输出模式-闪光灯信息号~~<br /><br />~~十字线-在采集的图像上画十字线~~<br /> <br />~~设备信息-版本信息~~ <br />~~设备信息-固件更新~~ <br />~~设备信息-设备昵称（设备列表中显示的设备名）~~ <br />~~设备信息-序列号~~<br /><br />~~参数保存加载-参数分组~~  <br />~~参数保存加载-保存~~ <br />~~参数保存加载-回复默认参数~~ <br />~~参数保存加载-保存当前配置参数到指定文件~~ <br />~~参数保存加载-从指定文件中加载参数（加载方式）~~ |      |      |
| 预览控制             | ~~播放/暂停~~                                                |      |      |
| 图像采集             | ~~抓拍-设置/提示~~<br />~~录像-设置/暂停/停止~~              |      |      |
| 工具                 | ~~IP配置工具~~<br />~~日志查看~~                             |      |      |
| 帮助                 | ~~关于软件~~<br />~~查看文档~~<br />~~查看demo教程~~         |      |      |
| 快捷交互按钮         | ~~执行一次软触发~~<br />~~开始预览~~<br />~~暂停预览~~<br />~~扩大/缩小~~<br />~~最大化窗口~~<br />~~抓拍~~<br />~~抓拍设置~~<br />~~自动曝光~~<br />~~一键白平衡~~ |      |      |
| 安装和卸载           | ~~Windows~~                                                  |      |      |
| 多语言切换           | ~~中文~~<br />~~英文~~                                       |      |      |
| 测试                 | 功能测试<br />用例测试                                       |      |      |
| 修改                 | 功能BUG解决                                                  |      |      |
| 产品交付             | UI演示升级交付                                               |      |      |

## 测试用例

### 相机选型

- MV-SUA630C
- MV-UBS500M
- MV-UBS300C-T
- MV-GE200C
- MV-GE630C
- MV-GED500M
- MV-GE134GM2

### 相机列表-刷新

- 插入相机接口。
- 左侧相机面板中左击刷新。
- 显示已连接的相机。

### 相机预览

- 左侧相机面板中选择一个相机右击打开。
- 若：相机已在使用，则会弹出错误窗口。
- 几秒后，画面将会在显示在预览窗口中。
- 下侧状态面板会显示部分预览参数。

### 相机预览-切换相机

- 对多个相机，开始`相机预览`。
- 在多个相机间左击选中。
- 若：相机未打开，则预览窗口不会显示画面。
- 相机画面会显示在预览窗口中。

### 相机预览-关闭

- 对一个相机，开始`相机预览`。
- 选择一个已打开的相机右击关闭。
- 若：其他相机处于打开中，几秒后，画面将被关闭。
- 全部相机都已关闭，几秒后，默认画面将显示在预览窗口中。

### 相机预览-缩放

- 开始`相机预览`。
- 在下侧控制面板中左击`放大`。
- 若：上滑滚轮放大。
- 相机画面在预览窗口中将会变大。
- 下侧状态面板中`缩放`值将会变大。
- 在下侧控制面板中左击`缩小`。
- 若：下滑滚轮缩小。
- 相机画面在预览窗口中将会缩小。
- 下侧状态面板中`缩放`值将会变小。

### 相机预览-全屏预览

- 开始`相机预览`。
- 在下侧控制面板中左击`全屏`。
- 预览画面将显示在整个屏幕上。
- 按`ESC`退出全屏。

### 相机预览-单次截图

- 开始`相机预览`。
- 在下侧控制面板中左击抓取。
- 截图默认会保存在系统个人目录的`图片`文件夹下。
- 打开图片将会是抓取时的画面。

### 相机预览-录像

- 开始`相机预览`。
- 在下侧控制面板中左击录像。
- 录像默认会保存在系统个人目录的`视频`文件夹下。
- 在下侧控制面板中座机录像-停止。
- 打开视频将会是录像时的画面。

### 相机预览-暂停/播放

- 开始`相机预览`。
- 在下侧控制面板中左击`暂停`。
- 预览画面将会静止。
- 在下侧控制面板中左击`播放`。
- 预览画面将会非静止。
- 该功能会影响到`录像`功能。

### 相机预览-曝光

- 开始`相机预览`。
- 在右侧参数面板中选择曝光。
- 设置模式`自动`。
- 修改`亮度`值。
- 预览画面将会发生一些亮度变化。
- 设置模式`手动`。
- 修改`增益`值。
- 预览画面将会发生一些亮度变化。
- 修改`曝光时间`值。
- 预览画面将会发生一些亮度变化。

### 相机预览-曝光-抗闪频

- 开始`相机预览`。
- 在右侧参数面板中选择`曝光`。
- 设置模式`自动`。
- 启动`抗闪频`。
- 设置`抗闪频-频率`。
- ？？？？？？？？？？？？？？？？？？？？？？

### 相机预览-颜色-色温

- 开始`相机预览`。
- 在右侧参数面板中选择颜色。
- 设置模式`手动`。
- 设置不同`色温`项。
- 预览画面将会发生一些颜色变化。

### 相机预览-颜色-快速白平衡

- 开始`相机预览`。
- 在右侧参数面板中选择颜色。
- 设置模式`手动`。
- 左击`一次白平衡`。
- 若：亦可左击下侧控制面板`白平衡`。
- 预览画面将会发生一些颜色变化。
- 启用`显示白平衡窗口`。
- 预览画面将会出现一个矩形区域。
- 鼠标左击滑动设置这个矩形区域。
- 左击`确定白平衡窗口`。
- 左击`一次白平衡`。
- 预览画面将会发生一些颜色变化。

### 相机预览-颜色-白平衡

- 开始`相机预览`。
- 在右侧参数面板中选择颜色。
- 设置模式`手动`。
- 更改`RGB`及`饱和度`的值。
- 预览画面将会发生一些颜色变化。

### 相机预览-颜色-特效

- 启用`反转`。
- 预览画面将会变成底片色。

### 相机预览-颜色-算法

- 开始`相机预览`。
- 在右侧参数面板中选择颜色。
- 设置`算法`。
- 预览画面将会发生一些颜色变化。

### 相机预览-查表变换-伽马对比度

- 开始`相机预览`。
- 在右侧参数面板中选择`查表变换`。
- 设置模式`动态`。
- 设置`伽马`。
- 设置`对比度`。 
- 预览画面将会发生一些变化。
- 设置模式`预设`。
- 设置`预设`。
- 预览画面将会发生一些变化。

### 相机预览-变换-镜像

- 开始`相机预览`。
- 在右侧参数面板中选择`变换`。
- 启用`水平镜像`。
- 预览画面将会左右交换。
- 启用`垂直镜像`。
- 预览画面将会上下交换。

### 相机预览-变换-锐度

- 开始`相机预览`。
- 在右侧参数面板中选择`变换`。
- 启用`锐度`。
- 预览画面将会发生变化。

### 相机预览-变换-降噪

- 开始`相机预览`。
- 在右侧参数面板中选择`变换`。
- 启用降噪`2D`或`3D`。
- 预览画面将会发生变化。

### 相机预览-变换-旋转

- 开始`相机预览`。
- 在右侧参数面板中选择`变换`。
- 设置`旋转`角度。
- 预览画面将会发生变化。

### 相机预览-变换-畸变校正

- 开始`相机预览`。
- 抓取多张带有标定板照片。
- 在右侧参数面板中选择`变换`。
- 左击标定。
- 在标定窗口左击`载入`标定板照片后左击`矫正`。
- 畸变的照片将会被矫正。
- 点击`应用`。
- 勾选`启用`，预览画面将被矫正。

### 相机预览-变换-坏点

- 开始`相机预览`。
- 在右侧参数面板中选择`变换`。
- 勾选`坏点编辑`。
- 预览窗口进行缩放到像素级别，通过左击点击像素点来确定坏点。
- 点击`保存`。
- 勾选`启用`。
- ？？？？？？？？？

### 相机预览-变换-平场矫正

- 开始`相机预览`。
- 在右侧参数面板中选择`变换`。
- 让摄像头对黑色背景左击`暗场`。
- 让摄像头对白色背景左击`明场`。
- 左击`保存`。
- 勾选`启用`。
- ？？？？？？

### 相机预览-视频-帧率

- 开始`相机预览`。
- 在右侧参数面板中选择`视频`。
- 选择`高帧率`。
- 预览画面会更加流畅。
- 选择`低帧率`。
- 预览画面会更加卡顿。
- 设置`帧率限制`。
- 预览画面的帧率不会超过限制的值。

### 相机预览-分辨率

- 开始`相机预览`。
- 在右侧参数面板中选择`分辨率`。
- 选择模式`预设`。
- 选择分辨率。
- 预览画面将会发生变化。
- 选择模式`自定义`。
- 设置`x`和`y`。
- 预览画面将会发生变化。

### 相机预览-IO

- 开始`相机预览`。
- 在右侧参数面板中选择`IO`。
- ？？？？？？？？？？？？

### 相机预览-输出模式

- 开始`相机预览`。
- 在右侧参数面板中选择`输出模式`。
- 选择触发模式`连续`。
- 预览画面将会持续更新。
- 选择触发模式`软件`。
- 左击软件触发一次。
- 预览画面将会更新一次。
- 选择触发模式`硬件`。
- ？？？？？？？？？？？？？
- 预览画面将会更新一次。

### 相机预览-十字线

- 开始`相机预览`。
- 在右侧参数面板中选择`十字线`。
- 设置`x`和`y`的坐标。
- 勾选线条编号。
- 在预览窗口将会出现一条十字线。
- 可以同时显示多条十字线。

### 相机预览-设置-昵称

- 开始`相机预览`。
- 在右侧参数面板中选择`设置`。
- 设置`昵称`左击修改。
- 重新打开相机后，相机昵称会被更改。

### 相机预览-设置

- 开始`相机预览`。
- 在任何参数面板进行修改后。
- 在右侧参数面板中选择`设置`。
- 左击`保存`。
- 重新打开相机后，相机预览画面将会已上次修改后的参数呈现。
