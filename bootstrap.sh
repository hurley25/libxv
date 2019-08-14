#!/usr/bin/env bash

set -e

build_all()
{
    rm -rf build && mkdir build && cd build && cmake .. && make -j && cd ..
}

run_tests()
{
    cd build && make test && cd ..
}

clean_all()
{
    rm -rf ${PROJECT_DIR}/build
}

if [ $# == 1 ]; then
    if [ $1 = "clean" ]; then
        clean_all
    elif [ $1 = "build" ]; then
        build_all
    elif [ $1 = "test" ]; then
        clean_all
        build_all
        run_tests
    fi
    exit 0
fi

printf "Usage:\n\t./bootstrap.sh build 编译程序\n\t./bootstrap.sh test  运行测试\n\t./bootstrap.sh clean 清理\n\n"

