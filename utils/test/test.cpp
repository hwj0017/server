#include <iostream>
#include <memory>

class Base
{
  public:
    ~Base()
    {
        std::cout << "Base析构\n";
    }
};

class Derived : public Base
{
  public:
    ~Derived()
    {
        std::cout << "Derived析构\n";
    }
};

int main()
{
    auto deleter = [](Base* ptr) { delete static_cast<Derived*>(ptr); };
    std::unique_ptr<Base, decltype(deleter)> ptr(new Derived(), deleter);
    return 0;
}