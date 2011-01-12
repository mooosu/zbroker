#include "common.h"
#include "asio_handler.hpp"

void connection::start()
{
     packet_header &header = m_in.get_header();
     asio::async_read(m_socket,
               asio::buffer(header.buffer(), header.length()),
               boost::bind( &connection::handle_read_header, shared_from_this(),
                    asio::placeholders::error));
}

void connection::process(in_packet& packet)
{
     BSONObj obj;
     Command cmd;
     string ret;
     Response res =processor::parse_request(obj,cmd,packet.body());
     if( m_processor == NULL ){
          string hash ;
          if( cmd != OPEN ){
               hash = packet.get_packet_id();
          } else {
               hash = broker::hash(obj);
          }
          m_processor = m_handler->get_processors().find(hash);
          BOOST_ASSERT(m_processor);
     }
     LOG(INFO) << "connection::process: m_processor_opened == " << m_processor_opened << endl;
     if( cmd != OPEN ){
          ret =m_processor->process(res,cmd,obj);
     } else {
          if( ! m_processor_opened ){
               ret =m_processor->process(res,cmd,obj);
               m_processor_opened = true;
          } else {
               out_packet_ptr packet(new out_packet());
               ret = processor::pack_response(*packet.get(),AlreadyOpen,"connection::process");
          }
     }
     asio::async_write(m_socket, asio::buffer(ret.c_str(), ret.size()),
               boost::bind(&connection::handle_write, shared_from_this(),
                    asio::placeholders::error));
}
void connection::handle_read_header(const system::error_code& error)
{
     if (!error )
     {
          // will throw ex if exceed limit
          size_t body_len = m_in.get_header().decode();
          asio::async_read(m_socket, 
                    asio::buffer(m_in.reserve(body_len), body_len),
                    boost::bind(&connection::handle_read_body, shared_from_this(),
                         asio::placeholders::error));
     } else {

          if( m_processor != NULL ){
               m_processor->close();
               m_processor = NULL;
          }
          LOG(ERROR)<< "connection::handle_read_body: "  << error.message() << endl;
          /*
          if( error.value() == boost::asio::error::eof &&  error.category() == boost::asio::error::get_misc_category()){
               LOG(INFO) << "connection::handle_read_body: client dead or close, then close processor" << endl;
          } else{
               LOG(ERROR)<< "connection::handle_read_body:"  << error.message() << endl;
          }
          */
     }
}

void connection::handle_read_body(const system::error_code& error)
{
     if (!error) {
          process(m_in);
          packet_header &header = m_in.get_header();
          asio::async_read(m_socket,
                    asio::buffer(header.buffer(), header.length()),
                    boost::bind(&connection::handle_read_header, shared_from_this(),
                         asio::placeholders::error));
     } else {
          throw error;
     }
}

void connection::handle_write(const system::error_code& error)
{
     if (!error) {
          LOG(INFO) << "connection::handle_write: write done!" << endl;
     } else {

          LOG(ERROR)<< "connection::handle_write: "  << error.message() << endl;
          system::error_code ignored_ec;
          m_socket.shutdown(tcp::socket::shutdown_both, ignored_ec);
     }
}

// ----------------------------------------------------------------------------------//

void asio_handler::handle_accept(connection_ptr pro, const error_code& error)
{
     if (!error)
     {
          pro->start();
          connection_ptr new_connection = make_connection();
          m_acceptor.async_accept(new_connection->socket(),
                    boost::bind(&asio_handler::handle_accept, this, new_connection,
                         asio::placeholders::error));
     }
}


