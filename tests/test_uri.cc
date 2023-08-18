#include <iostream>

#include "marco/uri.h"

int main(int argc, char** argv) {
    // marco::Uri::ptr uri =
    // marco::Uri::Create("https://www.sylar.top/test/uri?id=100&name=sylar#frg");
    marco::Uri::ptr uri
    =
    marco::Uri::Create("http://admin@www.sylar.top/test/中文/uri?id=100&name=sylar&vv=中文#frg中文");
    // marco::Uri::ptr uri = marco::Uri::Create("http://admin@www.sylar.top");
    // sylar::Uri::ptr uri = sylar::Uri::Create("http://www.sylar.top/test/uri");
    std::cout << uri->toString() << std::endl;
    auto addr = uri->createAddress();
    std::cout << (*addr).toString() << std::endl;
    return 0;
}
