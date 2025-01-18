

# asio多线程模型IOServicePool
之前单线程模式的并发量不高，为了提升网络的并发处理效率，那肯定得加入多线程。使用多线程有两种方式：

 - 启动多个线程， 每个线程管理一个io_context。
 -  启动一个io_context,被多个线程共享
 
这里先写第一种多线程多io_context的，每个线程管理独立的io_context。

> 这个方式有以下特点：
>  - 每一个io_context在不同的线程里面，那么同一个socket会在同一个io_context里注册，回调函数也会单独被一个线程调用，每次触发回调函数都是在一个线程里面，所以不会有线程安全问题，网络io层面并发是安全的。
>  -    对于不同的socket，回调函数触发就不确定了。有可能触发的是同一个线程，也可能是不同的线程。我们这里采用逻辑队列的方式来解决安全问题。


## 实现
差不多就是一个线程池，然后每一个线程都初始化一个io_context,这样就可以并发处理不同的io_context读事件了。
**.h文件**
```cpp
class AsioIOServicePool:public Singleton<AsioIOServicePool>
{
    friend Singleton<AsioIOServicePool>;
public:
    using IOService = boost::asio::io_context;
    using Work = boost::asio::io_context::work;
    using WorkPtr = std::unique_ptr<Work>;
    ~AsioIOServicePool();
    AsioIOServicePool(const AsioIOServicePool&) = delete;
    AsioIOServicePool& operator=(const AsioIOServicePool&) = delete;
    // 使用 round-robin 的方式返回一个 io_service
    boost::asio::io_context& GetIOService();
    void Stop();
private:
    AsioIOServicePool(std::size_t size = std::thread::hardware_concurrency());
    std::vector<IOService> _ioServices;
    std::vector<WorkPtr> _works;
    std::vector<std::thread> _threads;
    std::size_t   _nextIOService;
};
```
**构造函数**

```cpp
AsioIOServicePool::AsioIOServicePool(std::size_t size):_ioServices(size),
_works(size), _nextIOService(0){
    for (std::size_t i = 0; i < size; ++i) {
        _works[i] = std::unique_ptr<Work>(new Work(_ioServices[i]));
    }
    //遍历多个ioservice，创建多个线程，每个线程内部启动ioservice
    for (std::size_t i = 0; i < _ioServices.size(); ++i) {
        _threads.emplace_back([this, i]() {
            _ioServices[i].run();
            });
    }
}
```
上面这个是右值赋值，所以是允许的
这里为什么不用push_back,push_back相比于emplace_back多了一个赋值拷贝操作，增加了资源消耗。
使用emplace_back插入一个操作，这样可以快速构造线程，其实就是回调了thread内部的构造函数。

**获取service**

```cpp
boost::asio::io_context& AsioIOServicePool::GetIOService() {
    auto& service = _ioServices[_nextIOService++];
    if (_nextIOService == _ioServices.size()) {
        _nextIOService = 0;
    }
    return service;
}
```

**Stop函数**

保证每个线程安全退出后才能让AsioIOServicePool停止。
```cpp
void AsioIOServicePool::Stop(){
    for (auto& work : _works) {
        work.reset();
    }
    for (auto& t : _threads) {
        t.join();
    }
}
```

## 主函数

```cpp
        auto pool = AsioIOServicePool::GetInstance();
        boost::asio::io_context  io_context;
        boost::asio::signal_set signals(io_context, SIGINT, SIGTERM);
        signals.async_wait([&io_context,pool](auto, auto) {
            io_context.stop();
            pool->Stop();
            });
        CServer s(io_context, 10086);
        io_context.run();
```
