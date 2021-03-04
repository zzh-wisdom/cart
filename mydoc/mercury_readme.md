# Mercury

Mercury是专门为HPC系统设计的**RPC框架**，它允许参数和执行请求的**异步传输**以及对**大数据参数的直接支持**。网络的实现是抽象的，可以轻松移植到将来的系统，并有效利用现有的本机传输机制。Mercury的接口是通用的，并允许对任何函数调用进行序列化。

Mercury是一个用于实现远程过程调用的C库，该库针对高性能计算系统进行了优化。Mercury支持包含**大量数据参数**的远程函数调用。由于代码生成是使用C预处理程序完成的，因此不需要任何外部工具。

## 支持的架构

网络抽象层通常支持MPI实现所支持的体系结构。OFI libfabric插件以及SM插件都很稳定，并在大多数工作负载中提供最佳性能。当前支持的Libfabric提供程序包括：`tcp`，`verbs`，`psm2`，`gni`。 MPI和BMI（tcp）插件仍受支持，但已逐渐弃用，因此应仅用作后备方法。不建议使用CCI插件，并且不再支持基础CCI传输插件（`tcp`，`sm`，`verbs`，`gni`）。

有关[插件要求](https://github.com/mercury-hpc/mercury#plugin-requirements)的详细信息，请参见插件要求部分。

## Documentation

请参阅mercury[网站](http://mercury-hpc.github.io/documentation/)上的文档，以快速了解mercury。

## 软件要求

编译和运行Mercury需要各种软件包的最新版本。请注意，使用这些程序包的过旧版本可能会导致间接错误，很难跟踪。

(...)
