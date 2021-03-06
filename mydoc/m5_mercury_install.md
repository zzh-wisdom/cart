# mercury install

官网安装文档：<https://github.com/mercury-hpc/mercury#building>

## clone 代码

```shell
git clone --recurse-submodules https://github.com/mercury-hpc/mercury.git
```

如果忘了clone子模块，则运行

```shell
git submodule update --init
```

## build

```shell
cd mercury
mkdir build
cd build
ccmake ..
```

运行上述命令后会跳出一个配置界面。

按字母`c`会弹出配置选项，按照文档的说明配置即可。

然后继续按`c`，如果没有看到任何错误，最后下面会多一个`g`的选项，按`g`，然后就会在当前目录生成一个Makefile文件。

编译

```shell
make
```

## install

```shell
make install
```

