#ifndef SHELL_HPP
#define SHELL_HPP

#include <sys/types.h>

#include <string>
#include <sstream>

class shell
{
    public:
        shell();
        ~shell();

        bool run(const std::string& cmd);
        bool readln(std::string& str);
        bool wait();

    private:
        void child();

        std::string m_cmd;
        pid_t m_pid;
        std::string m_sout; 
        bool m_exitstatus;

        int m_pin[2];
        int m_pout[2];
};

#endif // SHELL_HPP
