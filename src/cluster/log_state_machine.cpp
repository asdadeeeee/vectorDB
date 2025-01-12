#include "cluster/log_state_machine.h"
#include <iostream>
#include "common/constants.h"
#include "logger/logger.h"

namespace vectordb {

void LogStateMachine::SetVectorDatabase(VectorDatabase *vector_database) {
  vector_database_ = vector_database;  // 设置 vector_database_ 指针
  last_committed_idx_ = vector_database->GetStartIndexId();
}

auto LogStateMachine::commit(const nuraft::ulong log_idx, nuraft::buffer &data) -> nuraft::ptr<nuraft::buffer> {
  std::string content(reinterpret_cast<const char *>(data.data() + data.pos() + sizeof(int)),
                      data.size() - sizeof(int));
  global_logger->debug("Commit log_idx: {}, content: {}", log_idx, content);  // 添加打印日志

  rapidjson::Document json_request;
  json_request.Parse(content.c_str());
  uint64_t label = json_request[REQUEST_ID].GetUint64();

  // Update last committed index number.
  last_committed_idx_ = log_idx;

  // 获取请求参数中的索引类型
  IndexFactory::IndexType index_type = vector_database_->GetIndexTypeFromRequest(json_request);

  // vector_database_->Upsert(label, json_request, index_type);
  vector_database_->Upsert(label, json_request, index_type);
  // 在 upsert 调用之后调用 VectorDatabase::writeWALLog
//   vector_database_->WriteWalLog("upsert", json_request);

  // Return Raft log number as a return result.
  nuraft::ptr<nuraft::buffer> ret = nuraft::buffer::alloc(sizeof(log_idx));
  nuraft::buffer_serializer bs(ret);
  bs.put_u64(log_idx);
  return ret;
}

auto LogStateMachine::pre_commit(const nuraft::ulong log_idx, nuraft::buffer &data) -> nuraft::ptr<nuraft::buffer> {
  std::string content(reinterpret_cast<const char *>(data.data() + data.pos() + sizeof(int)),
                      data.size() - sizeof(int));
  global_logger->debug("Pre Commit log_idx: {}, content: {}", log_idx, content);  // 添加打印日志
  return nullptr;
}

}  // namespace vectordb