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
    virtual int operator()() {//����(),��ʹ��CFunction()ʱ������ִ�й��캯������ֱ��ִ�ж�Ӧ�󶨵ĺ���
        return m_binder();
    }
    decltype(std::bind(std::declval<_FUNCTION_>(), std::declval<_ARGS_>()...)) m_binder;
};
/*Ϊʲô������������������Ҫ������麯����
* ��Ϊ������Ҫ������ָ����������г�ʼ�����Ӷ�ʵ�ֶ�̬����ô�����ָ�뱻����ʱ����Ҫ���ø���������������������������
* ҲҪ�����������Ա����Ǹ����඼���������������У������ڶ�̬�ڣ�����ָ��ָ�����������ʹ������Ĺ���ֻ��ͨ���麯����ʵ��
*/