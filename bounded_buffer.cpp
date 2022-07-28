#include "bounded_buffer.hpp"

bounded_buffer::bounded_buffer(int max_size){
    m_capacity = max_size;
}

void bounded_buffer::put_item(int new_item){
    std::unique_lock<std::mutex> lock(m_buff_mutex);
    
    while(m_buffer.size() == m_capacity){
        m_space_available.wait(lock);
    }

    m_buffer.push(new_item);
    m_data_available.notify_one();
    lock.unlock();
}

int bounded_buffer::get_item(){
    std::unique_lock<std::mutex> lock(m_buff_mutex);
    while(m_buffer.empty()){
        m_data_available.wait(lock);
    }

    int item = m_buffer.front();
    m_buffer.pop();
    m_space_available.notify_one();
    lock.unlock();
    return item;
}   
