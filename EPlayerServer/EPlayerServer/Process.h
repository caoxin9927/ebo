#pragma once
#include <memory.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include "Function.h"


class CProcess
{
public:
    CProcess()
    {
        m_func = NULL;
        memset(pipes, 0, sizeof(pipes));
    }
    template<typename _FUNCTION_, typename... _ARGS_ >
    int SetEntryFunction(_FUNCTION_ func, _ARGS_... args)//若干可变参数
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
            ret = (*m_func)();
            exit(0);
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
        char buf[2][10] = { "edoyun","jueding" };
        iov[0].iov_base = buf[0];
        iov[0].iov_len = sizeof(buf[0]);
        iov[1].iov_base = buf[1];
        iov[1].iov_len = sizeof(buf[1]);
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
            free(cmsg);//给cmsg分配的是一块堆空间，需要手动释放
            return -2;
        }
        fd = *(int*)CMSG_DATA(cmsg);
        free(cmsg);
        return 0;
    }
    static int SwitchDeamon() {
        pid_t ret = fork();
        if (ret == -1)return -1;
        if (ret > 0)exit(0);//主进程到此为止
        //子进程内容如下
        ret = setsid();
        if (ret == -1)return -2;//失败，则返回
        ret = fork();
        if (ret == -1)return -3;
        if (ret > 0)exit(0);//子进程到此为止
        //孙进程的内容如下，进入守护状态
        for (int i = 0; i < 3; i++) close(i);
        umask(0);
        signal(SIGCHLD, SIG_IGN);
        return 0;
    }
private:
    CFunctionBase* m_func;
    pid_t m_pid;
    int pipes[2];

};
