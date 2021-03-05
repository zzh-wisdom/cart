# Mercury的取消操作

Mercury提供两种单独的传输类型：RPC和批量数据。我们在此页面中介绍如何**取消**正在进行的操作以及如何从取消状态中**恢复**。

## 介绍

Mercury已被定义为分布式服务的基础。在这种情况下，增加对取消mercury运行的支持是提供弹性并允许服务在发生故障（例如，节点故障等）后恢复的主要要求。这意味着要回收已取消的操作先前分配的资源。Mercury定义了远程**查找操作**以及**两种类型的传输操作**，即RPC和批量数据传输，如果所涉及的任何一方不再响应或到达无响应时间（例如，在进程终止时），这些操作可能会中断。待处理的操作必须取消。为了达到正确的完成状态，必须取消由于发生故障或已达到超时而无法完成的操作。

本文档假定读者已经了解Mercury及其层（NA / HG / HG Bulk）。在GitHub第[34号issue](https://github.com/mercury-hpc/mercury/issues/34)中引用了添加mercury取消功能。

## 进度模型

Mercury最近切换到了基于回调的机制，该机制建立在网络抽象（NA）层的回调机制之上。与传统的基于请求的模型相比，**回调机制具有两个优点：没有显式的`wait()`控制点；无需任何其他代码分支即可轻松完成操作取消**。

Mercury的进度直接取决于NA的进度。NA操作完成后，内部回调将推入NA的完成队列。为了在Mercury的操作上取得进展，Mercury层还在内部触发了这些NA回调操作。触发NA操作可能反过来导致Mercury操作完成。这些操作完成后，与这些操作关联的用户回调又被推送到完成队列。(即一环扣着一环的意思吧)

RPC操作与由用户显式创建并与执行目标链接的句柄相关联。句柄以及包含输入参数的结构被传递到**转发调用forward**操作，该操作对输入参数进行序列化，然后将RPC参数传递给目标。完成后，传递给**转发调用**的用户回调将被推送到完成队列。RPC转发操作完成时，会将相同的句柄传递给`get output`调用，以检索输出参数并将其反序列化。请注意，在完成（或取消）之后，可以安全地**重用该句柄**，以向同一目标发出另一个RPC。当用户不再需要对给定的句柄进行操作时，可以将其显式销毁。**参考计数**可确保操作安全。

## RPC取消

在这种情况下，可以定义Mercury的RPC取消，以便：

- 不需要显式跟踪句柄（用户调用`HG_Destroy()`）。
- 用户回调以已取消状态被推送到完成队列。
- 取消的句柄可以重新使用，以重试将调用转发到目标。

当对HG操作的取消操作完成时，在该HG操作涉及的NA操作的内部也完成了取消操作。当此取消成功完成并且NA操作以已取消状态完成时，与NA操作关联的HG回调将被推送到完成队列。当这些回调以取消状态执行时，实际的HG操作被成功取消，并且传递给回调函数的操作状态表明该操作已取消。

重要的是要注意，从某种意义上讲，取消总是在本地进行的，这意味着与远程方不涉及任何通信（即，**它不依赖于远程方能够进行通信**）。

还值得注意的是，我们实现Mercury的RPC取消的主要重点是**从故障中恢复**，而不是对依赖于取消的RPC传输脚本的支持（例如，数据服务中的乐观查询或更高级别的协议操作，从理论上讲也可以得到支持）。

可以支持以下操作的取消：

- `HG_Forward()`，`HG_Respond()`（RPC操作）
- `HG_Bulk_transfer()`（批量操作）
- `HG_Addr_lookup()`（Lookup操作，尚不支持取消）

我们定义以下调用：

- RPC操作：

    ```c
    hg_return_t HG_Cancel(hg_handle_t handle);
    ```

- 批量操作：

    ```c
    hg_return_t HG_Bulk_cancel(hg_op_id_t op_id);
    ```

- 查找操作：

    ```c
    hg_return_t HG_Lookup_cancel(hg_op_id_t op_id);
    ```

## 在不涉及批量传输的情况下取消RPC操作

在下面描述的情况下，将创建一个句柄并将其转发给目标。达到超时后，该句柄引用的当前操作将被取消。用户回调函数`forward_cb`以已取消的返回状态被推送到上下文的完成队列。

```c
static hg_return_t
forward_cb(const struct hg_cb_info *cb_info)
{
    if (cb_info->ret == HG_CANCELED) {
        /* Canceled */
    }
    if (cb_info->ret == HG_SUCCESS) {
        /* Sucessfully executed */
    }
}

int
main(void)
{
    hg_handle_t handle;

    /* Create new HG handle */
    HG_Create(hg_context, target, rpc_id, &hg_handle);
    /* Encode RPC */
    ...
    /* Forward call */
    HG_Forward(hg_handle, forward_cb, forward_cb_args, in_struct);
    /* No progress after timeout */
    ...
    /* Cancel operation */
    HG_Cancel(hg_handle);
    /* Trigger user callback */
    HG_Trigger(hg_context);
    /* Destroy handle */
    HG_Destroy(hg_handle);
}
```

## 取消涉及批量传输的RPC操作

在这种情况下，RPC操作还涉及由远程目标启动的批量传输。首先创建描述内存区域的批量句柄并将其注册到NIC。然后，此批量句柄与RPC参数一起被序列化，从而将内存区域公开/发布给目标。 RPC句柄类似地转发到目标。达到超时后，句柄将被取消。 用户回调函数`forward_cb`以已取消的返回状态被推送到上下文的完成队列。

值得注意的是，当取消RPC操作时，目标可能正在访问暴露的批量区域。
为确保防止未来的远程访问涉及批量传输的区域，必须另外销毁和取消批量传输（请注意，此保证仅在基础NA插件支持的情况下才有效）。批量句柄是输入数据结构的一部分，**可以在RPC句柄回调函数中进行引用，以分别取消远程批量操作**。

```c
static hg_return_t
forward_cb(const struct hg_cb_info *cb_info)
{
    if (cb_info->ret == HG_CANCELED) {
        /* Canceled */
    }
    if (cb_info->ret == HG_SUCCESS) {
        /* Sucessfully executed */
    }
}

int
main(void)
{
    hg_handle_t handle;
    hg_bulk_t bulk_handle;

    /* Create new HG bulk handle */
    HG_Bulk_create(hg_class, buffer_ptrs, buffer_sizes,
                   &hg_bulk_handle);
    /* Create new HG handle */
    HG_Create(hg_context, target, rpc_id, &hg_handle);
    /* Encode RPC and bulk handle */
    in_struct.bulk_handle = bulk_handle;
    ...
    /* Forward call */
    HG_Forward(hg_handle, forward_cb, forward_cb_args, &in_struct);
    /* No progress after timeout */
    ...
    /* Cancel operation */
    HG_Cancel(hg_handle);
    /* Trigger user callback */
    HG_Trigger(hg_context);
    /* Destroy HG handle */
    HG_Destroy(hg_handle);
    /* Destroy HG bulk handle */
    HG_Bulk_destroy(hg_bulk_handle);
}
```

## 取消批量操作

批量取消遵循类似的模型。但是，批量操作是由**RPC目标**而不是源发起的。批量操作由操作ID标识，当执行批量回调时，该ID将被释放。

```c
static hg_return_t
bulk_cb(const struct hg_cb_info *cb_info)
{
    if (cb_info->ret == HG_CANCELED) {
        /* Canceled */
    }
    if (cb_info->ret == HG_SUCCESS) {
        /* Sucessfully executed */
    }
}

static hg_return_t
rpc_cb(hg_handle_t handle)
{
    hg_bulk_t origin_handle, local_handle;

    /* Setup handles etc */
    ...
    /* Start the transfer */
    HG_bulk_transfer(context, bulk_cb, bulk_cb_args, HG_BULK_PULL,
                     origin_addr, origin_handle, origin_offset,
                     local_handle, local_offset, size, &op_id);
    /* No progress after timeout */
    ...
    /* Cancel operation */
    HG_Bulk_cancel(op_id);
    /* Trigger user callback */
    HG_Trigger(context);
}
```

## 添加超时支持

Mercury当前不支持操作超时，因为这将需要跟踪句柄/操作ID，这可能会导致Mercury层内不必要的开销。对于需要超时的情况，应用程序可以包装Mercury调用。下面的示例显示了如何使用HG请求仿真库发出带有超时的RPC调用。使用了`hg_request_t`类型。

```c
static hg_return_t
forward_cb(const struct hg_cb_info *cb_info)
{
    hg_request_t *request = (hg_request_t *) cb_info->arg;
    if (cb_info->ret == HG_CANCELED) {
        /* Canceled */
    }
    if (cb_info->ret == HG_SUCCESS) {
        /* Sucessfully executed */
    }
    hg_request_complete(request);
}

int
main()
{
    hg_handle_t handle;
    hg_request_t *request;

    /* Create new HG handle */
    HG_Create(hg_context, target, rpc_id, &hg_handle);
    /* Encode RPC */
    ...
    /* Create new request */
    request = hg_request_create(request_class);
    /* Forward call */
    HG_Forward(hg_handle, forward_cb, request, &in_struct);
    /* Wait for completion */
    hg_request_wait(request, timeout, &completed);
    if (!completed)
        HG_Cancel(hg_handle);
    /* Wait for completion */
    hg_request_wait(request, timeout, &completed);
    /* Destroy request */
    hg_request_destroy(request);
    /* Destroy handle */
    HG_Destroy(hg_handle);
}
```

## 支持NA中的取消

HG操作的取消只能通过首先在NA层支持取消来实现。通过以下调用支持取消：

```c
na_return_t NA_Cancel(na_class_t *na_class, na_context_t *context,
                      na_op_id_t op_id);
```

附加的`cancel`回调被添加到NA层，允许插件开发人员支持非阻塞操作的取消。这些包括：

- `NA_Addr_lookup()` (theoretically supported)
- `NA_Msg_send_unexpected()`, `NA_Msg_recv_unexpected()`
- `NA_Msg_send_expected()`, `NA_Msg_recv_expected()`
- `NA_Put()`, `NA_Get()`

NA操作的取消在内部进行，实际上是在内部插件取消成功完成后立即完成的（实际上可以立即取消，也可以不是立即取消）。成功取消NA操作后，与HG操作相关的内部回调将以`NA_CANCELED`返回状态放入NA上下文的完成队列中。

BMI/MPI和CCI插件支持取消。值得注意的是，BMI和MPI在双面操作之上模拟了单面操作（NA_Put（），NA_Get（）），并且尚不支持针对**这些操作**的取消。为了模拟这些操作，将发送/接收请求发送到远程目标，该远程目标在get的情况下可能发出send，而在put的情况下可能发出recv。取消这些操作意味着在目标处可能发出的send/recv操作被取消，这只能使用超时来完成（远程通知不是一种选择，因为调用者无法保证目标仍然存在）。

取消功能和支持取消的执行场景（即，不仅用于容错）仅取决于基础的NA插件及其支持取消正在进行的传输的功能。例如，当使用TCP和集合点协议时，BMI插件可能无法通过取消传输来恢复，因为TCP通道必须完全刷新才能进行进一步的通信。如果当时管道中还存在其他操作，则可能会造成危害，因为这些操作可能会因此失败。

## 结论与未来工作

增加mercury的取消是建设弹性服务的重要步骤，也是定义未来高级mercury功能（例如团体成员和发布/订阅服务）的必要组成部分，在这些功能中必须考虑容错性以防止集体失败。

这项工作的其余任务包括支持取消查找lookup操作。查找是一项可能涉及其他连接步骤的操作，具体取决于所涉及的NA插件，并且必须正确支持延迟的连接/异步查找执行方案，以支持取消这些操作。取消BMI / MPI单方面操作也可以得到改善，尽管目前还不是优先考虑的事情。
