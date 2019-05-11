#include <iostream>
#include <pqxx/pqxx> 

using namespace std;
using namespace pqxx;

int main(int argc, char* argv[])
{
   char * sql;
   
   try{
      connection C("dbname=postgres hostaddr=192.168.18.78 user=cjy password=cjy port=5432");
      if (C.is_open()) {
         cout << "Opened database successfully: " << C.dbname() << endl;
      } else {
         cout << "Can't open database" << endl;
         return 1;
      }
      /* Create SQL statement */
      sql = "INSERT INTO COMPANY (ID,NAME,AGE,ADDRESS,SALARY) "  \
      "VALUES (1, 'Paul', 32, 'California', 20000.00 ); " \
      "INSERT INTO COMPANY (ID,NAME,AGE,ADDRESS,SALARY) "  \
      "VALUES (2, 'Allen', 25, 'Texas', 15000.00 ); "     \
      "INSERT INTO COMPANY (ID,NAME,AGE,ADDRESS,SALARY)" \
      "VALUES (3, 'Teddy', 23, 'Norway', 20000.00 );" \
      "INSERT INTO COMPANY (ID,NAME,AGE,ADDRESS,SALARY)" \
      "VALUES (4, 'Mark', 25, 'Rich-Mond ', 65000.00 );";

      /* Create a transactional object. */
      work W(C);
      
      /* Execute SQL query */
      W.exec( sql );
      W.commit();
      cout << "Records created successfully" << endl;
      C.disconnect ();
   }catch (const std::exception &e){
      cerr << e.what() << std::endl;
      return 1;
   }

   return 0;
}  