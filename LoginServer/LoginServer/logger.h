#ifndef LOGGER_H
#define LOGGER_H
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <time.h>
#include <stdio.h>
//define some AMCRO to make usage more simply
#define INIT_LOGGER(x) Logger::Instance().openLogFile(x)
#define LOGGER Logger::Instance()
#define INFO_LOGGER LOGGER << INFO 
#define WARNNING_LOGGER LOGGER << WARNNING
#define ERROR_LOGGER LOGGER << ERROR
#define CRITICAL_LOGGER LOGGER << CRITICAL 
#define DEBUG_LOGGER LOGGER<< DEBUG

class Logger{
public:
    //define logger level
    enum LEVEL {information=0, warnning, error, critical, debug};
    //single instance modle, static function to get ref of object 
    static Logger& Instance();
    bool openLogFile(const std::string &logFile, const std::string &del=">>");
    void setLevelMarker(
        std::string _marker_information = "[INFORMATION] ", 
        std::string _marker_warnning = "[WARNNING] ", 
        std::string _marker_error = "[ERROR] ", 
        std::string _marker_critical = "[CRITICAL] ", 
        std::string _marker_debug = "[DEBUG] "
    );
    void writeLogToFile();
    void closeLogFile();
    //overload operator << to support: LOGGER << str1 << str2 << END_LOGGER
    Logger& operator << (std::string msg);
    Logger& operator<< (int number);
    Logger& operator<< (double number);
    //Logger& operator<< (bool ok);
    //overload operator << to support: LOGGER << pfunc << str2 << END_LOGGER, pfunc a function pointer to do somethigs
    Logger& operator <<(Logger& (*pfunc)(Logger&));
    //maybe should define mort << functions, like <<(int), <<(double), <<(char*), <<(obj)
    std::stringstream& getStringStream();
    std::string& getDelimiter();
    void setLevel(LEVEL level);
    std::string getLevelMarker(LEVEL level);
private:
    //single instance modle, private empty and copy construct functions to forbid to instance this class 
    Logger(){};
    Logger(Logger const&){};
    Logger& operator=(Logger const&){ return *this; };  // assignment operator is private
    //static self
    static Logger* m_pInstance;
    //to write strings to file
    std::fstream fs;
    //to append and cache log message
    std::stringstream ss;
    //define the delimiter after time, default: >>
    std::string delimiter;
    //define current logger level
    LEVEL level;
    std::string marker_information;
    std::string marker_warnning;
    std::string marker_error;
    std::string marker_critical;
    std::string marker_debug;
    void resetLogger();
	std::string getTime();
    std::string removePrefix(const std::string &content, LEVEL level);
};
//a global function to support LOGGER << pfunc << str2 << END_LOGGER. In these functions could do something before log message was writen into file
//flag
Logger& INFO(Logger& logger);
Logger& WARNNING(Logger& logger);
Logger& ERRORS(Logger& logger);
Logger& CRITICAL(Logger& logger);
Logger& DEBUG(Logger& logger);
//end action
Logger& END_LOGGER(Logger& logger);

#endif
