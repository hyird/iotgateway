#pragma once
#include <map>
#include "Group.h"
class COPCItem;
class COPCGroup;
class OpcdaDevice;
class CTransComplete;
class CMyCallback;
class OpcdaGroup final : public Group {
public:
    using Group::Group;
    void pollVariablesImpl(const std::shared_ptr<Device> &dev) override ;
private:
    mutable std::shared_ptr<COPCGroup> group_;
    mutable std::map<std::wstring, COPCItem*> opcItems_;
};
