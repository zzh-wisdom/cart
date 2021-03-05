# Mercury论文概要

传统RPC的缺点：

- Socket网络接口（基于TCP/IP）效率低。这些协议限制传输数据的大小，即使没有限制，通常也建议不要通过RPC库传输大量数据，这主要是由于**序列化和编码的开销**。导致传输过程中数据的多次复制。
- 不支持处理大数据参数

## What

- 提供异步接口的RPC框架
- 专门设计用于HPC系统
- 参数和执行请求的异步传输，并提供对**大数据参数的直接支持**。

有效利用现有的原生(native)传输机制，即无法最大化地发挥机器的传输能力。

Mercury主要包含3个主要组件：

- 网络抽象层(NA)
- RPC 层。能够以通用方式处理调用的RPC接口。
- 大数据层。通过抽象内存段轻松地传输大量数据。
- 还有第四个更高级的层(参考官网文档)

我们区分两种类型的传输：

- 包含典型函数参数的传输（通常较小，称为元数据metadata）
- 描述大量数据的函数参数的参数（称为批量数据bulk data）。

## 评估

Mercury的体系结构**使常规的RPC调用可以与句柄一起发送**，这些句柄可以在需要批量数据传输时描述连续和不连续的内存区域。在本节中，我们将介绍如何利用这一架构来构建**一种流水线机制**，该机制可以轻松地按需请求数据块。

### 管道批量数据传输

当人们想要重叠通信和执行时，流水线传输是一种典型的用例。在我们描述的体系结构中，请求处理大量数据将导致RPC请求从RPC客户端发送到RPC服务器以及批量数据传输。在常见的使用情况下，**服务器可以在执行请求的调用之前等待接收全部数据**。但是，通过流水线传输，人们可以在传输数据时开始处理数据，并避免为整个RMA传输支付等待时间的费用。如果RPC服务器**没有足够的内存**来处理所有需要发送的数据，则使用此技术也特别有用，在这种情况下，它还需要在处理数据时传输数据。 RPC客户端代码的简化版本如下所示。

```c
#define BULK_NX 16
#define BULK_NY 128

int main(int argc, char *argv[])
{
    hg_id_t rpc_id;
    write_in_t in_struct;
    write_out_t out_struct;
    hg_request_t rpc_request;
    int buf[BULK_NX][BULK_NY];
    hg_bulk_segment_t segments[BULK_NX];
    hg_bulk_t bulk_handle = HG_BULK_NULL;

    /* Initialize the interface */
    [...]

    /* Register RPC call */
    rpc_id = HG_REGISTER("write",
                        write_in_t, write_out_t);

    /* Provide data layout information */
    for (i = 0; i < BULK_NX; i++)
    {
        segments[i].address = buf[i];
        segments[i].size = BULK_NY * sizeof(int);
    }

    /* Create bulk handle with segment info */
    HG_Bulk_handle_create_segments(segments,
                                   BULK_NX, HG_BULK_READ_ONLY, &bulk_handle);

    /* Attach bulk handle to input parameters */
    [...]
    in_struct.bulk_handle = bulk_handle;

    /* Send RPC request */
    HG_Forward(server_addr, rpc_id,
               &in_struct, &out_struct, &rpc_request);

    /* Wait for RPC completion and response */
    HG_Wait(rpc_request, HG_MAX_IDLE_TIME,
            HG_STATUS_IGNORE);

    /* Get output parameters */
    [...]
    ret = out_struct.ret;

    /* Free bulk handle */
    HG_Bulk_handle_free(bulk_handle);

    /* Finalize the interface */
    [...]
}
```

客户端初始化时，它将注册要发送的RPC调用。由于此调用涉及**不连续**的批量数据传输，因此将创建并注册描述内存区域的内存段。然后将生成的bulk_handle与其他调用参数一起传递给HG_Forward调用。然后，可以等待响应并在请求完成后释放批量句柄（将来也可以发送通知，以允许批量句柄被更早释放，从而取消固定内存）。

