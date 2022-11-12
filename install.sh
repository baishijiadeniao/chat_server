#!bin/sh/



#获取项目绝对路径
current_path=$(cd "$(dirname $0)";pwd)
sh ${current_path}/my_muduo/install.sh

if [ ! -d "${current_path}/build" ]; then
    mkdir ${current_path}/build/
fi

cd ${current_path}/build
rm -rf ./*
cmake ..
make
