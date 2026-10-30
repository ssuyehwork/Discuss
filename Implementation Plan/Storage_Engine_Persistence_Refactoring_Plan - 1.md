Storage Engine & Metadata Persistence Architecture Refactoring Plan (Step 1: QuarkMetaJsonStore & QuarkMetaJson)
1. Audit Conclusions & Benefit/Risk Analysis
1.1 Fact Audit of Discussion & Code Analysis
Following code archaeology on origin/main (commit 30b3f1b), the identified architectural risks and issues are confirmed:

Write Amplification & Debounce Stall: scheduleFlush in QuarkMetaJsonStore uses a resetting debounce timer (300ms/500ms). Under continuous background processing (e.g. 1000+ images in media pipelines), frequent updates repeatedly reset the timer, preventing disk flushes during extraction (data loss risk on crash). When it eventually flushes, it serializes whole JSON files in Indented format, causing severe write amplification.
Unstable file_id & Primary Key Inconsistency: Missing file_id / meta_file_id fields during serialization/deserialization cause new UUIDs to be generated on every save, violating the key binding between .QuarkMeta.json sidecars and global.db.
Missing Fields: Encryption fields (encryptSalt, encryptIv, encryptVerifyHash) and folderId are missing in itemToEntry / folderToEntry / entryToItem / entryToFolder, leading to data loss across restarts.
Cyclic Dependencies & Synchronous Flushes: Static wrappers in QuarkMetaJson.cpp directly depend on MetadataManager and MetaMemoryCache while forcing synchronous flushAllDirtyBuffers(true) calls.
1.2 Benefit & Risk Analysis
Benefits:
Eliminate Data Loss Risk: Bounded max flush delay (3s max latency) guarantees persistence under heavy writes; failed saves re-flag dirty buffers.
Reduce Write Amplification: Compact JSON format + background thread pool serialization.
Stable Primary Keys: Complete file_id and encryption field persistence.
Decoupled Architecture: Removes cyclic dependencies on MetadataManager.
Mitigation:
Thread-pool serialization uses copy-on-write snapshot isolation to prevent memory spikes.
2. Refactoring Scope (Step 1: QuarkMetaJsonStore & QuarkMetaJson)
2.1 Target Files
src/meta/QuarkMetaJson.h
src/meta/QuarkMetaJson.cpp
src/meta/QuarkMetaJsonStore.h
src/meta/QuarkMetaJsonStore.cpp
2.2 Detailed Modifications
(1) src/meta/QuarkMetaJson.h & QuarkMetaJson.cpp
Field Completeness: Parse and write encryptSalt, encryptIv, encryptVerifyHash, and folderId in itemToEntry, folderToEntry, entryToItem, and entryToFolder.
Stable file_id: Preserve existing file_id on read/write; generate UUID only once when missing.
Remove Cyclic Dependencies: Purge static wrapper functions (QuarkMetaJson::updateItemMeta, QuarkMetaJson::renameItem, etc.) that trigger synchronous flushAllDirtyBuffers(true). Route all updates through QuarkMetaJsonStore.
(2) src/meta/QuarkMetaJsonStore.h & QuarkMetaJsonStore.cpp
Dual Timer Debounce & Max Delay: m_flushTimer (300ms Debounce) + m_maxFlushTimer (3000ms Max Delay) to guarantee execution within 3s.
Folder Batch Update API: Add updateFolderItemsBatch to allow pipeline tasks to mutate multiple items in a single pass with minimal lock contention.
Flush Failure Guard: If saveTicket fails (e.g., file lock), re-mark the buffer as dirty instead of dropping changes.
In-Memory Subtree Renaming: Replace full-disk copies during folder renames with $O(1)$ in-memory re-keying.
3. Verification & Self-Audit Plan
Compile and verify public header contracts for QuarkMetaJson and QuarkMetaJsonStore.
Verify sidecar JSON file serialization, file_id stability, and field completeness.
Verify max-delay flush and failed-write recovery logic.