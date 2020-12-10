# CMake 最低版本号要求
cmake_minimum_required(VERSION 3.0.0)
# 项目信息
project(net VERSION 0.1.0)

# 添加头文件
include_directories(./include)
# 查找当前目录下的所有源文件，并将名称保存到DIR_SRCS变量
aux_source_directory(./src DIR_SRCS)
# 指定生成目标
add_executable(main ${DIR_SRCS})
# 添加链接库
# target_link_libraries(main pcap)