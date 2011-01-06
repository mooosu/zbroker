#ifndef _ASIO_HANDLER_H_
#define _ASIO_HANDLER_H_

#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>

using namespace boost;
using namespace boost::system;
using asio::ip::tcp;

class connection: public boost::enable_shared_from_this<connection>{

     private:

          tcp::socket m_socket;
          request_packet m_read_msg;
          
     public:
          connection(asio::io_service& io_service):m_socket(io_service) {}

          tcp::socket& socket() { return m_socket; }

          void start();
          void deliver(const request_packet& msg);
          void handle_read_header(const system::error_code& error);
          void handle_read_body(const system::error_code& error);
          void handle_write(const system::error_code& error);

};

typedef boost::shared_ptr<connection> connection_ptr;

//----------------------------------------------------------------------

class asio_handler
{
     public:
          asio_handler(asio::io_service& io_service, const tcp::endpoint& endpoint) : 
               m_io_service(io_service), m_acceptor(io_service, endpoint) {
                    connection_ptr new_connection(new connection(m_io_service));
                    m_acceptor.async_accept( new_connection->socket(), 
                              bind(&asio_handler::handle_accept, this, new_connection, asio::placeholders::error)
                              );
               }


          void handle_accept(connection_ptr pro, const error_code& error);
     private:
          asio::io_service& m_io_service;
          tcp::acceptor m_acceptor;
};

typedef boost::shared_ptr<asio_handler> asio_handler_ptr;
typedef std::list<asio_handler_ptr> asio_handler_list;

//----------------------------------------------------------------------
#endif
