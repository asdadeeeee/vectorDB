curl -X POST -H "Content-Type: application/json" -d '{"vectors": [0.8], "id": 2, "indexType": "FLAT"}'  http://localhost:7777/UserService/insert
curl -X POST -H "Content-Type: application/json" -d '{"vectors": [0.5], "k": 2, "indexType": "FLAT"}'  http://localhost:7777/UserService/search
curl -X POST -H "Content-Type: application/json" -d '{"vectors": [0.555], "id":3, "indexType": "FLAT","Name":"hello","Ci":1111}'  http://localhost:7777/UserService/upsert
curl -X POST -H "Content-Type: application/json" -d '{"id": 3}'  http://localhost:7777/UserService/query
curl -X POST -H "Content-Type: application/json" -d '{"vectors": [0.999], "id":6, "int_field":47,"indexType": "FLAT"}'  http://localhost:7777/UserService/upsert
curl -X POST -H "Content-Type: application/json" -d '{"vectors": [0.888], "id":7, "int_field":48,"indexType": "FLAT"}'  http://localhost:7777/UserService/upsert
curl -X POST -H "Content-Type: application/json" -d '{"vectors": [0.999], "k": 5 , "indexType": "FLAT","filter":{"fieldName":"int_field","value":47,"op":"="}}'  http://localhost:7777/UserService/search
curl -X POST -H "Content-Type: application/json" -d '{"vectors": [0.999], "k": 5 , "indexType": "FLAT","filter":{"fieldName":"int_field","value":47,"op":"!="}}'  http://localhost:7777/UserService/search
curl -X POST -H "Content-Type: application/json" -d '{"vectors": [0.888], "k": 1, "indexType": "FLAT","filter":{"fieldName":"int_field","value":48,"op":"="}}'  http://localhost:7777/UserService/search
curl -X POST -H "Content-Type: application/json" -d '{}' http://localhost:7777/AdminService/snapshot
curl -X POST -H "Content-Type: application/json" -d '{"vectors": [0.999], "k": 1 , "indexType": "FLAT","filter":{"fieldName":"int_field","value":47,"op":"="}}'  http://localhost:7777/UserService/search