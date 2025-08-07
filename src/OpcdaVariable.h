#pragma once
#include "Variable.h"
#include <comdef.h>

class OpcdaVariable final : public Variable {
public:
    using Variable::Variable;
    void setRawValue(const VARIANT& var);

    static ValueType decodeValue(const VARIANT& var);
    [[nodiscard]] VARIANT getRawVariant() const { return rawVar_; }

private:
    VARIANT rawVar_{};
};
