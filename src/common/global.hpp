#pragma once

#include <map>
#include <memory>

#include "utils/assertion.hpp"

namespace ubiquant {

template<typename T>
class Global final {
public:
    static T* Get() { return *GetPPtr(); }
    static void SetAllocated(T* val) { *GetPPtr() = val; }
    template<typename... Args>
    static T* New(Args&&... args) {
        if(Get() != nullptr) {
            Delete();
        }
        ASSERT(Get() == nullptr);
        T* ptr = new T(std::forward<Args>(args)...);
        *GetPPtr() = ptr;
        return ptr;
    }
    static void Delete() {
        if (Get() != nullptr) {
            delete Get();
            *GetPPtr() = nullptr;
        }
    }

private:
    static T** GetPPtr() {
        static T* ptr = nullptr;
        return &ptr;
    }
};

}  // namespace ubiquant
