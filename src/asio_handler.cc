#include "common.h"


void connection::start()
{
     asio::async_read(m_socket,
               asio::buffer(m_read_msg.data(), request_packet::header_length),
               boost::bind( &connection::handle_read_header, shared_from_this(),
                    asio::placeholders::error));
}

void connection::deliver(const request_packet& msg)
{
     asio::async_write(m_socket, asio::buffer(msg.data(), msg.length()),
               boost::bind(&connection::handle_write, shared_from_this(),
                    asio::placeholders::error));
}
void connection::handle_read_header(const system::error_code& error)
{
     if (!error && m_read_msg.decode_header())
     {
          asio::async_read(m_socket, asio::buffer(m_read_msg.body(), m_read_msg.body_length()),
                    boost::bind(&connection::handle_read_body, shared_from_this(),
                         asio::placeholders::error));
     } else {
          throw error;
     }
}

void connection::handle_read_body(const system::error_code& error)
{
     if (!error) {
          using namespace std;
          cout << "length: " << m_read_msg.length() <<",str: |" << m_read_msg.data() << "|" << endl;
          deliver(m_read_msg);
          asio::async_read(m_socket,
                    asio::buffer(m_read_msg.data(), request_packet::header_length),
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
          connection_ptr new_connection(new connection(m_io_service));
          m_acceptor.async_accept(new_connection->socket(),
                    boost::bind(&asio_handler::handle_accept, this, new_connection,
                         asio::placeholders::error));
     }
}


