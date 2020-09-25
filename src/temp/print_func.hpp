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
            //os << item << string("-");
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
    string word;
    string spaces;

    if(arg_count <= 0)
        return;

    for(int i = 0; i < arg_count; i++) {
        string str = string(va_arg(ap, char*));
        if(str.empty())
            continue;
        if(str[0] == ' ') { 
            if(!m_out.empty() && m_out.back()[0] == ' ')
                m_out.back() += str;
            else
                spaces += str;
        } else {
            word += spaces + str;
            spaces = "";
        }
    }

    word += spaces;

    if(!word.empty())
        m_out.push_back(word);
}

}
    //using namespace Print_func;

    //std::vector<std::string> out;
    //const char *space = " ";
    //wrapper(&out, 1, space);
    //print_vec(out); out.clear();

    //wrapper(&out, 2, space, "lol");
    //print_vec(out); out.clear();

    //wrapper(&out, 3, space, "lol", space);
    //print_vec(out); out.clear();

    //wrapper(&out, 3, space,  space, "lol");
    //print_vec(out); out.clear();

    //wrapper(&out, 3, "lol",  space, space);
    //print_vec(out); out.clear();

//void wrapper(void* out, int arg_count, ...)
//{
//    va_list ap;
//    va_start(ap, arg_count);
//    print_func(out, arg_count, ap);
//    va_end(ap);
//}
//
//void print_vec(vector<string> out)
//{
//    for(auto item: out)
//        std::cout << item << "|";
//    std::cout << std::endl;
//}


#endif
