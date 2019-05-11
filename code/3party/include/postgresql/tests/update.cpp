#include <iostream>
#include <pqxx/pqxx>

using namespace std;

int main(int, char *argv[])
{
    try{
      pqxx::connection c("dbname=postgres hostaddr=192.168.18.78 user=cjy password=cjy port=5432");
      pqxx::work txn(c);

      pqxx::result r = txn.exec(
    "SELECT id "
    "FROM company "
    "WHERE name =" + txn.quote(argv[1]));

  if (r.size() != 1)
  {
    std::cerr
      << "Expected 1 employee with name " << argv[1] << ", "
      << "but found " << r.size() << std::endl;
    return 1;
  }

  int employee_id = r[0][0].as<int>();
  std::cout << "Updating employee #" << employee_id << std::endl;

  txn.exec(
    "UPDATE company "
    "SET salary = salary + 1 "
    "WHERE id = " + txn.quote(employee_id));

  txn.commit();
  c.disconnect ();
  }
  catch (const std::exception &e){
      cerr << e.what() << std::endl;
      return 1;
   }
  return 0;
}