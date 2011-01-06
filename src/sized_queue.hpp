#ifndef _SIZED_QUEUE_HPP_
#define _SIZED_QUEUE_HPP_

#include <queue>
#include <boost/utility.hpp>
#include <boost/thread/condition.hpp>
namespace zbroker{
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
               std::queue<T> m_queue;
               boost::condition m_cond_can_push, m_cond_can_pop;
               boost::mutex m_monitor;
          public:
               typedef boost::mutex::scoped_lock lock;

               sized_queue(size_t size) {
                    m_limit_size = size;
               }
               sized_queue() {
                    m_limit_size = 100;
               }
               void set_size(size_t size){
                    m_limit_size = size;
               }
               size_t size(){ return m_queue.size(); }
               size_t get_limit_size(){ return m_limit_size; }
               T pop(size_t seconds=0) {
                    lock lk(m_monitor);
                    if( m_queue.size() == 0 ){
                         m_cond_can_push.notify_one();
                         if( seconds> 0 ){
                              boost::xtime xt;
                              boost::xtime_get(&xt, boost::TIME_UTC);
                              xt.sec += seconds;
                              if(!m_cond_can_pop.timed_wait(lk,xt)){
                                   throw broker_timeout("no more items"); 
                              }
                         }else {
                              m_cond_can_pop.wait(lk);
                         }
                    }
                    T item = m_queue.front();
                    m_queue.pop();
                    m_cond_can_push.notify_one();
                    return item;
               }
               int push(T item, size_t seconds=0) {
                    lock lk(m_monitor);
                    if( m_queue.size() >= m_limit_size ){
                         m_cond_can_pop.notify_one();
                         if( seconds> 0 ){
                              boost::xtime xt;
                              boost::xtime_get(&xt, boost::TIME_UTC);
                              xt.sec += seconds;
                              if(!m_cond_can_push.timed_wait(lk,xt)){
                                   throw broker_timeout("no consumer"); 
                              }
                         } else {
                              m_cond_can_push.wait(lk);
                         }

                    }
                    m_queue.push(item);
                    m_cond_can_pop.notify_one();
                    return m_queue.size();
               }
     };
};
#ifdef TEST
#include <time.h>
#include <unistd.h>
#include <iostream>
#include <boost/thread/thread.hpp>
sized_queue<int> buf(2);

boost::mutex io_mutex;

void sender() {
     int n = 0;
     sleep(10);
     while (n < 100) {
          sleep(1);
          buf.push(n);
          {
               boost::mutex::scoped_lock io_lock(io_mutex);
               std::cout << "sent: " << n << std::endl;
          }
          ++n;
     }
     buf.push(-1);
}

void receiver() {
     int n;
     do {
          n = buf.pop();
          {
               boost::mutex::scoped_lock io_lock(io_mutex);
               std::cout << "received: " << n << std::endl;
          }
     } while (n != -1); // -1 indicates end of buffer
}

int main(int, char*[])
{
     boost::thread thrd1(&sender);
     boost::thread thrd2(&receiver);
     thrd1.join();
     thrd2.join();
     return 0;
}
#endif
#endif // _SIZED_QUEUE_HPP_
