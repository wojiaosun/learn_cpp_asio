# asio协程实现并发服务器
**定义协程函数**
协程函数的返回类型为boost::asio::awaitable<T>，其中T是协程的返回值类型。例如，一个返回类型为void的协程可以这样定义：
```cpp
boost::asio::awaitable<void> my_coroutine() {
    // 协程逻辑
}
```

**co_await挂起协程**
co_await是C++20协程的关键字，用于挂起当前协程，直到等待的操作完成。例如，异步读取操作可以这样写：
```cpp
std::size_t n = co_await socket.async_read_some(boost::asio::buffer(data), boost::asio::use_awaitable);
```
**启动协程**

```cpp
boost::asio::co_spawn(ioc, my_coroutine, boost::asio::detached);
```


主要涉及上面的操作，协程是C++20改进提出的，在visual studio中需要改一下版本支持。



## 定义 `echo` 协程函数

```cpp
awaitable<void> echo(tcp::socket socket) {
    try {
        char data[1024];
        for (;;) {
            std::size_t n = co_await socket.async_read_some(boost::asio::buffer(data), use_awaitable);
            if (n == 0) break; // Connection closed cleanly by peer.
            co_await boost::asio::async_write(socket, boost::asio::buffer(data, n), use_awaitable);
        }
    } catch (std::exception& e) {
        std::cout << "Exception: " << e.what() << std::endl;
    }
}
```

- `echo` 是一个返回类型为 `awaitable<void>` 的协程，用于处理与客户端的通信。
- 它进入一个无限循环，等待从客户端读取数据。
- 每当读取到数据时（`n > 0`），它会将相同的数据写回到客户端，实现回显功能。
- 如果读取到的数据长度为0 (`n == 0`)，表示客户端关闭了连接，则退出循环。
- 如果在读写过程中遇到异常，会在捕获异常后打印错误信息。

##  定义 `listener` 协程函数

```cpp
awaitable<void> listener() {
    auto executor = co_await boost::asio::this_coro::executor;
    tcp::acceptor acceptor(executor, {tcp::v4(), 8888});
    while (true) {
        tcp::socket socket = co_await acceptor.async_accept(use_awaitable);
        co_spawn(executor, echo(std::move(socket)), detached);
    }
}
```

- `listener` 是一个返回类型为 `awaitable<void>` 的协程，负责监听新的客户端连接。
- 它首先获取当前协程的执行器（`executor`），然后创建一个TCP接受者（`acceptor`），绑定到IPv4地址的8888端口上。
- 接下来进入一个无限循环，等待新的连接请求。
- 当有新连接到来时，它会异步地接受这个连接，并启动一个新的 `echo` 协程来处理该连接。
- 使用 `co_spawn` 来启动新的 `echo` 协程，并传入 `detached` 表示不关心协程的结果。

##  `main` 函数：程序入口点

```cpp
int main() {
    try {
        boost::asio::io_context ioc;
        co_spawn(ioc, listener, detached);
        ioc.run();
    } catch (const std::exception& e) {
        std::cout << e.what() << std::endl;
    }
    return 0;
}
```

- 在 `main` 函数中，首先创建了一个 `io_context` 对象 `ioc`，它是事件循环的核心，所有的异步操作都依赖于它。
- 然后调用 `co_spawn` 启动 `listener` 协程，同样使用 `detached` 标志。
- 最后调用 `ioc.run()` 启动事件循环，开始处理所有挂起的异步操作。
- 如果在事件循环中发生任何未捕获的异常，它们会被 `main` 函数中的 `catch` 块捕获并打印出来。

## 流程总结

- **初始化**：程序启动时，`main` 函数创建 `io_context` 并启动 `listener` 协程。
- **监听连接**：`listener` 协程在指定端口监听新的连接请求。
- **处理连接**：每当有一个新的连接到来时，`listener` 协程就会启动一个新的 `echo` 协程来处理这个连接。
- **回显数据**：每个 `echo` 协程会接收来自客户端的数据并将其发送回去，直到连接被关闭。
- **异常处理**：在整个过程中，无论是 `listener` 还是 `echo` 协程，都会进行适当的异常处理，确保程序能够稳定运行。
- **事件循环**：`io_context` 的 `run` 方法会一直运行，直到没有更多的工作需要处理或遇到异常为止。

通过这种方式，服务器可以高效地并发处理多个客户端连接，同时保持代码的简洁性和可读性。