```c
#define PIPELINE_BUFFER_SIZE 256
#define PIPELINE_SIZE 4

int rpc_write(hg_handle_t handle)
{
    write_in_t in_struct;
    write_out_t out_struct;
    hg_bulk_t bulk_handle;
    hg_bulk_block_t block_handle[PIPELINE_SIZE];
    hg_bulk_request_t bulk_request[PIPELINE_SIZE];
    void *buf[PIPELINE_SIZE];
    size_t nbytes, nbytes_read = 0;
    size_t start_offset = 0;

    /* Get input parameters and bulk handle */
    HG_Handler_get_input(handle, &in_struct);
    [...]
    bulk_handle = in_struct.bulk_handle;

    /* Get size of data and allocate buffer */
    nbytes = HG_Bulk_handle_get_size(bulk_handle);

    /* Initialize pipeline and start reads */
    for (p = 0; p < PIPELINE_SIZE; p++)
    {
        size_t offset = p * PIPELINE_BUFFER_SIZE;
        buf[p] = malloc(PIPELINE_BUFFER_SIZE);

        /* Create block handle to read data */
        HG_Bulk_block_handle_create(
            buf[p], PIPELINE_BUFFER_SIZE,
            HG_BULK_READWRITE, &block_handle[p]);

        /* Start read of data chunk */
        HG_Bulk_read(client_addr, bulk_handle,
                     offset, block_handle[p], 0,
                     PIPELINE_BUFFER_SIZE, &bulk_request[p]);
    }

    while (nbytes_read != nbytes)
    {
        for (p = 0; p < PIPELINE_SIZE; p++)
        {
            size_t offset = start_offset +
                            p * PIPELINE_BUFFER_SIZE;

            /* Wait for data chunk */
            HG_Bulk_wait(bulk_request[p],
                         HG_MAX_IDLE_TIME, HG_STATUS_IGNORE);
            nbytes_read += PIPELINE_BUFFER_SIZE;

            /* Do work (write data chunk) */
            write(fd, buf[p], PIPELINE_BUFFER_SIZE);

            /* Start another read */
            offset += PIPELINE_BUFFER_SIZE *
                      PIPELINE_SIZE;
            if (offset < nbytes)
            {
                HG_Bulk_read(client_addr,
                             bulk_handle, offset,
                             block_handle, 0,
                             PIPELINE_BUFFER_SIZE,
                             &bulk_request[p]);
            }
            else
            {
                /* Start read with remaining piece */
            }
        }
        start_offset += PIPELINE_BUFFER_SIZE * PIPELINE_SIZE;
    }

    /* Finalize pipeline */
    for (p = 0; p < PIPELINE_SIZE; p++)
    {
        HG_Bulk_block_handle_free(block_handle[p]);
        free(buf[p]);
    }

    /* Start sending response back */
    [...]
    out_struct.ret = ret;
    HG_Handler_start_output(handle, &out_struct);
}

int main(int argc, char *argv[])
{
    /* Initialize the interface */
    [...]

    /* Register RPC call */
    HG_HANDLER_REGISTER("write", rpc_write,
                        write_in_t, write_out_t);

    while (!finalized)
    {
        /* Process RPC requests (nonblocking) */
        HG_Handler_process(0, HG_STATUS_IGNORE);
    }

    /* Finalize the interface */
    [...]
}
```

每个RPC服务器一旦初始化，就必须在`HG_Handler_process`调用上循环，该调用等待新的RPC请求并执行相应的已注册回调（在同一线程或新线程中，具体取决于用户需求）。将请求反序列化后，即可使用bulk_handle参数获取要传输的数据的总大小，分配适当大小的缓冲区，然后开始批量数据传输。在此示例中，流水线大小设置为4，流水线缓冲区大小设置为256，这意味着将启动4个256字节的RMA请求。然后，可以等待第一个256字节到达并对其进行处理。
虽然它被处理时，其他数据片可能会到达。处理完一个数据片后，将为管道中处于阶段4的零件开始新的RMA传输，并且可以等待下一数据片并对其进行处理。请注意，虽然在客户端上注册的内存区域是不连续的，但是服务器上的HG_Bulk_read调用将其显示为连续的区域，从而简化了服务器代码。此外，可以提供逻辑偏移量（相对于数据的开头）来分别移动数据块，而批量数据接口则负责从连续逻辑偏移量到不连续客户端存储区域的映射。

我们重复此过程，直到所有数据都已被读取/处理，然后可以将响应（即函数调用的结果）发送回去。同样，我们仅通过调用HG_Handler_start_output调用开始发送响应，并且仅通过调用HG_Handler_process来测试其完成，在这种情况下，与响应相关的资源将被释放。请注意，所有功能都支持异步执行，并允许在需要的情况下在事件驱动的代码中使用Mercury。

