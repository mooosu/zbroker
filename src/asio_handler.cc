#include "common.h"


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

     using namespace std;
     BSONObj obj;
     Command cmd;
     Response res =processor::parse_request(obj,cmd,packet.body());
     string hash = broker::hash(obj);
     processor *pro = m_handler->get_processors().find(hash);
     string ret =pro->process(res,cmd,obj);
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
          cout << "handle_read_body"  << error.message() << endl;
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
          cout << "write done!" << endl;
     } else {
          throw error;
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


