#include "common.h"

using namespace std;

int main(int argc, char* argv[])
{
     try
     {
          google::InitGoogleLogging(argv[0]);
          if (argc < 2)
          {
               std::cerr << "Usage: asio_handler <port> [<port> ...]\n";
               return 1;
          }

          asio::io_service io_service;

          asio_handler_list servers;
          for (int i = 1; i < argc; ++i)
          {
               using namespace std; // For atoi.
               tcp::endpoint endpoint(tcp::v4(), atoi(argv[i]));
               asio_handler_ptr server(new asio_handler(io_service, endpoint));
               servers.push_back(server);
          }

          io_service.run();
     }
     catch (std::exception& e)
     {
          std::cerr << "Exception: " << e.what() << "\n";
     }

     return 0;
}
