# Bounded snapshot consolidation

Observed X captures reached the 64-file quota at 53345/51145 captured events. The native v2 parser reads the count from offset264, iterates count records, and dynamically grows its file descriptor vector (iorapd addresses0x6eb0,0x6fac,0x7074). The 64-file limit was local prototype policy, not the transport layout.

This candidate uses 1024 file records and a separate 8MiB aggregate bitmap allocation limit. It preserves 65536 raw events, the per-file2GiB bound, 10MiB maximum wire bytes,16MiB total cached blobs,8cache slots, all exact metadata/staleness checks and real errors. Worst serialized payload280+1024*296+8388608=8691992bytes <10MiB. Record metadata and8KiB lookup keys are separately bounded heap allocations; no enlarged kernel stack array. All allocations remain in process-context consolidation, not capture callbacks. Duplicate render-thread collect semantics and PTE code are unchanged.

The new limits are resource budgets, not a measured claim that the app uses1024files. Successful snapshots log actual file/event/bitmap counts. Both record and bitmap budget failures stay errors and do not produce a partial cache. Device training/replay acceptance is still required.
