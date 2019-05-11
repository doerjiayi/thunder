#include <iostream>
#include <pqxx/pqxx>

/// Query employees from database.  Return result.
pqxx::result query()
{
  pqxx::connection c{"dbname=postgres hostaddr=192.168.18.78 user=cjy password=cjy port=5432"};
  pqxx::work txn{c};

  pqxx::result r = txn.exec("SELECT name, salary FROM company");
  for (auto row: r)
    std::cout
	  << row.size() << " "
	  << row[0] << " "
      // Address column by name.  Use c_str() to get C-style string.
      << row["name"].c_str()
      << " makes "
      // Address column by zero-based index.  Use as<int>() to parse as int.
      << row[1].as<int>()
      << "."
      << std::endl;

  // Not really needed, since we made no changes, but good habit to be
  // explicit about when the transaction is done.
  txn.commit();

  // Connection object goes out of scope here.  It closes automatically.
  return r;
}


/// Query employees from database, print results.
int main(int, char *argv[])
{
  try
  {
    pqxx::result r = query();

    // Results can be accessed and iterated again.  Even after the connection
    // has been closed.
    for (auto row: r)
    {
      std::cout << "Row: ";
      // Iterate over fields in a row.
      for (auto field: row) std::cout << field.c_str() << " ";
      std::cout << std::endl;
    }
  }
  catch (const pqxx::sql_error &e)
  {
    std::cerr << "SQL error: " << e.what() << std::endl;
    std::cerr << "Query was: " << e.query() << std::endl;
    return 2;
  }
  catch (const std::exception &e)
  {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }
}
