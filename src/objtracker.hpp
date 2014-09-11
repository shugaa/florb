#ifndef OBJTRACKER_HPP
#define OBJTRACKER_HPP

#include <set>

namespace florb
{
    template <class T>
    class objtracker
    {
        protected:
            static void add_instance(T* o)
            {
                m_instances.insert(o);
            }

            static void del_instance(T* o)
            {
                if (!is_instance(o))
                    return;

                m_instances.erase(m_instances.find(o));
            }

            static bool is_instance(T* o)
            {
                typename std::set<T*>::iterator it = m_instances.find(o);
                if (it == m_instances.end())
                    return false;

                return true;       
            };

        private:
            static std::set<T*> m_instances;
    };

    // There can be more than one definition of [...] in a program provided
    // that each definition appears in a different translation unit, and
    // provided the definitions satisfy the following requirements: In summary
    // all duplicate definitions must be identical, which is the case here.
    template <class T> 
    std::set<T*> objtracker<T>::m_instances;
};

#endif // OBJTRACKER_HPP

