# Getting started with Mchecksum

<https://mercury-hpc.github.io/documentation/2018/10/24/mchecksum.html>

Mchecksum是在Mercury上下文中创建的库，用于对RPC头部以及RPC函数参数进行校验和。在Mercury中，不为批量数据传输计算校验和，并由用户决定是否验证传输。由于mchecksum是一个单独的库，因此应用程序可以独立使用它来验证数据完整性。 Mchecksum提供了一个简单的界面，并使用插件系统来抽象和提供各种哈希方法。

## 接口

