## 客户端流程：

开一百个线程，每个线程都发hello world!然后等待回包。 按照TLV的格式，MsgID、MsgHead和MsgData，
> 发数据：先发MsgID，偏移下数据，发MsgHead，更新偏移，发MsgData

![在这里插入图片描述](https://i-blog.csdnimg.cn/direct/4ae600a373b84b76a8b76b599793dc3f.png)
>收数据：接受MsgID（2B）长度的数据，接受MsgHead（2B）长度的数据，接受MsgLeng长度的数据

![在这里插入图片描述](https://i-blog.csdnimg.cn/direct/58c95d11f4f248e49d24809ebef7ff72.png)
## 服务端流程：

 - 采用的是多io_context多线程的模式异步协程处理模式 主线程初始化一个io_context，异步监听连接事件
   ![在这里插入图片描述](https://i-blog.csdnimg.cn/direct/1e88c08c851347b58e262f604247dc61.png)
 - 有连接上来后触发回调函数HandleAccept，HandleAccept主要是新建一个Session，这个Session会启动协程去接受数据，且这里Session和io_context都是新的，每来一个连接都会在ServicePool里面拿一个出来。

   
 

 - 读数据流程： 读取MsgID（2B）长度的数据，读取MsgHead（2B）长度的数据，读取MsgLeng长度的数据

![在这里插入图片描述](https://i-blog.csdnimg.cn/direct/b91af5f6b84f4f12a1b3a7e039eb2fa6.png)

 - 写数据流程：之前异步读完之后不会立马去写，而是先投放到一个逻辑队列里面去统一处理，在投递时会调用LogicSystem的构造函数，LogicSystem()会注册一个map，map里存放的回调函数，当回调函数获得锁会去读取队列中的数据返回消息给客户端，这里每个socket对应的io_context是独立的，所以不用再封装一层strand队列去单独控制处理回调函数。

>  发送数据流程： 发送MsgID（2B）长度的数据，发送MsgHead（2B）长度的数据，发送MsgLeng长度的数据

![在这里插入图片描述](https://i-blog.csdnimg.cn/direct/aee8667b35864267827172430b3cd4b2.png)
### 大概的一些细节

 - MsgNode是一个基类，_cur_len是表示当前发送的长度，_total_len是代表总长度，_data是数据首地址，用首地址+偏移来定位数据，**但_total_len这里比较宽泛，正对发送和接受节点有不一样的意义**，我们在构造函数中把成员变量使用列表初始化的方式初始化，数据后面用'\0'结尾。
 - 
   数据发送是依靠jsoncpp来序列化的，为什么选择jsoncpp呢？相比于Protobuf确实性能有些许下降，但优势在于数据是可视的，可读性较好，而且一般情况下**Protobuf适用于服务之间传递传输消息，服务端和客户端之间用json就足够**

![在这里插入图片描述](https://i-blog.csdnimg.cn/direct/defead9ae6334b88a9b3c2fcf536aba0.png)

然后单独有RecvNode和SendNode，这里声明LogicSystem的友元是因为消息id设置成了私有，
![在这里插入图片描述](https://i-blog.csdnimg.cn/direct/deeaa66a55a949c79d3e056ec4d70360.png)
RecvNode和SendNode构造函数会有些不一样，接受节点因为头部和数据分开接受的，所以_total_len=max_len,发送节点就需要包括头部的信息，所以是_total_len=max_len + HEAD_TOTAL_LEN
![在这里插入图片描述](https://i-blog.csdnimg.cn/direct/9dce719ba2d84678b40a6d38f6608985.png)

## 最后结果

最终效果：客户端开一百个线程，每个线程发500条消息，每个线程之间间隔10毫秒怕电脑顶不住，实验室工作站太拉了（老师有点抠门）
![在这里插入图片描述](https://i-blog.csdnimg.cn/direct/428d754be98f4e82b5a6704d4f0e3d2d.png)
![在这里插入图片描述](https://i-blog.csdnimg.cn/direct/3f3251c26fc64d9bae6071ac5eabd21a.png)


计算一下用时结果，100x500=50k条消息，std::cout好像是很费时的，这么看好像性能还不错诶。
![在这里插入图片描述](https://i-blog.csdnimg.cn/direct/6b4464b2cc784e65a6e146355796b627.png)
![在这里插入图片描述](https://i-blog.csdnimg.cn/direct/72898fb9d4ec41369906ef4e6a7c1833.png)
