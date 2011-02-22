#ifndef _SIZED_QUEUE_HPP_
#define _SIZED_QUEUE_HPP_

#include <vector>
#include <deque>
#include <boost/utility.hpp>
#include <boost/thread/condition.hpp>
using std::vector;
namespace zbroker{
     using boost::mutex;
     using boost::condition;
     void wait( mutex::scoped_lock& lk, condition &cond , size_t seconds,const char* timeout_msg);

     class broker_exception : public std::exception {
          public:
               broker_exception() : m_msg("broker exception raised" ){}
               broker_exception( const char * msg ) : m_msg(msg){}
               broker_exception( std::string& msg ) : m_msg(msg){}
               virtual ~broker_exception() throw() { }
               virtual const char* what() const throw(){ return m_msg.c_str(); }
          protected:
               std::string m_msg;
     };
     typedef broker_exception broker_timeout;
     typedef broker_exception broker_argument_error;
     typedef broker_exception broker_not_inited;
     typedef broker_exception broker_not_connected;
     template <typename T>
     class sized_queue: private boost::noncopyable
     {
          private:
               size_t m_limit_size ;
               std::deque<T> m_queue;
               condition m_cond_can_push, m_cond_can_pop;
               mutex m_monitor;
          public:
               sized_queue(size_t size) {
                    m_limit_size = size;
               }
               sized_queue() {
                    m_limit_size = 100;
               }
               void set_size(size_t size){
                    m_limit_size = size;
               }
               size_t size(){ 
                    mutex::scoped_lock lk(m_monitor);
                    return m_queue.size(); 
               }
               size_t get_limit_size(){ return m_limit_size; }
               void clear(){
                    mutex::scoped_lock lk(m_monitor);
                    m_queue.clear();
               }
               T pop(size_t seconds = 0);
               int push(T item, size_t seconds = 0);
               int push(vector<T>& items , size_t seconds = 0);
     };
};
#endif // _SIZED_QUEUE_HPP_
