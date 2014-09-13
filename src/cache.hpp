#ifndef CACHE_HPP
#define CACHE_HPP

#include <time.h>
#include <string>
#include <vector>

namespace florb
{
    class cache
    {
        public:
            cache(const std::string& url, const std::string& session, const std::string& ext);
            ~cache();

            int get(int z, int x, int y, std::vector<char> &buf);
            int exists(int z, int x, int y);
            void put(int z, int x, int y, time_t expires, const std::vector<char> &buf);

            enum 
            {
                EXPIRED,
                NOTFOUND,
                FOUND
            };

        private:
            std::string m_url;
            std::string m_session;
            std::string m_ext;
            static const std::string dbextension;
    };
};

#endif // CACHE_HPP

