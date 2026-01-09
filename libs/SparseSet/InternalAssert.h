#ifndef ENG_ASSERT
#define ENG_ASSERT

// https://github.com/MauroDeryckere/SparseSet 

#include <iostream>

namespace Internal
{
#define NOASSERTS

#ifndef NOASSERTS

    #ifndef NDEBUG
    #define DEBUG_ASSERT(condition, message) \
            do { \
                if (!(condition)) { \
                    std::cerr << "Assertion `" #condition "` failed in " << __FILE__ \
                              << " line " << __LINE__ << ": " << message << std::endl; \
                    std::abort(); \
                } \
            } while (false)
    #else
    #define DEBUG_ASSERT(condition, message) ((void)0)
    #endif

    #define ASSERT(condition, message) \
            do { \
                if (!(condition)) { \
                    std::cerr << "Assertion `" #condition "` failed in " << __FILE__ \
                                << " line " << __LINE__ << ": " << message << std::endl; \
                    std::abort(); \
                } \
            } while (false)
#else 
    #define DEBUG_ASSERT(condition, message) ((void)0)
    #define ASSERT(condition, message) ((void)0)

#endif
}

#endif