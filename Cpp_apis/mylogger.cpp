#include "mylogger.h"

using namespace std;

void mylog::debug(string message) {
    cout << "DEBUG\t" << message << endl;
}

void mylog::error(string message) {
    cerr << "ERROR\t" << message << endl;
}
