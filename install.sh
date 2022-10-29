#!bin/sh/

#获取项目绝对路径
current_path=$(cd "$(dirname $0)";pwd)

cd ${current_path}/build
rm -rf ./*
cmake ..
make