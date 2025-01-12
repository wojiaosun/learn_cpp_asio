# Asio异步读写demo
参考官方案例，主要就是读完调用读回调函数，读回调函数才处理完后又调用写回调函数。
这里就是回调有点绕，需要多看看
## Session类
Session类主要是处理客户端收发消息的类，这里暂时不考虑粘包问题，只是为了学习asio异步编程

```cpp
class Session
{
public:
    Session(boost::asio::io_context& ioc):_socket(ioc){
    }
    tcp::socket& Socket() {
        return _socket;
    }
    void Start();
private:
	//处理读回调函数
    void handle_read(const boost::system::error_code & error, size_t bytes_transfered);
    //处理写回调函数
    void handle_write(const boost::system::error_code& error);
    tcp::socket _socket;
    enum {max_length = 1024};
    char _data[max_length];
};
```
_data用于接收客户端传递的数据，读完后再清空
_socket为单独处理客户端读写的socket
 handle_read和handle_write分别为读回调函数和写回调函数
 

```cpp
void Session::Start(){
    memset(_data, 0, max_length);
    _socket.async_read_some(boost::asio::buffer(_data, max_length),
        std::bind(&Session::handle_read, this, placeholders::_1,
            placeholders::_2)
    );
}
```
start方法监听消息，客户端发送消息后，触发handle_read

```cpp
void Session::handle_read(const boost::system::error_code& error, size_t bytes_transfered) {
    if (!error) {
        cout << "server receive data is " << _data << endl;
        boost::asio::async_write(_socket, boost::asio::buffer(_data, bytes_transfered), 
            std::bind(&Session::handle_write, this, placeholders::_1));
    }
    else {
        delete this;
    }
}
```

handle_read函数内将收到的数据发送给对端，当发送完成后触发handle_write回调函数。

```cpp
void Session::handle_write(const boost::system::error_code& error) {
    if (!error) {
        memset(_data, 0, max_length);
        _socket.async_read_some(boost::asio::buffer(_data, max_length), std::bind(&Session::handle_read,
            this, placeholders::_1, placeholders::_2));
    }
    else {
        delete this;
    }
}
```

handle_write函数内又一次监听了读事件，如果对端有数据发送过来则触发handle_read，我们再将收到的数据发回去。

## Server类
Server类为服务器接收连接的管理类

```cpp
class Server {
public:
    Server(boost::asio::io_context& ioc, short port);
private:
    void start_accept();
    void handle_accept(Session* new_session, const boost::system::error_code& error);
    boost::asio::io_context& _ioc;
    tcp::acceptor _acceptor;
};
```

start_accept将要接收连接的acceptor绑定到服务上，其内部就是将accpeptor对应的socket描述符绑定到epoll或iocp模型上，实现事件驱动。
handle_accept为新连接到来后触发的回调函数。
下面是具体实现

```cpp
Server::Server(boost::asio::io_context& ioc, short port) :_ioc(ioc),
_acceptor(ioc, tcp::endpoint(tcp::v4(), port)) {
    start_accept();
}
void Server::start_accept() {
    Session* new_session = new Session(_ioc);
    _acceptor.async_accept(new_session->Socket(),
        std::bind(&Server::handle_accept, this, new_session, placeholders::_1));
}
void Server::handle_accept(Session* new_session, const boost::system::error_code& error) {
    if (!error) {
        new_session->Start();
    }
    else {
        delete new_session;
    }
    start_accept();
}
```
