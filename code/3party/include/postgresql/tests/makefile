all:
	#libpq.so 是gpdb编译的 ,libpqxx.a 是pqxx编译的
	g++ connection.cpp -o connection -I../postgresql -I.. -L../../../lib -lpqxx -lpq -std=c++11 
	g++ create.cpp -o create -I../postgresql -I.. -L../../../lib -lpqxx -lpq -std=c++11
	g++ insert.cpp -o insert -I../postgresql -I.. -L../../../lib -lpqxx -lpq -std=c++11
	g++ query.cpp -o query -I../postgresql -I.. -L../../../lib -lpqxx -lpq -std=c++11
	g++ update.cpp -o update -I../postgresql -I.. -L../../../lib -lpqxx -lpq -std=c++11
	