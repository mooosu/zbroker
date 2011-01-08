#ifndef _ASIO_HANDLER_H_
#define _ASIO_HANDLER_H_

#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>
#include <boost/functional/hash.hpp>
#include <google/sparse_hash_map>
#include "asio_processor.hpp"

typedef google::sparse_hash_map<size_t,processor_ptr, boost::hash<size_t> > processor_hash_t;
typedef processor_hash_t::value_type processor_item_t;

using namespace boost;
using namespace boost::system;
using asio::ip::tcp;

class connection: public boost::enable_shared_from_this<connection>{

     private:

          tcp::socket m_socket;
          in_packet m_in;
          out_packet m_out;
          processor_ptr &m_processor;
          
     public:
          connection(asio::io_service& io_service,processor_ptr& pro):m_socket(io_service),m_processor(pro)
     {
          }

          tcp::socket& socket() { return m_socket; }

          void start();
          void deliver(out_packet& msg);
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
               m_io_service(io_service), m_acceptor(io_service, endpoint),m_processor_count(0) {

                    connection_ptr new_connection = make_connection();
                    m_acceptor.async_accept( new_connection->socket(), 
                              bind(&asio_handler::handle_accept, this, new_connection, asio::placeholders::error)
                              );
               }
          void handle_accept(connection_ptr pro, const error_code& error);
          connection_ptr make_connection(){
               processor_ptr pro(new processor(++m_processor_count));
               connection_ptr new_connection(new connection(m_io_service,pro)); //pro just a ref

               m_processors[m_processor_count] = pro; // to make processor_ptr a copy ,then m_processors[str_id] holds the pointer
               return new_connection;
          }
     private:
          asio::io_service& m_io_service;
          tcp::acceptor m_acceptor;

          processor_hash_t m_processors;
          size_t           m_processor_count;
};

typedef boost::shared_ptr<asio_handler> asio_handler_ptr;
typedef std::list<asio_handler_ptr> asio_handler_list;

//----------------------------------------------------------------------
#endif
