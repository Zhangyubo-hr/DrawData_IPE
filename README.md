<script type="text/javascript" src="http://cdn.mathjax.org/mathjax/latest/MathJax.js?config=TeX-AMS-MML_HTMLorMML"></script>
<script type="text/x-mathjax-config">
    MathJax.Hub.Config({ tex2jax: {inlineMath: [['$', '$']]}, messageStyle: "none" });
</script>

# 智慧体育项目视频处理专用程序

## 运行环境

- Ubuntu 20.04

- OpenCV 4.4

首先需要从源码编译并配置opencv，如果不会配置环境，请自己去谷歌搜索教程，也可以参考[相关博客(点击传送门直达)](https://www.jianshu.com/p/26dd452a362e)，如果opencv源码压缩包下载太慢，请使用[码云镜像下载](https://gitee.com/jiangdu/opencv/repository/archive/4.4.0?format=tar.gz)。

## 使用说明

1. 提前准备好`poseAdd2Id.json`放在该目录下。

2. 第一次运行时，需要先执行数据格式转换

    ```sh
    python3 convert_data.py
    ```

    执行之后会生成一个文件：

    > RawData.bin

3. 编辑相关参数

    ```sh
    vim config.h
    ```

    根据自己的视频文件路径修改代码中的：

    ```c
    #define VIDEONAME "./../datasets/ori.avi"
    
    #define ACTOR_NUM 1 // 演员编号：范围1-5
    ```

4. 编译运行

    ```sh
    ./make.sh
    ```

## 功能介绍

2020.7.29更新：

演员编号支持自由选择，方便生成分别关注5个人中每个人的独立视频。

2020.7.28更新：

基于抛物线的平滑原理：

设左关键帧的数值为$y_L$，右关键帧的数值为$y_R$，关键帧间隔为$d$，可以推导出抛物线的方程为：

$$y=ax^2+(\frac{y_R-y_L}{d}-da)x+y_L$$

其中a为待定系数，d在本程序中水平方向取$d=15$，竖直方向取$d=30$。

然后以左右关键帧为基准点，矫正中间每一帧的数值，实现平滑效果。

2020.7.28更新：

1. 对关注人的区域进行了平滑，采用了自适应非对称的阻尼门限方法。

2. 体操运动员头顶上的数字的位置设置更人性化。

3. 对计时器的运行机制进行了优化，输出平均帧率。由于H.264编码时间被统计进处理时间，fps显示会减少，但是实际上程序效率是提高的。

4. 左上角可以自由设置是否显示frame编号。

5. 写入视频机制优化，fps和视频尺寸也可以自适应了。

6. 视频处理结束后可调用ffmpeg以HEVC (`libx265`)进行二次编码压缩，进一步减少视频尺寸，如果安装了Nvidia显卡，可以用`hevc_nvenc`做编码器。

2020.7.27更新：

1. 改进底层操作方式，运行更流畅。

2. 代码结构优化，配置参数统一放在`config.h`文件中。

3. 支持自由选择是否保存视频，并支持设定格式(默认`H.264`)。

2020.7.24更新：

1. 已完成对单人的光照的简便渲染。

2. 对视频文件进行处理时，添加了滚动条动态显示进度的效果，更加人性化。

3. 目前提供了3种可视化方式：直接显示框、直接画框的外接圆、对关注的人的光照进行渐变渲染。默认使用方法3。

TODO:

- [ ] 基础功能
    - [x] 绘制人的框和圆形区域
    - [x] 对person的区域实现渐变的透明度
    - [X] 对人的ID编号进行标注
    - [X] 对数据进行平滑处理，解决跳动问题
    - [X] 演员编号支持自由选择，支持生成每个人的独立高亮视频
    - [ ] 对球的轨迹可视化
        - [ ] 实现动态透明度渐变(淡入淡出效果)
        - [ ] 优化视觉效果，自适应接球动作时轨迹的长度
- [ ] 性能优化
    - [x] 基于OpenCV和C语言的单线程CPU版
    - [x] 支持是否导出视频以及格式选择(H.264编码默认使用CPU多线程)
    - [ ] 调用C语言和python的接口，实现直接读json文件
    - [ ] 使用OpenCL或CUDA对视频处理进行速度优化

# LICENSE

[MIT License](./LICENSE)
