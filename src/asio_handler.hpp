#ifndef _ASIO_HANDLER_H_
#define _ASIO_HANDLER_H_

#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>
#include <boost/functional/hash.hpp>
#include <google/sparse_hash_map>
#include <glog/logging.h>
#include "asio_processor.hpp"


using namespace boost;
using namespace boost::system;
using asio::ip::tcp;

namespace zbroker{

     class asio_handler;

     typedef google::sparse_hash_map<string,processor*, boost::hash<string> > processor_hash_t;
     typedef processor_hash_t::value_type processor_item_t;

     class connection: public boost::enable_shared_from_this<connection>{

          private:

               tcp::socket m_socket;
               in_packet m_in;
               out_packet m_out;
               asio_handler *m_handler;
               processor* m_processor;
               bool   m_processor_opened;
               string m_send_buffer;
               size_t m_connection_id;
               string m_connection_id_string;


          public:
               connection(asio::io_service& io_service,asio_handler* handler,size_t connection_id):
                    m_socket(io_service), m_handler(handler){
                         m_connection_id = connection_id;
                         m_processor = NULL;
                         m_processor_opened = false;
                         m_connection_id_string="[conn#"+boost::lexical_cast<std::string>(m_connection_id) + "]";
                    }

               tcp::socket& socket() { return m_socket; }

               void start();
               void process(in_packet& msg);
               void handle_read_header(const system::error_code& error);
               void handle_read_body(const system::error_code& error);
               void handle_write(const system::error_code& error);

     };

     typedef boost::shared_ptr<connection> connection_ptr;

     //----------------------------------------------------------------------
     class processors{
          private:
               typedef boost::mutex::scoped_lock lock;
               boost::mutex m_hash_mutex;

               processor_hash_t m_processors;
               size_t           m_processor_count;
          public:
               processors(){
                    m_processor_count = 0;
               }
               processor* find(string& processor_id ){
                    processor* ret =NULL;
                    lock lk(m_hash_mutex);
                    ret = m_processors[processor_id];
                    if( ret == NULL ){
                         m_processor_count ++;
                         ret = new processor(processor_id);
                         m_processors[ret->id()] = ret;
                         LOG(INFO) << "processors count: " << m_processor_count << endl;
                    } else {
                         LOG(INFO) << "processors find "  << (void*)ret << endl;
                    }
                    return ret;
               }
     };

     class asio_handler
     {
          public:
               asio_handler(asio::io_service& io_service, const tcp::endpoint& endpoint) : 
                    m_io_service(io_service), m_acceptor(io_service, endpoint) {
                         connection_ptr new_connection = make_connection();
                         m_acceptor.async_accept( new_connection->socket(), 
                                   bind(&asio_handler::handle_accept, this, new_connection, asio::placeholders::error)
                                   );
                    }
               void handle_accept(connection_ptr pro, const error_code& error);
               connection_ptr make_connection(){
                    m_connection_count ++;
                    LOG(INFO) << "connections: " << m_connection_count << endl;
                    connection_ptr new_connection(new connection(m_io_service,this,m_connection_count));
                    return new_connection;
               }
               processors& get_processors(){return m_processors;}
          private:
               asio::io_service& m_io_service;
               tcp::acceptor m_acceptor;
               processors m_processors;
               size_t     m_connection_count;

     };

     typedef boost::shared_ptr<asio_handler> asio_handler_ptr;
     typedef std::list<asio_handler_ptr> asio_handler_list;
};

using zbroker::connection;
using zbroker::asio_handler;
using zbroker::asio_handler_ptr;
using zbroker::asio_handler_list;

//----------------------------------------------------------------------
#endif
