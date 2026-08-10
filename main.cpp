#include <optional>
#include <print>

// trivial example to test CI
class Base {
public:
    virtual void print() const {
        std::println("Base::print()");
    }
    virtual ~Base() = default;
};

class Derived : public Base {
    void print() const override {
        std::println("Derived::print()");
    }
};

int main() {
    {
        Base* b_obj = new Base {};
        std::println("Invoking Base::print():");
        b_obj->print();
        delete b_obj;
    }
    {
        Base* d_obj = new Derived {};
        std::println("Invoking Derived::print():");
        d_obj->print();
	delete d_obj;
    }
    return 0;
}
