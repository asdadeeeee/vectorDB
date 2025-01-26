curl -X POST -H "Content-Type: application/json" -d '{"vectors": [0.8], "id": 2, "indexType": "FLAT"}'  http://localhost:7781/UserService/insert
curl -X POST -H "Content-Type: application/json" -d '{"vectors": [0.5], "k": 2, "indexType": "FLAT"}'  http://localhost:7781/UserService/search
curl -X POST -H "Content-Type: application/json" -d '{"vectors": [0.555], "id":3, "indexType": "FLAT","Name":"hello","Ci":1111}'  http://localhost:7781/UserService/upsert
curl -X POST -H "Content-Type: application/json" -d '{"id": 3}'  http://localhost:7781/UserService/query
curl -X POST -H "Content-Type: application/json" -d '{"vectors": [0.999], "id":6, "int_field":47,"indexType": "FLAT"}'  http://localhost:7781/UserService/upsert
curl -X POST -H "Content-Type: application/json" -d '{"vectors": [0.888], "id":7, "int_field":48,"indexType": "FLAT"}'  http://localhost:7781/UserService/upsert
curl -X POST -H "Content-Type: application/json" -d '{"vectors": [0.999], "k": 5 , "indexType": "FLAT","filter":{"fieldName":"int_field","value":47,"op":"="}}'  http://localhost:7781/UserService/search
curl -X POST -H "Content-Type: application/json" -d '{"vectors": [0.999], "k": 5 , "indexType": "FLAT","filter":{"fieldName":"int_field","value":47,"op":"!="}}'  http://localhost:7781/UserService/search
curl -X POST -H "Content-Type: application/json" -d '{"vectors": [0.888], "k": 1, "indexType": "FLAT","filter":{"fieldName":"int_field","value":48,"op":"="}}'  http://localhost:7781/UserService/search
curl -X POST -H "Content-Type: application/json" -d '{}' http://localhost:7781/AdminService/snapshot
curl -X POST -H "Content-Type: application/json" -d '{"vectors": [0.999], "k": 1 , "indexType": "FLAT","filter":{"fieldName":"int_field","value":47,"op":"="}}'  http://localhost:7781/UserService/search
curl -X POST -H "Content-Type: application/json" -d '{}' http://localhost:7781/AdminService/SetLeader
curl -X GET http://localhost:7781/AdminService/ListNode
curl -X GET http://localhost:7781/AdminService/GetNode
curl -X POST -H "Content-Type: application/json" -d '{"nodeId": 2, "endpoint": "127.0.0.1:8082"}' http://localhost:7781/AdminService/AddFollower


查看node信息
curl -X POST -H "Content-Type: application/json" -d '{"instanceId" : 1,"nodeId": 1}'  http://localhost:6060/MasterService/GetNodeInfo
增加node1信息
curl -X POST -H "Content-Type: application/json" -d '{"instanceId": 1, "nodeId": 1, "url": "http://127.0.0.1:7781", "role": 0, "status": 0}' http://localhost:6060/MasterService/AddNode
查看instance下的所有node信息
curl -X POST -H "Content-Type: application/json" -d '{"instanceId" : 1}'  http://localhost:6060/MasterService/GetInstance
增加node2信息
curl -X POST -H "Content-Type: application/json" -d '{"instanceId": 1, "nodeId": 2, "url": "http://127.0.0.1:7782", "role": 1, "status": 0}' http://localhost:6060/MasterService/AddNode
删除node2信息
curl -X DELETE -H "Content-Type: application/json" -d '{"instanceId" : 1,"nodeId": 2}'  http://localhost:6060/MasterService/RemoveNode


查看top结构
curl -X GET http://localhost:6061/ProxyService/topology

读请求
curl -X POST -H "Content-Type: application/json" -d '{"vectors": [0.9], "k": 5, "indexType": "FLAT", "filter":{"fieldName":"int_field","value":49, "op":"="}}' http://localhost:6061/ProxyService/search

写请求
curl -X POST -H "Content-Type: application/json" -d '{"id": 6, "vectors": [0.9], "int_field": 49, "indexType": "FLAT"}' http://localhost:6061/ProxyService/upsert

强制读主
curl -X POST -H "Content-Type: application/json" -d '{"vectors": [0.9], "k": 5, "indexType": "FLAT", "filter":{"fieldName":"int_field","value":49 ,"op":"="},"forceMaster" : true}' http://localhost:6061/ProxyService/search