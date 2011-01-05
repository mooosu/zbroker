#include <algorithm>
#include <cstdlib>
#include <deque>
#include <iostream>
#include <list>
#include <set>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>
#include "common.h"
using namespace boost;
using namespace mongo;
using asio::ip::tcp;

typedef struct{
     const char* msg;
     size_t size;
}error_message;
#define MAX_REQUEST_MESSAGE_SIZE  (1024*1024)
#define ERROR_MESSAGE(x) {x,sizeof(x)-1}
static error_message error_messages[]={
     ERROR_MESSAGE( "Command not found"),
     ERROR_MESSAGE( "Command unimplemented yet!"),
     ERROR_MESSAGE( "Request body exceed limit(1MB)!"),
     ERROR_MESSAGE( "No more items!"),
     ERROR_MESSAGE( "Internal Error(invalid error code)")
};
class processor: public boost::enable_shared_from_this<processor>{

     typedef enum {
          OPEN  = 100,
          READ  = 101,
          WRITE = 102
     }Command;
     typedef enum {
          MinError       = 500,
          UnknownCommand = 500,
          Unimplemented  = 501,
          RequestTooLong = 502,
          NoMoreItem     = 503,
          ErrorMax = NoMoreItem+1,
          OK             = 200,
     }Response;
     private:
          zbroker::broker m_read_broker;
          zbroker::broker m_update_broker;

          tcp::socket m_socket;

          request_packet m_read_msg;

          char* m_request_buf;
          size_t m_request_buf_size;
          
     public:
          processor(asio::io_service& io_service):m_socket(io_service)
          {
               m_request_buf = NULL;
               m_request_buf_size = 0;
          }
          tcp::socket& socket()
          {
               return m_socket;
          }

          void start()
          {
               asio::async_read(m_socket,
                         asio::buffer(m_read_msg.data(), request_packet::header_length),
                         boost::bind( &processor::handle_read_header, shared_from_this(),
                              asio::placeholders::error));
          }

          void deliver(const request_packet& msg)
          {
               asio::async_write(m_socket, asio::buffer(msg.data(), msg.length()),
                         boost::bind(&processor::handle_write, shared_from_this(),
                              asio::placeholders::error));
          }
          void handle_read_header(const system::error_code& error)
          {
               if (!error && m_read_msg.decode_header())
               {
                    asio::async_read(m_socket, asio::buffer(m_read_msg.body(), m_read_msg.body_length()),
                              boost::bind(&processor::handle_read_body, shared_from_this(),
                                   asio::placeholders::error));
               } else {
                    throw error;
               }
          }

          void handle_read_body(const system::error_code& error)
          {
               if (!error) {
                    using namespace std;
                    cout << "length: " << m_read_msg.length() <<",str: |" << m_read_msg.data() << "|" << endl;
                    asio::async_read(m_socket,
                              asio::buffer(m_read_msg.data(), request_packet::header_length),
                              boost::bind(&processor::handle_read_header, shared_from_this(),
                                   asio::placeholders::error));
               } else {
                    throw error;
               }
          }

          void handle_write(const system::error_code& error)
          {
               if (!error) {
                    request_packet m_reply;
                    asio::async_write(m_socket,
                              asio::buffer(m_reply.data(), m_reply.length()),
                              boost::bind(&processor::handle_read_header, shared_from_this(),
                                   asio::placeholders::error));
               } else {
                    throw error;
               }
          }
          void send_error( Response error ){
               if(error < ErrorMax && error> MinError ){
                    int err_no = error - MinError;
                    error_message* msg = &error_messages[err_no];
                    request_packet reply ();
               }
          }
          void process( request_packet& request )
          {
               if( request.length() > MAX_REQUEST_MESSAGE_SIZE ){
                    send_error( RequestTooLong );
                    return;
               }
               BSONObj cmdObj=fromjson(request.data());
               Command cmd = (Command)cmdObj.getIntField("cmd");
               switch( cmd ){
                    case READ:
                         break;
                    case WRITE:
                         send_error( Unimplemented);
                         break;
                    default:
                         send_error( UnknownCommand );
               }

          }
};

typedef boost::shared_ptr<processor> processor_ptr;

//----------------------------------------------------------------------

class asio_handler
{
     public:
          asio_handler(asio::io_service& io_service, const tcp::endpoint& endpoint) : 
               m_io_service(io_service), m_acceptor(io_service, endpoint) {
                    processor_ptr new_processor(new processor(m_io_service));
                    m_acceptor.async_accept( new_processor->socket(), 
                              boost::bind(&asio_handler::handle_accept, this, new_processor, asio::placeholders::error)
                              );
               }

          void handle_accept(processor_ptr pro,
                    const system::error_code& error)
          {
               if (!error)
               {
                    pro->start();
                    processor_ptr new_processor(new processor(m_io_service));
                    m_acceptor.async_accept(new_processor->socket(),
                              boost::bind(&asio_handler::handle_accept, this, new_processor,
                                   asio::placeholders::error));
               }
          }

     private:
          asio::io_service& m_io_service;
          tcp::acceptor m_acceptor;
};

typedef boost::shared_ptr<asio_handler> asio_handler_ptr;
typedef std::list<asio_handler_ptr> asio_handler_list;

//----------------------------------------------------------------------

int main(int argc, char* argv[])
{
     try
     {
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
