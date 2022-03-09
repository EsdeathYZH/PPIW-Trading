#pragma once

#include <exception>

namespace ubiquant {

#define ERR_MSG(n) (err_msgs[n])

// error begin with 1, no error can be 0
enum {
    SUCCESS, 
    UNKNOWN_ERROR,
    SYNTAX_ERROR,
    UNKNOWN_PATTERN,
    ATTR_DISABLE,
    NO_REQUIRED_VAR,
    UNSUPPORT_UNION,
    OBJ_ERROR,
    VERTEX_INVALID,
    UNKNOWN_SUB,
    SETTING_ERROR,
    FIRST_PATTERN_ERROR,
    UNKNOWN_FILTER,
    FILE_NOT_FOUND,
    ERROR_LAST
};

// error_messages
static const char *err_msgs[ERROR_LAST] = {
    "Everythong is ok",
    "Something wrong happened",
    "Something wrong in the query syntax, fail to parse!",
    "Unsupported triple pattern.",
    "MUST enable attribute support!",
    "NO required variables!",
    "Unsupport UNION on attribute results",
    "Object should not be an index",
    "Subject or object is not valid",
    "Tripple pattern should not start from unknown subject.",
    "You may change SETTING files to avoid this error. (e.g. global.hpp/config/...)",
    "Const_X_X or index_X_X must be the first pattern.",
    "Unsupported filter type.",
    "Query file not found."};

// An exception
struct UbiquantException : public std::exception {
   private:
    int status_code;
    const char *message;

   public:
    /// Constructor
    UbiquantException(const int status_code) : status_code(status_code) {}
    UbiquantException(const char *message)
        : message(message), status_code(UNKNOWN_ERROR) {}

    const char *what() const throw() { return ERR_MSG(status_code); }

    int code() { return status_code; }

    /// Destructor
    ~UbiquantException() {}
};

} // namespace ubiquant