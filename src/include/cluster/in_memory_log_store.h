// /************************************************************************
// Copyright 2017-2019 eBay Inc.
// Author/Developer(s): Jung-Sang Ahn

// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     https://www.apache.org/licenses/LICENSE-2.0

// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
// **************************************************************************/

// #pragma once

// #include <atomic>
// #include <map>
// #include <mutex>
// #include "libnuraft/event_awaiter.h"
// #include "libnuraft/internal_timer.hxx"
// #include "libnuraft/log_store.hxx"
// #include "libnuraft/pp_util.hxx"
// #include "database/vector_database.h" // 包含 VectorDatabase 类的头文件
// namespace vectordb {

// class raft_server;

// class InmemLogStore : public nuraft::log_store {
//  public:
//   explicit InmemLogStore(VectorDatabase* vector_database);

//   ~InmemLogStore() override;

//   // NOLINTNEXTLINE
//   __nocopy__(InmemLogStore);

//  public:
//   auto next_slot() const -> ulong override;

//   auto start_index() const -> ulong override;

//   auto last_entry() const -> nuraft::ptr<nuraft::log_entry> override;

//   auto append(nuraft::ptr<nuraft::log_entry> &entry) -> ulong override;

//   void write_at(ulong index, nuraft::ptr<nuraft::log_entry> &entry) override;

//   auto log_entries(ulong start, ulong end) -> nuraft::ptr<std::vector<nuraft::ptr<nuraft::log_entry>>> override;

//   auto log_entries_ext(nuraft::ulong start, nuraft::ulong end, nuraft::int64 batch_size_hint_in_bytes = 0)
//       -> nuraft::ptr<std::vector<nuraft::ptr<nuraft::log_entry>>> override;

//   auto entry_at(ulong index) -> nuraft::ptr<nuraft::log_entry> override;

//   auto term_at(ulong index) -> ulong override;

//   auto pack(ulong index, nuraft::int32 cnt) -> nuraft::ptr<nuraft::buffer> override;

//   void apply_pack(ulong index, nuraft::buffer &pack) override;

//   auto compact(ulong last_log_index) -> bool override;

//   auto flush() -> bool override;

//   void Close();

//   auto last_durable_index() -> ulong override;

//   void SetDiskDelay(raft_server *raft, size_t delay_ms);

//  private:
//   static auto MakeClone(const nuraft::ptr<nuraft::log_entry> &entry) -> nuraft::ptr<nuraft::log_entry>;

//   void DiskEmulLoop();

//   /**
//    * Map of <log index, log data>.
//    */
//   std::map<ulong, nuraft::ptr<nuraft::log_entry>> logs_;

//   /**
//    * Lock for `logs_`.
//    */
//   mutable std::mutex logs_lock_;

//   /**
//    * The index of the first log.
//    */
//   std::atomic<ulong> start_idx_;

//   /**
//    * Backward pointer to Raft server.
//    */
//   raft_server *raft_server_bwd_pointer_;

//   // Testing purpose --------------- BEGIN

//   /**
//    * If non-zero, this log store will emulate the disk write delay.
//    */
//   std::atomic<size_t> disk_emul_delay_;

//   /**
//    * Map of <timestamp, log index>, emulating logs that is being written to disk.
//    * Log index will be regarded as "durable" after the corresponding timestamp.
//    */
//   std::map<uint64_t, uint64_t> disk_emul_logs_being_written_;

//   /**
//    * Thread that will update `last_durable_index_` and call
//    * `notify_log_append_completion` at proper time.
//    */
//   std::unique_ptr<std::thread> disk_emul_thread_;

//   /**
//    * Flag to terminate the thread.
//    */
//   std::atomic<bool> disk_emul_thread_stop_signal_;

//   /**
//    * Event awaiter that emulates disk delay.
//    */
//   EventAwaiter disk_emul_ea_;

//   /**
//    * Last written log index.
//    */
//   std::atomic<uint64_t> disk_emul_last_durable_index_;

//    VectorDatabase* vector_database_; // 添加一个 VectorDatabase 指针成员变量
//   // Testing purpose --------------- END
// };

// }  // namespace vectordb
