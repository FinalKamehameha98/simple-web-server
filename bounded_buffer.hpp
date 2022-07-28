#include <condition_variable>
#include <queue>

class bounded_buffer{
    private:
        std::size_t m_capacity;
        std::queue<int> m_buffer;
        std::condition_variable m_data_available;
        std::condition_variable m_space_available;
        std::mutex m_buff_mutex; 
    
    public:
        bounded_buffer(int max_size);
        void put_item(int item);
        int get_item();
};
