
#ifndef TuiPointer_h
#define TuiPointer_h

#include <memory>

// A shared pointer. Manages the instance count/lifetime of data
template<typename T>
using TuiPointer = std::shared_ptr<T>;

// A base class for objects that can create TuiPointer instances to themselves
// Useful to convert "this" to a TuiPointer
template<typename T>
using TuiPointerSource = std::enable_shared_from_this<T>;

// Helper functions
namespace Tui
{
    // Helper to create a TuiPointer instance from constructor arguments
    template<typename T, typename... Args>
    inline TuiPointer<T> create(Args&&... args)
    {
        return std::make_shared<T>(std::forward<Args>(args)...);
    }

    // Helper to cast a TuiPointer<T> instance to another type
    template<typename T, typename U>
    inline TuiPointer<T> cast(const TuiPointer<U>& ptr)
    {
        return std::static_pointer_cast<T>(ptr);
    }
}

#endif // TuiPointer_h