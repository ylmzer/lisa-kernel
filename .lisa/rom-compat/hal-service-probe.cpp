#include "BootControlClient.h"
#include <android/log.h>
#include <sys/system_properties.h>
#include <string.h>
#define REPORT(...) __android_log_print(ANDROID_LOG_INFO, "LISA_BOOTCTRL_TEST", __VA_ARGS__)
int main(int argc, char** argv) {
    const bool mark = argc == 2 && !strcmp(argv[1], "--mark-success");
    if (argc > 2 || (argc == 2 && !mark)) return 1;
    char complete[PROP_VALUE_MAX] = {}, suffix[PROP_VALUE_MAX] = {};
    __system_property_get("sys.boot_completed", complete);
    __system_property_get("ro.boot.slot_suffix", suffix);
    if (strcmp(complete, "1")) { REPORT("FAIL boot not complete"); return 2; }
    auto client = android::hal::BootControlClient::WaitForService();
    if (!client) { REPORT("FAIL no genuine boot HAL"); return 3; }
    const int version = int(client->GetVersion());
    const int slots = client->GetNumSlots(), slot = client->GetCurrentSlot();
    if (version != int(android::hal::BootControlVersion::BOOTCTL_V1_1) || slots != 2 || slot < 0 || slot >= slots) {
        REPORT("FAIL unexpected version=%d slots=%d slot=%d", version, slots, slot); return 4;
    }
    auto bootable = client->IsSlotBootable(slot), before = client->IsSlotMarkedSuccessful(slot);
    auto reported_suffix = client->GetSuffix(slot);
    if (!bootable.has_value() || !*bootable || !before.has_value() || reported_suffix != suffix) {
        REPORT("FAIL slot metadata mismatch"); return 5;
    }
    REPORT("CONNECTED HIDL1.1 slot=%d suffix=%s bootable=%d successful_before=%d merge=%d", slot, suffix, int(*bootable), int(*before), int(client->getSnapshotMergeStatus()));
    if (!mark) return 0;
    const auto result = client->MarkBootSuccessful();
    const auto after = client->IsSlotMarkedSuccessful(slot);
    if (!result.IsOk() || !after.has_value() || !*after) {
        REPORT("FAIL genuine markBootSuccessful result=%d message=%s readback=%d", result.IsOk(), result.errMsg.c_str(), after.has_value()?int(*after):-1); return 6;
    }
    REPORT("PASS genuine markBootSuccessful and successful-slot readback; slot=%d", slot);
    return 0;
}
