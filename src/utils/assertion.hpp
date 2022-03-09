#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "errors.hpp"
#include "logger2.hpp"

namespace ubiquant {

// failure handling option
/*
 * if set UBIQUANT_LOGGER_THROW_ON_FAILURE
 * ---assertion will throw fail message
 * else
 * ---assertion will abort the program
 */
//#define UBIQUANT_LOGGER_FAIL_METHOD
#define UBIQUANT_LOGGER_THROW_ON_FAILURE

#ifdef UBIQUANT_LOGGER_THROW_ON_FAILURE
  #define UBIQUANT_LOGGER_FAIL_METHOD(status_code) \
      throw(ubiquant::UbiquantException(status_code))
#else
  #define UBIQUANT_LOGGER_FAIL_METHOD(status_code) abort()
#endif

/*
 * actual check
 * use __buildin_expect to provide the complier with branch prediction
 */
// check the condition
#define CHECK(condition)                                                       \
  do {                                                                         \
    if (__builtin_expect(!(condition), 0)) {                                   \
      logstream(LOG_ERROR) << "Assertion: " << __FILE__ << "(" << __func__     \
                           << ":" << __LINE__ << ")"                           \
                           << ": \'" << #condition << "\' failed" << LOG_endl; \
      UBIQUANT_LOGGER_FAIL_METHOD(ubiquant::UNKNOWN_ERROR);                                \
    }                                                                          \
  } while (0)

// check the val1 op val2
#define UBIQUANT_CHECK_OP(op, val1, val2)                                            \
  do {                                                                      \
    const decltype(val1) _CHECK_OP_v1_ = (decltype(val1))val1;                  \
    const decltype(val2) _CHECK_OP_v2_ = (decltype(val2))val2;                  \
    if (__builtin_expect(!((_CHECK_OP_v1_)op(decltype(val1))(_CHECK_OP_v2_)), \
                         0)) {                                              \
      logstream(LOG_ERROR) << "Assertion: " << __FILE__ << "(" << __func__  \
                           << ":" << __LINE__ << ")"                        \
                           << ": \'" << #val1 << " " << #op << " " << #val2 \
                           << " [ " << val1 << " " << #op << " " << val2    \
                           << " ]\'"                                        \
                           << " failed" << LOG_endl;                        \
      UBIQUANT_LOGGER_FAIL_METHOD(ubiquant::UNKNOWN_ERROR);                             \
    }                                                                       \
  } while (0)

#define CHECK_EQ(val1, val2) UBIQUANT_CHECK_OP(==, val1, val2)
#define CHECK_NE(val1, val2) UBIQUANT_CHECK_OP(!=, val1, val2)
#define CHECK_LE(val1, val2) UBIQUANT_CHECK_OP(<=, val1, val2)
#define CHECK_LT(val1, val2) UBIQUANT_CHECK_OP(<, val1, val2)
#define CHECK_GE(val1, val2) UBIQUANT_CHECK_OP(>=, val1, val2)
#define CHECK_GT(val1, val2) UBIQUANT_CHECK_OP(>, val1, val2)

// condition assert
#define ASSERT_TRUE(cond) CHECK(cond)
#define ASSERT_FALSE(cond) CHECK(!(cond))
// adapt to the original ubiquant assert
#define ASSERT(cond) CHECK(cond)

#define ASSERT_EQ(val1, val2) CHECK_EQ(val1, val2)
#define ASSERT_NE(val1, val2) CHECK_NE(val1, val2)
#define ASSERT_LE(val1, val2) CHECK_LE(val1, val2)
#define ASSERT_LT(val1, val2) CHECK_LT(val1, val2)
#define ASSERT_GE(val1, val2) CHECK_GE(val1, val2)
#define ASSERT_GT(val1, val2) CHECK_GT(val1, val2)

// string equal
#define ASSERT_STREQ(a, b) CHECK(strcmp(a, b) == 0)

// check the condition if wrong print out the message of variable parameters in
// fmt
#define ASSERT_MSG(condition, fmt, ...)                                        \
  do {                                                                         \
    if (__builtin_expect(!(condition), 0)) {                                   \
      logstream(LOG_ERROR) << "Assertion: " << __FILE__ << "(" << __func__     \
                           << ":" << __LINE__ << ")"                           \
                           << ": \'" << #condition << "\' failed" << LOG_endl; \
      logger(LOG_ERROR, fmt, ##__VA_ARGS__);                                   \
      UBIQUANT_LOGGER_FAIL_METHOD(fmt);                                          \
    }                                                                          \
  } while (0)

#define ASSERT_ERROR_CODE(condition, error_code)                     \
    do {                                                               \
        if (__builtin_expect(!(condition), 0)) {                       \
            logstream(LOG_DEBUG)                                       \
                << "Assertion: " << __FILE__ << "(" << __func__ << ":" \
                << __LINE__ << ")"                                     \
                << ": \'" << #condition << "\' failed" << LOG_endl;    \
            UBIQUANT_LOGGER_FAIL_METHOD(error_code);                    \
        }                                                              \
    } while (0)

} // namespace ubiquant