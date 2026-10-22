IORAP file snapshot input eligibility

Observed two genuine X cold starts rejected a Firebase sessionDataStore.data inode already deleted at snapshot consolidation (f2fs, stagepath, inode105476/22842). A file preread record must have a reopenable path. At consolidation entry, unlinked file events are explicitly ineligible and counted as excluded_unlinked_events. No record is created for those events. Raw capture overflow still rejects before filtering, all live file range/size/path/metadata and final unlink checks stay fail-closed. A file unlinked after its record was accepted still rejects the whole snapshot. All-unlinked nonempty traces return ENODATA, never success with empty training data. No hotpath change or file IO occurs in the capture callback.

This is an explicitly logged input classification, not discarded capture overflow or suppressed ESTALE. Runtime app training/replay remains required.
