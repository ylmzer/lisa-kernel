# Writable input candidate (not device accepted)

Actual successful capture/replay on135f8882c still intermittently fails final metadata validation for a changing Crashlytics userlog. Exclude events from FMODE_WRITE handles or inodes with positive i_writecount before constructing a record, with excluded_writable_events. No package/path-specific blacklist. Raw overflow remains checked first; all excluded input returns ENODATA. Existing accepted records still face index/path/mtime/ctime/size/unlink validation. Writers starting after classification can still cause ESTALE; do not claim this eliminates every race or provides an atomic filesystem snapshot. Capture hotpath unchanged.

Repeated START still returns EBUSY, preserving active capture and ownership. Added owner/same-app/UID/PID diagnostics to distinguish render reattachment from competing app sessions before changing lifecycle behavior. No idempotent success or timeout reset introduced.
