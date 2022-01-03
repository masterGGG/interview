#include <iostream>

#pragma once

template<typename T>
class Singleton {
public:
    ~Singleton() {}

    template<typename... Args>//可变参数函数模板
    static T *getInstance(Args&& ...args) {//对参数的右值引用
        if (instance == nullptr) {
            instance = new T(std::forward<Args>(args)...);//转发具体的初始化函数参数
        }

        return instance;
    }
private:
    Singleton() {}
    static volatile T *instance;
};

template<class T> T* Singleton<T>::instance = nullptr;