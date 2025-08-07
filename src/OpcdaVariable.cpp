#include "OpcdaVariable.h"
#include <codecvt>
#include <locale>

void OpcdaVariable::setRawValue(const VARIANT& var) {
    rawVar_ = var;
    value_=decodeValue(var);
    timestamp_ = std::chrono::system_clock::now();
    quality_ = VarQuality::GOOD;
}

OpcdaVariable::ValueType OpcdaVariable::decodeValue(const VARIANT& var) {
    switch (var.vt) {
        case VT_BOOL:
            return var.boolVal ? true : false;
        case VT_UI1:
            return var.bVal;
        case VT_I1:
            return var.cVal;
        case VT_R8:
            return var.dblVal;
        case VT_I4:
            return static_cast<int32_t>(var.lVal);
        case VT_UI4:
            return static_cast<uint32_t>(var.ulVal);
        case VT_R4:
            return var.fltVal;
        case VT_I8:
            return var.llVal;
        case VT_UI8:
            return var.ullVal;
        case VT_I2:
            return var.iVal;
        case VT_UI2:
            return var.uiVal;
        case VT_BSTR:
            if (var.bstrVal) {
                const _bstr_t b(var.bstrVal);
                return std::string((const char*)b);
            }
            return {};
        default:
            return {};
    }
}
