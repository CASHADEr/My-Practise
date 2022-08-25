#include "../include/UserLog.hpp"

int main() {
    cshr_log(Cshr::Logger::DEBUG, "This is a test %d %s", "aa", 1);
}