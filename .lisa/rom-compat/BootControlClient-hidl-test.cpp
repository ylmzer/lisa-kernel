/*
 * Copyright (C) 2022 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <BootControlClient.h>


#include <android-base/logging.h>


#include <android/hardware/boot/1.0/IBootControl.h>
#include <android/hardware/boot/1.1/IBootControl.h>

#include "utils/StrongPointer.h"

// Compatibility with VNDK33 libc++ before the nullptr stream overload.
static std::ostream& operator<<(std::ostream& stream, std::nullptr_t) {
    return stream << "nullptr";
}

#define CONCAT(x, y) x##y

#define LOG_NDK_STATUS(x)                                                                   \
    do {                                                                                    \
        const auto CONCAT(status, __COUNTER__) = x;                                         \
        if (!CONCAT(status, __COUNTER__).isOk()) {                                          \
            LOG(ERROR) << #x << " failed " << CONCAT(status, __COUNTER__).getDescription(); \
        }                                                                                   \
    } while (0)

using aidl::android::hardware::boot::MergeStatus;

#define TEST_OP(_x, _y, op)                                                            \
    do {                                                                               \
        const auto& x = _x;                                                            \
        const auto& y = _y;                                                            \
        if (!(x op y)) {                                                               \
            LOG(ERROR) << #_x " " #op " " #_y << " failed: " << x << " " #op " " << y; \
            return {};                                                                 \
        }                                                                              \
    } while (0)
#define TEST_NE(_x, _y) TEST_OP(_x, _y, !=)

std::ostream& operator<<(std::ostream& os, MergeStatus status) {
    switch (status) {
        case MergeStatus::NONE:
            os << "MergeStatus::NONE";
            break;
        case MergeStatus::UNKNOWN:
            os << "MergeStatus::UNKNOWN";
            break;
        case MergeStatus::SNAPSHOTTED:
            os << "MergeStatus::SNAPSHOTTED";
            break;
        case MergeStatus::MERGING:
            os << "MergeStatus::MERGING";
            break;
        case MergeStatus::CANCELLED:
            os << "MergeStatus::CANCELLED";
            break;
        default:
            os << static_cast<int>(status);
            break;
    }
    return os;
}

namespace android::hal {
using namespace android::hardware::boot;

class BootControlClientHIDL final : public BootControlClient {
  public:
    BootControlClientHIDL(android::sp<V1_0::IBootControl> module_v1,
                          android::sp<V1_1::IBootControl> module_v1_1)
        : module_v1_(module_v1), module_v1_1_(module_v1_1) {
        CHECK(module_v1_ != nullptr);
    }
    BootControlVersion GetVersion() const override {
        if (module_v1_1_ != nullptr) {
            return BootControlVersion::BOOTCTL_V1_1;
        } else {
            return BootControlVersion::BOOTCTL_V1_0;
        }
    }
    int32_t GetNumSlots() const override {
        const auto ret = module_v1_->getNumberSlots();
        if (!ret.isOk()) {
            LOG(ERROR) << __FUNCTION__ << " failed " << ret.description();
        }
        return ret.withDefault(-1);
    }

    int32_t GetCurrentSlot() const override {
        const auto ret = module_v1_->getCurrentSlot();
        if (!ret.isOk()) {
            LOG(ERROR) << __FUNCTION__ << " failed " << ret.description();
        }
        return ret.withDefault(-1);
    }

    std::string GetSuffix(int32_t slot) const override {
        std::string suffix;
        const auto ret = module_v1_->getSuffix(
                slot,
                [&](const ::android::hardware::hidl_string& slotSuffix) { suffix = slotSuffix; });
        if (!ret.isOk()) {
            LOG(ERROR) << __FUNCTION__ << "(" << slot << ")" << " failed " << ret.description();
        }
        return suffix;
    }

    std::optional<bool> IsSlotBootable(int32_t slot) const override {
        const auto ret = module_v1_->isSlotBootable(slot);
        if (!ret.isOk()) {
            LOG(ERROR) << __FUNCTION__ << "(" << slot << ")" << " failed " << ret.description();
            return {};
        }
        const auto bool_result = ret.withDefault(V1_0::BoolResult::INVALID_SLOT);
        if (bool_result == V1_0::BoolResult::INVALID_SLOT) {
            return {};
        }
        return bool_result == V1_0::BoolResult::TRUE;
    }

    CommandResult MarkSlotUnbootable(int32_t slot) override {
        CommandResult result{false, "HAL callback not received"};
        const auto ret =
                module_v1_->setSlotAsUnbootable(slot, [&](const V1_0::CommandResult& error) {
                    result.success = error.success;
                    result.errMsg = error.errMsg;
                });
        if (!ret.isOk()) {
            LOG(ERROR) << __FUNCTION__ << "(" << slot << ")" << " failed " << ret.description();
        }
        return ret.isOk() ? result : CommandResult{false, ret.description()};
    }

    CommandResult SetActiveBootSlot(int32_t slot) override {
        CommandResult result{false, "HAL callback not received"};
        const auto ret = module_v1_->setActiveBootSlot(slot, [&](const V1_0::CommandResult& error) {
            result.success = error.success;
            result.errMsg = error.errMsg;
        });
        if (!ret.isOk()) {
            LOG(ERROR) << __FUNCTION__ << "(" << slot << ")" << " failed " << ret.description();
        }
        return ret.isOk() ? result : CommandResult{false, ret.description()};
    }

    CommandResult MarkBootSuccessful() override {
        CommandResult result{false, "HAL callback not received"};
        const auto ret = module_v1_->markBootSuccessful([&](const V1_0::CommandResult& error) {
            result.success = error.success;
            result.errMsg = error.errMsg;
        });
        if (!ret.isOk()) {
            LOG(ERROR) << __FUNCTION__ << " failed " << ret.description();
        }
        return ret.isOk() ? result : CommandResult{false, ret.description()};
    }

    std::optional<bool> IsSlotMarkedSuccessful(int32_t slot) const override {
        const auto ret = module_v1_->isSlotMarkedSuccessful(slot);
        if (!ret.isOk()) {
            LOG(ERROR) << __FUNCTION__ << "(" << slot << ")" << " failed " << ret.description();
            return {};
        }
        const auto bool_result = ret.withDefault(V1_0::BoolResult::INVALID_SLOT);
        if (bool_result == V1_0::BoolResult::INVALID_SLOT) {
            return {};
        }
        return bool_result == V1_0::BoolResult::TRUE;
    }

    MergeStatus getSnapshotMergeStatus() const override {
        if (module_v1_1_ == nullptr) {
            LOG(ERROR) << __FUNCTION__ << " is unsupported, requires at least boot v1.1";
            return MergeStatus::UNKNOWN;
        }
        const auto ret = module_v1_1_->getSnapshotMergeStatus();
        if (!ret.isOk()) {
            LOG(ERROR) << __FUNCTION__ << " failed " << ret.description();
        }
        return static_cast<MergeStatus>(
                ret.withDefault(static_cast<V1_1::MergeStatus>(MergeStatus::UNKNOWN)));
    }

    CommandResult SetSnapshotMergeStatus(MergeStatus merge_status) override {
        if (module_v1_1_ == nullptr) {
            return {.success = false,
                    .errMsg = "setSnapshotMergeStatus is unsupported, requires at least boot v1.1"};
        }
        const auto ret =
                module_v1_1_->setSnapshotMergeStatus(static_cast<V1_1::MergeStatus>(merge_status));
        if (!ret.isOk()) {
            LOG(ERROR) << __FUNCTION__ << "(" << merge_status << ")" << " failed "
                       << ret.description();
        }
        return {.success = ret.isOk(), .errMsg = ret.description()};
    }

    int32_t GetActiveBootSlot() const override {
        LOG(ERROR) << "GetActiveBootSlot unsupported in this HIDL1.1-only test client";
        return -1;
    }

  private:
    android::sp<V1_0::IBootControl> module_v1_;
    android::sp<V1_1::IBootControl> module_v1_1_;
};

std::unique_ptr<BootControlClient> BootControlClient::WaitForService() {
    android::sp<V1_0::IBootControl> v1_0_module;
    android::sp<V1_1::IBootControl> v1_1_module;
    v1_0_module = V1_0::IBootControl::getService();
    if (v1_0_module == nullptr) {
        LOG(ERROR) << "Error getting bootctrl v1.0 module.";
        return nullptr;
    }
    v1_1_module = V1_1::IBootControl::castFrom(v1_0_module);
    if (v1_1_module != nullptr) {
        LOG(INFO) << "Using HIDL version 1.1 of IBootControl";
    } else {
        LOG(INFO) << "Using HIDL version 1.0 of IBootControl";
    }

    return std::make_unique<BootControlClientHIDL>(v1_0_module, v1_1_module);
}

}  // namespace android::hal
