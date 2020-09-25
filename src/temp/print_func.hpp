#ifndef PRINT_FUNC_H_
#define PRINT_FUNC_H_

#include <vector>
#include <string>
#include <stdarg.h>
#include <initializer_list>

namespace Out
{

using std::vector;
using std::string;

class Out
{
    public:

    Out(): data(){}

    Out(std::initializer_list<string> l): data(l){}

    Out& operator=(std::initializer_list<string> l)
    {
        data = l;
        return *this;
    }

    bool operator==(Out const& other) const
    {
        return data == other.data;
    }

    friend std::ostream& operator<<(std::ostream& os, Out const& out)
    {
        os << string("\"");
        for(auto const& item: out.data)
            os << item;
        os << string("\"");
        return os;
    }

    void push_back(string const& val)
    {
        data.push_back(val);
    }

    string& back()
    {
        return data.back();
    }

    bool empty() const
    {
        return data.empty();
    }

    void clear()
    {
        data.clear();
    }

    private:
    vector<string> data;
};

void print(void* out, int arg_count, va_list ap)
{
    Out& m_out = *(Out*)out;

    if(arg_count <= 0)
        return;

    for(int i = 0; i < arg_count; i++) {
        string str = string(va_arg(ap, char*));
        if(str.empty())
            continue;
        /* if str is space token */
        if(str[0] == ' ') { 
            /* if last stored token was space token */
            if(!m_out.empty() && m_out.back()[0] == ' ')
                /* concatenate space token to space token */
                m_out.back() += str;
            else
                /* add space token behind word token */
                m_out.push_back(str);
        } else {
            /* str is word token */
            /* if last stored token was space token */
            if(!m_out.empty() && m_out.back()[0] == ' ')
                /* add word token behind space token */
                m_out.push_back(str);
            else
                /* concatenate work token with word token */
                m_out.back() += str;
        }
    }
}

}

#endif
