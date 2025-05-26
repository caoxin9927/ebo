#include <cstdio>

#include <unistd.h>
#include <sys/types.h>
#include <functional>
#include <memory.h>
#include <sys/socket.h>

class CFunctionBase
{
public:
    virtual ~CFunctionBase() {}
    virtual int operator()() = 0;
};


template<typename _FUNCTION_, typename... _ARGS_>
class CFunction :public CFunctionBase
{
public:
    CFunction(_FUNCTION_ func, _ARGS_... args)
        :m_binder(std::forward<_FUNCTION_>(func), std::forward<_ARGS_>(args)...)
    {
    }
    virtual ~CFunction() {}
    virtual int operator()() {//重载(),当使用CFunction()时，不会执行构造函数而是直接执行对应绑定的函数
        return m_binder();
    }
    decltype(std::bind(func, args...)) m_binder;
};
/*为什么父类子类析构函数都要定义成虚函数？
* 因为我们需要将基类指针用子类进行初始化，从而实现多态，那么当这个指针被销毁时不光要调用父类的析构函数，子类的析构函数
* 也要被析构，所以必须是父子类都调用析构函数才行，另外在多态内，父类指针指向了子类后，想使用子类的功能只能通过虚函数来实现
*/

class CProcess
{
public:
    CProcess()
    {
        m_func = NULL;
        memset(pipes, 0, sizeof(pipes));
    }
    template<typename _FUNCTION_, typename... _ARGS_ >
    int SetEntryFunction(_FUNCTION_ func,_ARGS_... args)//若干可变参数
    {
        m_func = new CFunction<_FUNCTION_, _ARGS_...>(func, args...);
        return 0;
    }
    int CreateSubProcess() {
        if (m_func == NULL)return -1;
        int ret = socketpair(AF_LOCAL, SOCK_STREAM, 0, pipes);
        if (ret == -1)return -2;
        pid_t pid = fork();
        if (pid == -1)return -3;
        if (pid == 0) {
            //子进程
            close(pipes[1]);//关闭掉写
            pipes[1] = 0;
            return (*m_func)();
        }
        //主进程
        close(pipes[0]);
        pipes[0] = 0;
        m_pid = pid;
        return 0;
    }
    int SendFD(int fd) {//主进程完成
        struct msghdr msg;
        iovec iov[2];
        iov[0].iov_base = (char*)"edoyun";
        iov[0].iov_len = 7;
        iov[1].iov_base = (char*)"jueding";
        iov[1].iov_len = 8;
        msg.msg_iov = iov;
        msg.msg_iovlen = 2;

        // 下面的数据，才是我们需要传递的。
        cmsghdr* cmsg = (cmsghdr*)calloc(1, CMSG_LEN(sizeof(int)));//使用calloc()分配一块内存，大小刚好能容纳一个控制消息头和一个int
        if (cmsg == NULL)return -1;
        cmsg->cmsg_len = CMSG_LEN(sizeof(int));
        cmsg->cmsg_level = SOL_SOCKET;//套接字级别的控制信息
        cmsg->cmsg_type = SCM_RIGHTS;//传递一个文件描述符
        *(int*)CMSG_DATA(cmsg) = fd;    //CMSG_DATA(cmsg)：返回控制消息数据部分的指针，对方收到控制消息后就可以拿到这个文件描述符
        msg.msg_control = cmsg;
        msg.msg_controllen = cmsg->cmsg_len;

        ssize_t ret = sendmsg(pipes[1], &msg, 0);
        free(cmsg);
        if (ret == -1) {
            return -2;
        }
        return 0;
    }
    int RecvFD(int& fd)
    {
        msghdr msg;
        iovec iov[2];
        char buf[][10] = { "","" };
        iov[0].iov_base = buf[0];
        iov[0].iov_len = sizeof(buf[0]);
        iov[1].iov_base = buf[1];
        iov[1].iov_len = sizeof(buf[1]);
        msg.msg_iov = iov;
        msg.msg_iovlen = 2;

        cmsghdr* cmsg = (cmsghdr*)calloc(1, CMSG_LEN(sizeof(int)));
        if (cmsg == NULL)return -1;
        cmsg->cmsg_len = CMSG_LEN(sizeof(int));
        cmsg->cmsg_level = SOL_SOCKET;
        cmsg->cmsg_type = SCM_RIGHTS;
        msg.msg_control = cmsg;
        msg.msg_controllen = CMSG_LEN(sizeof(int));
        ssize_t ret = recvmsg(pipes[0], &msg, 0);
        if (ret == -1) {
            free(cmsg);
            return -2;
        }
        fd = *(int*)CMSG_DATA(cmsg);
        return 0;
    }

private:
    CFunctionBase* m_func;
    pid_t m_pid;
    int pipes[2];

};
int CreateLogServer(CProcess* proc)
{
    return 0;
}

int CreateClientServer(CProcess* proc)
{
    return 0;
}

int main()
{
    CProcess proclog, proccliets;
    proclog.SetEntryFunction(CreateLogServer, &proclog);
    int ret = proclog.CreateSubProcess();
    proccliets.SetEntryFunction(CreateClientServer, &proccliets);
    ret = proccliets.CreateSubProcess();
    return 0;
}