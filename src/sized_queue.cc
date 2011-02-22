#include "sized_queue.hpp"
using namespace zbroker;

void zbroker::wait( mutex::scoped_lock& lk, boost::condition& cond , size_t seconds,const char* timeout_msg){
     if( seconds > 0 ){
          boost::xtime xt;
          boost::xtime_get(&xt, boost::TIME_UTC);
          xt.sec += seconds;
          if(!cond.timed_wait(lk,xt)){
               throw broker_timeout(timeout_msg); 
          }
     } else {
          cond.wait(lk);
     }
}

template <typename T>
T sized_queue<T>::pop(size_t seconds) 
{
     mutex::scoped_lock lk(m_monitor);
     if( m_queue.size() == 0 ){
          m_cond_can_push.notify_one();
          wait(lk,m_cond_can_pop,seconds,"no more items");
     }
     T item = m_queue.front();
     m_queue.pop_front();
     m_cond_can_push.notify_one();
     return item;
}
template <typename T>
int sized_queue<T>::push(T item, size_t seconds) 
{
     mutex::scoped_lock lk(m_monitor);
     if( m_queue.size() >= m_limit_size ){
          m_cond_can_pop.notify_one();
          wait(lk,m_cond_can_push,seconds,"no consumer");
     }
     m_queue.push_back(item);
     m_cond_can_pop.notify_one();
     return m_queue.size();
}
template <typename T>
int sized_queue<T>::push(vector<T>& items , size_t seconds ){
     mutex::scoped_lock lk(m_monitor);
     if( m_queue.size() >= m_limit_size ){
          m_cond_can_pop.notify_one();
          wait(lk,m_cond_can_push,seconds,"no consumer");
     }
     while( items.size() > 0 && m_queue.size() < m_limit_size  ){
          m_queue.push_back(items.back());
          items.pop_back();
     }
     m_cond_can_pop.notify_one();
     return m_queue.size();
}
void __f(){
     sized_queue<std::string> sized_string_queue;
     vector<std::string> xxx;
     //sized_string_queue.push("xxx");
     sized_string_queue.push(xxx);
     sized_string_queue.pop();
}

#ifdef TEST
#include <time.h>
#include <unistd.h>
#include <iostream>
#include <boost/thread/thread.hpp>
sized_queue<int> buf(2);

mutex io_mutex;

void sender() {
     int n = 0;
     while (n < 100) {
          sleep(1);
          buf.push(n);
          {
               mutex::scoped_lock io_lock(io_mutex);
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
               mutex::scoped_lock io_lock(io_mutex);
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
