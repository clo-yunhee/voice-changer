#ifndef VCEXCEPTION_H
#define VCEXCEPTION_H

#include <exception>
#include <stdexcept>

namespace vc {

class Exception : public std::exception {
public:
    Exception(const char *what) {
        snprintf(message, MAX_MESSAGE_LENGTH, "%s", what);
    }

    Exception(const char *what, const char *error) {
        snprintf(message, MAX_MESSAGE_LENGTH, "%s: %s", what, error);
    }

    const char *what() const noexcept final {
        return message;
    }

private:
    static constexpr size_t MAX_MESSAGE_LENGTH = 256;
    char message[MAX_MESSAGE_LENGTH];
};

}

#endif // VCEXCEPTION_H