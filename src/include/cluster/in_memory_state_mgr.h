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

// #include "cluster/in_memory_log_store.h"
// #include <iostream>
// #include "libnuraft/nuraft.hxx"

// namespace vectordb {

// class InmemStateMgr : public nuraft::state_mgr {
//  public:
//   InmemStateMgr(int srv_id, const std::string &endpoint, VectorDatabase* vector_database)
//       : my_id_(srv_id), my_endpoint_(endpoint), cur_log_store_(nuraft::cs_new<InmemLogStore>(vector_database)) {
//     my_srv_config_ = nuraft::cs_new<nuraft::srv_config>(srv_id, endpoint);

//     // Initial cluster config: contains only one server (myself).
//     saved_config_ = nuraft::cs_new<nuraft::cluster_config>();
//     saved_config_->get_servers().push_back(my_srv_config_);
//   }

//   ~InmemStateMgr() override = default;

//   auto load_config() -> nuraft::ptr<nuraft::cluster_config> override {
//     // Just return in-memory data in this example.
//     // May require reading from disk here, if it has been written to disk.
//     return saved_config_;
//   }

//   void save_config(const nuraft::cluster_config &config) override {
//     // Just keep in memory in this example.
//     // Need to write to disk here, if want to make it durable.
//     nuraft::ptr<nuraft::buffer> buf = config.serialize();
//     saved_config_ = nuraft::cluster_config::deserialize(*buf);
//   }

//   void save_state(const nuraft::srv_state &state) override {
//     // Just keep in memory in this example.
//     // Need to write to disk here, if want to make it durable.
//     nuraft::ptr<nuraft::buffer> buf = state.serialize();
//     saved_state_ = nuraft::srv_state::deserialize(*buf);
//   }

//   auto read_state() -> nuraft::ptr<nuraft::srv_state> override {
//     // Just return in-memory data in this example.
//     // May require reading from disk here, if it has been written to disk.
//     return saved_state_;
//   }

//   auto load_log_store() -> nuraft::ptr<nuraft::log_store> override {
//     return std::static_pointer_cast<nuraft::log_store>(cur_log_store_);
//   }

//   auto server_id() -> nuraft::int32 override { return my_id_; }

//   void system_exit(const int exit_code) override {}

//   auto GetSrvConfig() const -> nuraft::ptr<nuraft::srv_config> { return my_srv_config_; }

//  private:
//   int my_id_;
//   std::string my_endpoint_;
//   nuraft::ptr<InmemLogStore> cur_log_store_;
//   nuraft::ptr<nuraft::srv_config> my_srv_config_;
//   nuraft::ptr<nuraft::cluster_config> saved_config_;
//   nuraft::ptr<nuraft::srv_state> saved_state_;
// };

// }  // namespace vectordb
