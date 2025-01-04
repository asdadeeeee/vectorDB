/************************************************************************
Copyright 2017-2019 eBay Inc.
Author/Developer(s): Jung-Sang Ahn

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    https://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
**************************************************************************/

#include "cluster/in_memory_log_store.h"

#include "libnuraft/nuraft.hxx"
#include "logger/logger.h"

#include <cassert>
#include <memory>

namespace vectordb {

InmemLogStore::InmemLogStore(VectorDatabase *vector_database)
    : start_idx_(vector_database->GetStartIndexId() + 1),
      raft_server_bwd_pointer_(nullptr),
      disk_emul_delay_(0),
      disk_emul_thread_(nullptr),
      disk_emul_thread_stop_signal_(false),
      disk_emul_last_durable_index_(0),
      vector_database_(vector_database) {
  // Dummy entry for index 0.
  nuraft::ptr<nuraft::buffer> buf = nuraft::buffer::alloc(sz_ulong);
  logs_[0] = nuraft::cs_new<nuraft::log_entry>(0, buf);
}

InmemLogStore::~InmemLogStore() {
  if (disk_emul_thread_) {
    disk_emul_thread_stop_signal_ = true;
    disk_emul_ea_.invoke();
    if (disk_emul_thread_->joinable()) {
      disk_emul_thread_->join();
    }
  }
}

auto InmemLogStore::MakeClone(const nuraft::ptr<nuraft::log_entry> &entry) -> nuraft::ptr<nuraft::log_entry> {
  // NOTE:
  //   Timestamp is used only when `replicate_log_timestamp_` option is on.
  //   Otherwise, log store does not need to store or load it.
  nuraft::ptr<nuraft::log_entry> clone = nuraft::cs_new<nuraft::log_entry>(
      entry->get_term(), nuraft::buffer::clone(entry->get_buf()), entry->get_val_type(), entry->get_timestamp());
  return clone;
}

auto InmemLogStore::next_slot() const -> nuraft::ulong {
  std::lock_guard<std::mutex> l(logs_lock_);
  // Exclude the dummy entry.
  return start_idx_ + logs_.size() - 1;
}

auto InmemLogStore::start_index() const -> nuraft::ulong { return start_idx_; }

auto InmemLogStore::last_entry() const -> nuraft::ptr<nuraft::log_entry> {
  ulong next_idx = next_slot();
  std::lock_guard<std::mutex> l(logs_lock_);
  auto entry = logs_.find(next_idx - 1);
  if (entry == logs_.end()) {
    entry = logs_.find(0);
  }

  return MakeClone(entry->second);
}

auto InmemLogStore::append(nuraft::ptr<nuraft::log_entry> &entry) -> nuraft::ulong {
  nuraft::ptr<nuraft::log_entry> clone = MakeClone(entry);

  std::lock_guard<std::mutex> l(logs_lock_);
  size_t idx = start_idx_ + logs_.size() - 1;
  logs_[idx] = clone;
  if (entry->get_val_type() == nuraft::log_val_type::app_log) {
    nuraft::buffer &data = clone->get_buf();
    std::string content(reinterpret_cast<const char *>(data.data() + data.pos() + sizeof(int)),
                        data.size() - sizeof(int));

    global_logger->debug("Append app logs {}, content: {}, value type {}", idx, content,
                         "nuraft::log_val_type::app_log");  //  添加打印日志

    vector_database_->writeWALLogWithID(idx, content);
  } else {
    nuraft::buffer &data = clone->get_buf();
    std::string content(reinterpret_cast<const char *>(data.data() + data.pos()), data.size());
    global_logger->debug("Append other logs {}, content: {}, value type {}", idx, content,
                         static_cast<int>(entry->get_val_type()));  // 添加打印日志
  }
  if (disk_emul_delay_ != 0U) {
    uint64_t cur_time = nuraft::timer_helper::get_timeofday_us();
    disk_emul_logs_being_written_[cur_time + disk_emul_delay_ * 1000] = idx;
    disk_emul_ea_.invoke();
  }

  return idx;
}

void InmemLogStore::write_at(ulong index, nuraft::ptr<nuraft::log_entry> &entry) {
  nuraft::ptr<nuraft::log_entry> clone = MakeClone(entry);

  // Discard all logs equal to or greater than `index.
  std::lock_guard<std::mutex> l(logs_lock_);
  auto itr = logs_.lower_bound(index);
  while (itr != logs_.end()) {
    itr = logs_.erase(itr);
  }
  logs_[index] = clone;

  if (disk_emul_delay_ != 0U) {
    uint64_t cur_time = nuraft::timer_helper::get_timeofday_us();
    disk_emul_logs_being_written_[cur_time + disk_emul_delay_ * 1000] = index;

    // Remove entries greater than `index`.
    auto entry = disk_emul_logs_being_written_.begin();
    while (entry != disk_emul_logs_being_written_.end()) {
      if (entry->second > index) {
        entry = disk_emul_logs_being_written_.erase(entry);
      } else {
        entry++;
      }
    }
    disk_emul_ea_.invoke();
  }
}

auto InmemLogStore::log_entries(nuraft::ulong start, nuraft::ulong end)
    -> nuraft::ptr<std::vector<nuraft::ptr<nuraft::log_entry>>> {
  nuraft::ptr<std::vector<nuraft::ptr<nuraft::log_entry>>> ret =
      nuraft::cs_new<std::vector<nuraft::ptr<nuraft::log_entry>>>();

  ret->resize(end - start);
  nuraft::ulong cc = 0;
  for (nuraft::ulong ii = start; ii < end; ++ii) {
    nuraft::ptr<nuraft::log_entry> src = nullptr;
    {
      std::lock_guard<std::mutex> l(logs_lock_);
      auto entry = logs_.find(ii);
      if (entry == logs_.end()) {
        entry = logs_.find(0);
        assert(0);
      }
      src = entry->second;
    }
    (*ret)[cc++] = MakeClone(src);
  }
  return ret;
}

auto InmemLogStore::log_entries_ext(nuraft::ulong start, nuraft::ulong end, nuraft::int64 batch_size_hint_in_bytes)
    -> nuraft::ptr<std::vector<nuraft::ptr<nuraft::log_entry>>> {
  nuraft::ptr<std::vector<nuraft::ptr<nuraft::log_entry>>> ret =
      nuraft::cs_new<std::vector<nuraft::ptr<nuraft::log_entry>>>();

  if (batch_size_hint_in_bytes < 0) {
    return ret;
  }

  size_t accum_size = 0;
  for (nuraft::ulong ii = start; ii < end; ++ii) {
    nuraft::ptr<nuraft::log_entry> src = nullptr;
    {
      std::lock_guard<std::mutex> l(logs_lock_);
      auto entry = logs_.find(ii);
      if (entry == logs_.end()) {
        entry = logs_.find(0);
        assert(0);
      }
      src = entry->second;
    }
    ret->push_back(MakeClone(src));
    accum_size += src->get_buf().size();
    if ((batch_size_hint_in_bytes != 0) && accum_size >= static_cast<ulong>(batch_size_hint_in_bytes)) {
      break;
    }
  }
  return ret;
}

auto InmemLogStore::entry_at(ulong index) -> nuraft::ptr<nuraft::log_entry> {
  nuraft::ptr<nuraft::log_entry> src = nullptr;
  {
    std::lock_guard<std::mutex> l(logs_lock_);
    auto entry = logs_.find(index);
    if (entry == logs_.end()) {
      entry = logs_.find(0);
    }
    src = entry->second;
  }
  return MakeClone(src);
}

auto InmemLogStore::term_at(ulong index) -> ulong {
  ulong term = 0;
  {
    std::lock_guard<std::mutex> l(logs_lock_);
    auto entry = logs_.find(index);
    if (entry == logs_.end()) {
      entry = logs_.find(0);
    }
    term = entry->second->get_term();
  }
  return term;
}

auto InmemLogStore::pack(nuraft::ulong index, nuraft::int32 cnt) -> nuraft::ptr<nuraft::buffer> {
  std::vector<nuraft::ptr<nuraft::buffer>> logs;

  size_t size_total = 0;
  for (ulong ii = index; ii < index + cnt; ++ii) {
    nuraft::ptr<nuraft::log_entry> le = nullptr;
    {
      std::lock_guard<std::mutex> l(logs_lock_);
      le = logs_[ii];
    }
    assert(le.get());
    nuraft::ptr<nuraft::buffer> buf = le->serialize();
    size_total += buf->size();
    logs.push_back(buf);
  }

  nuraft::ptr<nuraft::buffer> buf_out =
      nuraft::buffer::alloc(sizeof(nuraft::int32) + cnt * sizeof(nuraft::int32) + size_total);
  buf_out->pos(0);
  buf_out->put(cnt);

  for (auto &entry : logs) {
    nuraft::ptr<nuraft::buffer> &bb = entry;
    buf_out->put(static_cast<nuraft::int32>(bb->size()));
    buf_out->put(*bb);
  }
  return buf_out;
}

void InmemLogStore::apply_pack(nuraft::ulong index, nuraft::buffer &pack) {
  pack.pos(0);
  nuraft::int32 num_logs = pack.get_int();

  for (nuraft::int32 ii = 0; ii < num_logs; ++ii) {
    ulong cur_idx = index + ii;
    nuraft::int32 buf_size = pack.get_int();

    nuraft::ptr<nuraft::buffer> buf_local = nuraft::buffer::alloc(buf_size);
    pack.get(buf_local);

    nuraft::ptr<nuraft::log_entry> le = nuraft::log_entry::deserialize(*buf_local);
    {
      std::lock_guard<std::mutex> l(logs_lock_);
      logs_[cur_idx] = le;
    }
  }

  {
    std::lock_guard<std::mutex> l(logs_lock_);
    auto entry = logs_.upper_bound(0);
    if (entry != logs_.end()) {
      start_idx_ = entry->first;
    } else {
      start_idx_ = 1;
    }
  }
}

auto InmemLogStore::compact(ulong last_log_index) -> bool {
  std::lock_guard<std::mutex> l(logs_lock_);
  for (ulong ii = start_idx_; ii <= last_log_index; ++ii) {
    auto entry = logs_.find(ii);
    if (entry != logs_.end()) {
      logs_.erase(entry);
    }
  }

  // WARNING:
  //   Even though nothing has been erased,
  //   we should set `start_idx_` to new index.
  if (start_idx_ <= last_log_index) {
    start_idx_ = last_log_index + 1;
  }
  return true;
}

auto InmemLogStore::flush() -> bool {
  disk_emul_last_durable_index_ = next_slot() - 1;
  return true;
}

void InmemLogStore::Close() {}

void InmemLogStore::SetDiskDelay(nuraft::raft_server *raft, size_t delay_ms) {
  disk_emul_delay_ = delay_ms;
  raft_server_bwd_pointer_ = raft;

  if (!disk_emul_thread_) {
    disk_emul_thread_ = std::make_unique<std::thread>(&InmemLogStore::disk_emul_ea_, this);
  }
}

auto InmemLogStore::last_durable_index() -> ulong {
  uint64_t last_log = next_slot() - 1;
  if (disk_emul_delay_ == 0U) {
    return last_log;
  }

  return disk_emul_last_durable_index_;
}

void InmemLogStore::DiskEmulLoop() {
  // This thread mimics async disk writes.

  size_t next_sleep_us = 100 * 1000;
  while (!disk_emul_thread_stop_signal_) {
    disk_emul_ea_.wait_us(next_sleep_us);
    disk_emul_ea_.reset();
    if (disk_emul_thread_stop_signal_) {
      break;
    }

    uint64_t cur_time = nuraft::timer_helper::get_timeofday_us();
    next_sleep_us = 100 * 1000;

    bool call_notification = false;
    {
      std::lock_guard<std::mutex> l(logs_lock_);
      // Remove all timestamps equal to or smaller than `cur_time`,
      // and pick the greatest one among them.
      auto entry = disk_emul_logs_being_written_.begin();
      while (entry != disk_emul_logs_being_written_.end()) {
        if (entry->first <= cur_time) {
          disk_emul_last_durable_index_ = entry->second;
          entry = disk_emul_logs_being_written_.erase(entry);
          call_notification = true;
        } else {
          break;
        }
      }

      entry = disk_emul_logs_being_written_.begin();
      if (entry != disk_emul_logs_being_written_.end()) {
        next_sleep_us = entry->first - cur_time;
      }
    }

    if (call_notification) {
      raft_server_bwd_pointer_->notify_log_append_completion(true);
    }
  }
}

}  // namespace vectordb
