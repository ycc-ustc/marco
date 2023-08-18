#ifndef __MARCO_NONCOPYABLE_H__
#define __MARCO_NONCOPYABLE_H__

namespace marco {
class Noncopyable {
public:
    Noncopyable() = default;
    ~Noncopyable() = default;
    // 拷贝构造函数(禁用)
    Noncopyable(const Noncopyable&) = delete;
    // 赋值函数(禁用)
    Noncopyable& operator=(const Noncopyable&) = delete;

};
}

#endif