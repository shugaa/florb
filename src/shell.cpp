#include <unistd.h>
#include <sys/wait.h>

#include <iostream>
#include <stdexcept>
#include <vector>
#include <cstdio>
#include <cstdlib>

#include "utils.hpp"
#include "shell.hpp"

florb::shell::shell() :
    m_pid(-1),
    m_exitstatus(false)
{
};

florb::shell::~shell()
{
    if (m_pid > 0)
    {
        wait();
    }
};

bool florb::shell::run(const std::string& cmd)
{
    // Reset exit status for subsequet runs
    m_exitstatus = false;

    m_cmd = cmd;
    m_sout = "";

    m_pin[0] = -1;
    m_pin[1] = -1;
    m_pout[0] = -1;
    m_pout[1] = -1;

    // Create pipes for STDIN and STDOUT
    int rc;
    for (;;) 
    {
        rc = pipe2(m_pin, 0);
        if (rc != 0)
            break;

        rc = pipe2(m_pout, 0);
        if (rc != 0)
            break;

        m_pid = fork();
        if (m_pid < 0)
        {
            rc = -1;
            break;
        }

        // Fork
        if (m_pid == 0)
            child();
        else
        {
            // Close read end of child stdin pipe
            close (m_pin[0]);
            // Close write end of child stdout pipe
            close (m_pout[1]);
        }

        break;
    }

    // An error occured, close any open pipe FDs and exit
    if (rc != 0)
    {
        if (m_pin[0] != -1) close(m_pin[0]);
        if (m_pin[1] != -1) close(m_pin[1]);
        if (m_pout[0] != -1) close(m_pout[0]);
        if (m_pout[1] != -1) close(m_pout[1]);

        return false;
    }

    return true;
}

bool florb::shell::wait()
{
    // Already exited, not started or child
    if (m_pid <= 0)
        return m_exitstatus;

    int rc;
    pid_t pid = waitpid(m_pid, &rc, 0);

    for (;;)
    {
        // Waitpid error
        if (pid < 0)
        {
            break;
        }

        // Child has exited normally
        if (WIFEXITED(rc))
        {
            if (WEXITSTATUS(rc) == 0)
                m_exitstatus = true;
        }

        break;
    }

    m_pid = -1;
    close(m_pin[1]);
    close(m_pout[0]);

    return m_exitstatus;
}

bool florb::shell::readln(std::string& str)
{
    bool ok = true;

    for (;;)
    {
        // We're either in the child or have already exited
        if (m_pid <= 0)
        {
            ok = false;
            break;
        }

        // Blockingly read into buffer
        char buf[64];
        ssize_t count = read(m_pout[0], buf, 63);

        // An error occured
        if (count < 0)
        {
            if (errno == EINTR)
                continue;
            else
            {
                ok = false;
                break;
            }
        }
        
        // No data, check if child is still alive
        if (count == 0)
        {
            int rc;
            pid_t pid = waitpid(m_pid, &rc, WNOHANG);

            // Child has not exited yet, continue reading from its STDOUT
            if (pid == 0)
            {
                continue;
            }

            // Waitpid error
            if (pid < 0)
            {
                ok = false;
                break;
            }

            // Child has exited normally, update status
            if (WIFEXITED(rc))
            {
                if (WEXITSTATUS(rc) == 0)
                {
                    m_exitstatus = true;
                }
            }

            m_pid = -1;
            ok = false;
            break;
        }

        // We got data write to buffer
        buf[count] = '\0';
        m_sout += std::string(buf);

        // Check for '\r' separator
        std::size_t nl = m_sout.find("\r");

        // Check for '\n' separator
        if (nl ==  std::string::npos)
            nl = m_sout.find("\n");
        
        // Separator found? Return line and update line buffer
        if (nl != std::string::npos)
        {
            str = m_sout.substr(0, nl-1);

            nl++;
            if (nl < m_sout.length())
                m_sout = m_sout.substr(nl, m_sout.length()-nl);
            else
                m_sout = "";

            break;
        }
    }

    return ok;
}

void florb::shell::child()
{
    int rc;

    for(;;)
    {
        // Close write end of child stdin pipe
        close (m_pin[1]);

        // Close read end of child stdout pipe
        close (m_pout[0]);

        // Dup pipe ends to stdin / stdout respectively
        rc = dup2(m_pin[0], STDIN_FILENO);
        if (rc < 0)
            break;

        rc = dup2(m_pout[1], STDOUT_FILENO);
        if (rc < 0)
            break;

        rc = dup2(m_pout[1], STDERR_FILENO);
        if (rc < 0)
            break;

        // Execute command
        rc = execlp("sh", "sh", "-c", m_cmd.c_str(), NULL);

        break;
    }

    // Close open pipe ends and exit with an error
    if (rc < 0)
    {
        close(m_pin[0]);
        close(m_pout[1]);
        exit(-1);
    }
}
