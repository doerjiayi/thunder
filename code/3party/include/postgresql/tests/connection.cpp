#include <iostream>  
  
#include <pqxx/pqxx>  
  
int main()  
{  
    pqxx::connection conn("dbname=postgres hostaddr=192.168.18.78 user=cjy password=cjy" );  
    if(conn.is_open())  
    {  
        std::cout << "Connection succesful!" << std::endl;  
        std::cout << conn.options()<<std::endl;  
    
        pqxx::work w(conn);
        pqxx::result res = w.exec("SELECT 1");
        w.commit();

       std::cout << res[0][0].as<int>() << std::endl;
    }  
    else  
    {  
        std::cout << "Something went wrong... oops" << std::endl;  
    }
    return 0;  
}  
