#pragma once

#include <libnuraft/nuraft.hxx>
#include "database/vector_database.h"
namespace vectordb {
class LogStateMachine : public nuraft::state_machine {
 public:
  LogStateMachine() : last_committed_idx_(0) {}
  ~LogStateMachine() override = default;
  void SetVectorDatabase(VectorDatabase *vector_database);  // 重命名为 setVectorDatabase
  auto commit(nuraft::ulong log_idx, nuraft::buffer &data) -> nuraft::ptr<nuraft::buffer> override;

  auto pre_commit(nuraft::ulong log_idx, nuraft::buffer &data) -> nuraft::ptr<nuraft::buffer> override;
  void commit_config(const nuraft::ulong log_idx, nuraft::ptr<nuraft::cluster_config> &new_conf) override {
    // Nothing to do with configuration change. Just update committed index.
    last_committed_idx_ = log_idx;
  }
  void rollback(const nuraft::ulong log_idx, nuraft::buffer &data) override {}
  auto read_logical_snp_obj(nuraft::snapshot &s, void *&user_snp_ctx, nuraft::ulong obj_id,
                            nuraft::ptr<nuraft::buffer> &data_out, bool &is_last_obj) -> int override {
    return 0;
  }
  void save_logical_snp_obj(nuraft::snapshot &s, nuraft::ulong &obj_id, nuraft::buffer &data, bool is_first_obj,
                            bool is_last_obj) override {}
  auto apply_snapshot(nuraft::snapshot &s) -> bool override { return true; }
  void free_user_snp_ctx(void *&user_snp_ctx) override {}
  auto last_snapshot() -> nuraft::ptr<nuraft::snapshot> override { return nullptr; }
  auto last_commit_index() -> nuraft::ulong override { return last_committed_idx_; }

  void create_snapshot(nuraft::snapshot &s, nuraft::async_result<bool>::handler_type &when_done) override {}

 private:
  // Last committed Raft log number.
  std::atomic<uint64_t> last_committed_idx_;
  VectorDatabase *vector_database_;  // 添加一个 VectorDatabase 指针成员变量
};

}  // namespace vectordb