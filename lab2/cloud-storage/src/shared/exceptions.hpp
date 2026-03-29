#pragma once

#include <stdexcept>
#include <string>

namespace disk::exceptions {

class MissingEnvVarException final : public std::runtime_error {
public:
    explicit MissingEnvVarException(const std::string& var_name)
        : std::runtime_error("Required environment variable '" + var_name + "' is not set"),
          var_name_(var_name) {}

    const std::string& VarName() const noexcept { return var_name_; }

private:
    std::string var_name_;
};

}  // namespace disk::exceptions
