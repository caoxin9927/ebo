#pragma once
#include <unistd.h>
#include <sys/types.h>
#include <functional>

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
    decltype(std::bind(std::declval<_FUNCTION_>(), std::declval<_ARGS_>()...)) m_binder;
};
/*为什么父类子类析构函数都要定义成虚函数？
* 因为我们需要将基类指针用子类进行初始化，从而实现多态，那么当这个指针被销毁时不光要调用父类的析构函数，子类的析构函数
* 也要被析构，所以必须是父子类都调用析构函数才行，另外在多态内，父类指针指向了子类后，想使用子类的功能只能通过虚函数来实现
*/