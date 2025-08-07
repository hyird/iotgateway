#include "OpcdaGroup.h"
#include <codecvt>
#include <iomanip>
#include <sstream>
#include <utility>
#include "OpcdaDevice.h"
#include "Logger.h"
#include "DataBuffer.h"
#include "DeviceManager.h"
#include "OpcdaVariable.h"
#include "OPCItem.h"
#define MESSAGE_PUMP_UNTIL(x)                                 \
while (!x){                                                   \
        MSG msg;                                              \
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))   \
    {                                                         \
            TranslateMessage(&msg);                           \
            DispatchMessage(&msg);                            \
    }                                                         \
        Sleep(1);                                             \
}

class CTransComplete final : public ITransactionComplete {
    std::string msg_ = "async operation completed";
public:
    virtual ~CTransComplete() = default;
    explicit CTransComplete(std::string msg) : msg_(std::move(msg)) {
    }
    void complete(CTransaction &) override {
    }
};

class CMyCallback final : public IAsyncDataCallback {
public:
    void OnDataChange(COPCGroup &group, COPCItemDataMap &changes) override {}
};

void OpcdaGroup::pollVariablesImpl(const std::shared_ptr<Device> &dev) {
    CMyCallback usrCallBack;
    auto opcdaDev = std::dynamic_pointer_cast<OpcdaDevice>(dev);
    group_ = opcdaDev->getOrCreateGroup(getId(), true, intervalMs_, 0.0, true, &usrCallBack);
    try {
        std::vector<std::wstring> newItemNames;
        std::vector<const Variable*> newVars;
        for (const auto& v : getVariables()) {
            if (opcItems_.find(v->getAddress()) == opcItems_.end()) {
                newItemNames.push_back(v->getAddress());
                newVars.push_back(v.get());
            }
        }
        if (!newItemNames.empty()) {
            std::vector<COPCItem*> itemsCreated;
            std::vector<HRESULT> errors;
            group_->addItems(newItemNames, itemsCreated, errors, true);
            for (size_t i = 0; i < itemsCreated.size(); ++i) {
                if (errors[i] != 0 || !itemsCreated[i]) {
                    GLOG_WARN("OpcdaGroup[" + getId() + "] 变量[" +
                        newVars[i]->getId() +
                        "] 添加到组失败 HRESULT=" + std::to_string(errors[i]));
                    continue;
                }
                opcItems_[newItemNames[i]] = itemsCreated[i];
            }
        }
        std::vector<COPCItem *> items;
        for (const auto &[addr, item]: opcItems_) items.push_back(item);
        auto *complete = new CTransComplete("OpcdaGroup[" + getId() + "] 异步读成功");
        CTransaction *transaction = nullptr;
        transaction = group_->readAsync(items, complete);
        MESSAGE_PUMP_UNTIL(transaction->isCompleted());
        for (const auto& [addr, item] : opcItems_) {
            if (const OPCItemData* asyncData = transaction->getItemValue(item); asyncData && !FAILED(asyncData->Error)) {
                if (auto it = std::find_if(getVariables().begin(), getVariables().end(),[&addr](const auto& v) { return v->getAddress() == addr; }); it != getVariables().end()) {
                    if (const auto var= std::dynamic_pointer_cast<OpcdaVariable>(*it)) {
                        if (asyncData->wQuality==192) {
                            var->setRawValue(asyncData->vDataValue);
                            DataBuffer::instance().set(
                                var->getVarid(),
                                var->getValue(),
                                std::chrono::system_clock::now(),
                                VarQuality::BAD
                            );
                        }else {
                            var->setQuality(VarQuality::BAD);
                            DataBuffer::instance().set(
                                var->getVarid(),
                                var->getValue(),
                                std::chrono::system_clock::now(),
                                VarQuality::BAD
                            );
                        }
                    }else {
                        var->setQuality(VarQuality::BAD);
                        DataBuffer::instance().set(
                            var->getVarid(),
                            var->getValue(),
                            std::chrono::system_clock::now(),
                            VarQuality::BAD
                        );
                    }
                }
            }
        }
        group_->deleteTransaction(transaction);
        delete complete;
    } catch (const OPCException &ex) {
        const std::wstring &wreason = ex.reasonString();
        const std::string reasonUtf8 = OpcdaDevice::wstring_to_utf8(wreason);
        GLOG_ERROR("OpcdaGroup[" + getId() + "] 异步读取失败: " + reasonUtf8);
        opcdaDev->reportError("OpcdaGroup[" + getId() + "] 异步读取失败: " + reasonUtf8);
    } catch (const std::exception &ex) {
        GLOG_ERROR("OpcdaGroup[" + getId() + "] 异步读取异常: " + std::string(ex.what()));
    } catch (...) {
        GLOG_ERROR("OpcdaGroup[" + getId() + "] 异步读取异常: Unknown exception");
    }
}
