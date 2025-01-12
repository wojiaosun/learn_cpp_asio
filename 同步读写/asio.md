# 复习一下网络编程基本流程
服务端：
1.socket--创建socket对象
2.bind--绑定本机ip+port
3.listen--监听连接，有的话就建立连接
4.accept--在创建一个socket对象给其收发消息，原因是现实中服务端都是面对多个客户端，那么为了区分各个客户端，则每个客户端都需再分配一个socket对象进行收发消息
5.read、write--收发消息
客户端：
1.socket--创建socket对象
2.connect--根据服务端ip+port，发起建立连接请求
3.read、write--收发消息
![在这里插入图片描述](https://i-blog.csdnimg.cn/direct/1f1894cf85a4490f9e8e27352c549b3d.png)

> 那么asio网络编程和socket编程有啥区别？socket编程通常是同步io阻塞的，单机支持的qps可以说是很低，asio支持异步io操作，基于事件驱动，通过触发回调函数来执行其他任务，提高内在的工作效率。

在asio中其实类似socket，通过创建一个终端节点endpoint（用来通信的端对端节点，ip+port），服务端依靠endpoint实现通信。
## 终端节点创建
客户端
```cpp
std::string raw_ip_address = "127.0.0.1";
    unsigned short port_num = 3333;
    boost::system::error_code ec;
    asio::ip::address ip_address =
        asio::ip::address::from_string(raw_ip_address, ec);
    if (ec.value() != 0) {
        std::cout
            << "Failed to parse the IP address. Error code = "
            << ec.value() << ". Message: " << ec.message();
        return ec.value();
    }
    asio::ip::tcp::endpoint ep(ip_address, port_num);
```
服务端直接绑定ip+port就行

```cpp
 unsigned short port_num = 3333;
    asio::ip::address ip_address = asio::ip::address_v6::any();
    asio::ip::tcp::endpoint ep(ip_address, port_num);
```
## 创建socket
创建socket分为4步，创建上下文iocontext，选择协议，生成socket，打开socket

```cpp
    //创建上下文
    asio::io_context  ios;
    // Step 2. Creating an object of 'tcp' class representing
    asio::ip::tcp protocol = asio::ip::tcp::v4();
    // Step 3. Instantiating an active TCP socket object.
    asio::ip::tcp::socket sock(ios);
    boost::system::error_code ec;
    // Step 4. Opening the socket.
    sock.open(protocol, ec);
    if (ec.value() != 0) {
        std::cout
            << "Failed to open the socket! Error code = "
            << ec.value() << ". Message: " << ec.message();
        return ec.value();
    }
```
服务端还需要创建一个acceptor的socket
## 绑定acceptor

```cpp
unsigned short port_num = 3333;
    asio::ip::tcp::endpoint ep(asio::ip::address_v4::any(),
        port_num);
    asio::io_context  ios;
    asio::ip::tcp::acceptor acceptor(ios, ep.protocol());
    boost::system::error_code ec;
    acceptor.bind(ep, ec);
```
后面内容和socket编程类似，都是acceptor充当前单接待员，前台短暂接待顾客，后续分配给新的服务员（new socket），注意的是，需要保证服务员完成服务后，前台才能闭店（异步操作时需要等所有子线程完成时才能结束主线程）。


## 一些思考
asio网络库有自己的buffer数据结构，就是接受和发送数据时的缓冲区。
boost::asio提供了asio::mutable_buffer 和 asio::const_buffer这两个结构，可变长和固定长度。
常量用于读，变量用于写。
可以把buffer理解为一个vector里面存储的都是一个地址，每个地址指向了length+data。
