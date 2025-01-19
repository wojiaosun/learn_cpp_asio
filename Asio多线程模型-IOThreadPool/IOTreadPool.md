# Asio多线程模型-ThreadPool
使用多线程有两种方式：

 - 启动多个线程， 每个线程管理一个io_context。
 -  启动一个io_context,被多个线程共享

这次写的是第二种，我们只初始化一个io_context用来监听服务器的读写事件，包括新连接到来监听也用这个io_context。让iocontext.run()在多个线程中调用，这样回调就会被不同的线程触发。

## 具体实现
实现上和IOServicPool类似，不一样的点在于io_context只有一个。

```cpp
AsioThreadPool::AsioThreadPool(int threadNum ):_work(new boost::asio::io_context::work(_service)){
    for (int i = 0; i < threadNum; ++i) {
        _threads.emplace_back([this]() {
            _service.run();
            });
    }
}
boost::asio::io_context& AsioThreadPool::GetIOService() {
    return _service;
}
```
这里构造函数实现了一个线程池，线程池里每个线程都会运行_service.run()函数，_service.run()内部就是从iocp（windows）或者epoll（linux）获取就绪描述符和绑定的回调函数，进而调用回调函数。但是因为只有一个io_context，所以会有不同线程调用同一个socket的回调函数情况。

windows和linux流程类似，只是接口换了
**iocp**

> 1 创建完成端口(iocp)对象
2 创建一个或多个工作线程，在完成端口上执行并处理投递到完成端口上的I/O请求
3 Socket关联iocp对象，在Socket上投递网络事件
4 工作线程调用GetQueuedCompletionStatus函数获取完成通知封包，取得事件信息并进行处理

**epoll**

> 1 调用epoll_creat在内核中创建一张epoll表
2 开辟一片包含n个epoll_event大小的连续空间
3 将要监听的socket注册到epoll表里
4 调用epoll_wait，传入之前我们开辟的连续空间，epoll_wait返回就绪的epoll_event列表，epoll会将就绪的socket信息写入我们之前开辟的连续空间


## 隐患
单个IO_Context，多线程的模式，对于同一个socket就绪后，触发的回调函数可能在不同的线程里面，如果两次会掉的间隔时间短，那么就会导致从同一个socket缓冲区读取数据，从而会造成数据混乱。
![在这里插入图片描述](https://i-blog.csdnimg.cn/direct/edbb16bb5fcd4b6583d4d89fc7d86c18.png)
类似上面的过程，解决方式可以是对队列加锁，那么多线程就没有意义了，这里是利用asio内部的strand特性，对于每次多线程触发的回调函数不直接调用，而是将回调函数先投递给strand队列，再由strand队列统一串行来调用。

**构造函数**

```cpp
CSession::CSession(boost::asio::io_context& io_context, CServer* server):
    _socket(io_context), _server(server), _b_close(false),
    _b_head_parse(false), _strand(io_context.get_executor()){
    boost::uuids::uuid  a_uuid = boost::uuids::random_generator()();
    _uuid = boost::uuids::to_string(a_uuid);
    _recv_head_node = make_shared<MsgNode>(HEAD_TOTAL_LEN);
}
```

这里将std::bind再通过 boost::asio::bind_executor（）再封装了一次。
```cpp
void CSession::Start(){
    ::memset(_data, 0, MAX_LENGTH);
    _socket.async_read_some(boost::asio::buffer(_data, MAX_LENGTH),
        boost::asio::bind_executor(_strand, std::bind(&CSession::HandleRead, this,
            std::placeholders::_1, std::placeholders::_2, SharedSelf())));
}
```
