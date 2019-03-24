#ifndef MYLOGGER_H_INCLUDED
#define MYLOGGER_H_INCLUDED

#include <iostream>

namespace mylog {
    void debug(std::string message);
    void error(std::string message);
}
#endif // MYLOGGER_H_INCLUDED
