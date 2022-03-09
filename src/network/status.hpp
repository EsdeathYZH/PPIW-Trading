#pragma once

#include <cstring>
#include <iosfwd>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

namespace ubiquant {

enum MSG_CODE : uint32_t { INFO_RPC = 0, SPARQL_RPC = 1, STRING_RPC = 2, EXIT_RPC = 3 };

enum StatusCode : unsigned char {
  kOK = 0,
  kInvalid = 1,
  kIOError = 2,
  kAssertionFailed = 3,
  kConnectionFailed = 4,
  kConnectionError = 5,

  kUnknownError = 255
};

class Status {
public:
    Status(): code(StatusCode::kOK) {}
    Status(StatusCode code, const std::string& msg): code(code), msg(msg) {}

    /// Return a success status
    inline static Status OK() { return Status(); }

    /// Return an error status for invalid data (for example a string that
    /// fails parsing).
    static Status Invalid() { return Status(StatusCode::kInvalid, ""); }

    /// Return an error status for invalid data, with user specified error
    /// message.
    static Status Invalid(std::string const& message) {
        return Status(StatusCode::kInvalid, message);
    }

    /// Return an error status for IO errors (e.g. Failed to open or read from a
    /// file).
    static Status IOError(const std::string& msg = "") {
        return Status(StatusCode::kIOError, msg);
    }

    /// Return an error status when the condition assertion is false.
    static Status AssertionFailed(std::string const& condition) {
        return Status(StatusCode::kAssertionFailed, condition);
    }

    /// Return an error when client failed to connect to server.
    static Status ConnectionFailed(std::string const& message = "") {
        return Status(StatusCode::kConnectionFailed,
                    "Failed to connect to exchange: " + message);
    }

    /// Return an error when client losts connection to server.
    static Status ConnectionError(std::string const& message = "") {
        return Status(StatusCode::kConnectionError, message);
    }

    /// Return true iff the status indicates success.
    bool ok() const { return code == StatusCode::kOK; }

    StatusCode get_code() { return this->code; }

    std::string get_msg() { return this->msg; }

private:
    StatusCode code;
    std::string msg;
};

} // namespace ubiquant