#pragma once

#include <thread>
#include <queue>
#include <deque>
#include <mutex>  
#include <condition_variable> 



template<typename T>  
class safe_queue  
{  
  private:  
     mutable std::mutex mut;   
     std::queue<T> data_queue;  
     std::condition_variable data_cond;  
  public:  
    safe_queue(){}  
    safe_queue(safe_queue const& other)  
     {  
         std::lock_guard<std::mutex> lk(other.mut);  
         data_queue=other.data_queue;  
     }  
     void push(T new_value)
     {  
         std::lock_guard<std::mutex> lk(mut);  
         data_queue.push(new_value);  
         data_cond.notify_one();  
     }  
     void wait_and_pop(T& value)
     {  
         std::unique_lock<std::mutex> lk(mut);  
         data_cond.wait(lk,[this]{return !data_queue.empty();});  
         value=data_queue.front();  
         data_queue.pop();  
     }  
     std::shared_ptr<T> wait_and_pop()  
     {  
         std::unique_lock<std::mutex> lk(mut);  
         data_cond.wait(lk,[this]{return !data_queue.empty();});  
         std::shared_ptr<T> res(std::make_shared<T>(data_queue.front()));  
         data_queue.pop();  
         return res;  
     }  
     bool try_pop(T& value)
     {  
         std::lock_guard<std::mutex> lk(mut);  
         if(data_queue.empty())  
             return false;  
         value=data_queue.front();  
         data_queue.pop();  
         return true;  
     }  
     bool try_peek(T& value)
     {  
         std::lock_guard<std::mutex> lk(mut);  
         if(data_queue.empty())  
             return false;
         value=data_queue.front();
         return true;  
     }
     std::shared_ptr<T> try_pop()  
     {  
         std::lock_guard<std::mutex> lk(mut);  
         if(data_queue.empty())  
             return std::shared_ptr<T>();  
         std::shared_ptr<T> res(std::make_shared<T>(data_queue.front()));  
         data_queue.pop();  
         return res;  
     }  
     bool empty() const  
     {  
         std::lock_guard<std::mutex> lk(mut);  
         return data_queue.empty();  
     }  
     void clear() {
         std::lock_guard<std::mutex> lk(mut); 
         while(!data_queue.empty()) 
            data_queue.pop();
     }
}; 



template<typename T>  
class safe_dqueue  
{  
  private:  
     mutable std::mutex mut;   
     std::deque<T> data_queue;  
     std::condition_variable data_cond;  
  public:  
    safe_dqueue(){}  
    safe_dqueue(safe_dqueue const& other)  
     {  
         std::lock_guard<std::mutex> lk(other.mut);  
         data_queue=other.data_queue;  
     }  
     void push(T new_value)
     {  
         std::lock_guard<std::mutex> lk(mut);  
         data_queue.push_back(new_value);  
         data_cond.notify_one();  
     }  
     void push_front(T new_value)
     {  
         std::lock_guard<std::mutex> lk(mut);  
         data_queue.push_front(new_value);  
         data_cond.notify_one();  
     } 
     void wait_and_pop(T& value)
     {  
         std::unique_lock<std::mutex> lk(mut);  
         data_cond.wait(lk,[this]{return !data_queue.empty();});  
         value=data_queue.front();  
         data_queue.pop();  
     }  
     std::shared_ptr<T> wait_and_pop()  
     {  
         std::unique_lock<std::mutex> lk(mut);  
         data_cond.wait(lk,[this]{return !data_queue.empty();});  
         std::shared_ptr<T> res(std::make_shared<T>(data_queue.front()));  
         data_queue.pop();  
         return res;  
     }  
     bool try_pop(T& value)
     {  
         std::lock_guard<std::mutex> lk(mut);  
         if(data_queue.empty())  
             return false;  
         value=data_queue.front();  
         data_queue.pop_front();  
         return true;  
     }  
     bool try_peek(T& value)
     {  
         std::lock_guard<std::mutex> lk(mut);  
         if(data_queue.empty())  
             return false;
         value=data_queue.front();
         return true;  
     }
     std::shared_ptr<T> try_pop()  
     {  
         std::lock_guard<std::mutex> lk(mut);  
         if(data_queue.empty())  
             return std::shared_ptr<T>();  
         std::shared_ptr<T> res(std::make_shared<T>(data_queue.front()));  
         data_queue.pop_front();  
         return res;  
     }  
     bool empty() const  
     {  
         std::lock_guard<std::mutex> lk(mut);  
         return data_queue.empty();  
     }  

     void clear() {
         std::lock_guard<std::mutex> lk(mut);  
         data_queue.clear();
     }
}; 


