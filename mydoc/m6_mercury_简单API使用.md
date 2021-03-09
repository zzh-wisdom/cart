# Mercury 简单API使用

## NA 网络抽象层

普通用户可以不是这些API，通过调用相应的RPC和Bulk层的API也会自动完成NA层的操作。

主要对象：

- na_class_t，NA类
- na_context_t，NA上下文
- na_addr_t，源/目标地址

这些对象都有相应的创建和销毁API函数。

后面两层的类型和方法大都以`hg`开头，hg表示汞，即mercury

## RPC 层

主要对象：

- hg_class_t，hg rpc接口类
- hg_context_t，hg上下文
- callback，回调函数，用于函数参数编解码

前两个都有创建和销毁的方法。

对于callback，必须使用相同的func_name标识符在源和目标上进行注册。或者，可以使用HG_Register()传递用户定义的`唯一标识符`（可以自己定义，整形），并避免对提供的函数名称进行内部哈希处理。
