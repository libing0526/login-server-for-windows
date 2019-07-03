#include "logger.h"

// Global static pointer used to ensure a single instance of the class.
Logger* Logger::m_pInstance = NULL;  
/** This function is called to create an instance of the class. 
    Calling the constructor publicly is not allowed. The constructor 
    is private and is only called by this Instance function.
*/
Logger& Logger::Instance()
{
    if (!m_pInstance)   // Only allow one instance of class to be generated.
        m_pInstance = new Logger;
    return *m_pInstance;
}
bool Logger::openLogFile(const std::string &_logFile, const std::string &_del)
{
    delimiter = _del;
    setLevelMarker();
    fs.open (_logFile.c_str(),  std::fstream::out | std::fstream::app);
    if(fs.is_open())
    {
        //INFO_LOGGER << "======================================Logger Start======================================" << END_LOGGER;
		return true;
    }
    else
    {
        std::cout << "Logger is failed!" << std::endl;
		return false;
    }
}
void Logger::closeLogFile()
{
    fs.close();
}
void Logger::setLevelMarker(
        std::string _marker_information, 
        std::string _marker_warnning, 
        std::string _marker_error, 
        std::string _marker_critical, 
        std::string _marker_debug
    )
{
    marker_information = _marker_information;
    marker_warnning = _marker_warnning;
    marker_error = _marker_error;
    marker_critical = _marker_critical;
    marker_debug = _marker_debug;
}
std::string Logger::getLevelMarker(LEVEL level)
{
    std::string tmp_marker;
    switch(level)
    {
	    case Logger::critical:
	        tmp_marker = marker_critical;
	    break;
	    case Logger::information:
	        tmp_marker = marker_information;
	    break;
	    case Logger::warnning:
	        tmp_marker = marker_warnning;
	    break;
	    case Logger::error:
	        tmp_marker = marker_error;
	    break;
	    case Logger::debug:
	        tmp_marker = marker_debug;
	    break;
	    default:
	    break;
    }
    return tmp_marker;
}
Logger& Logger::operator<< (int _number)
{
    ss << _number;
    return *this;
}
Logger& Logger::operator<< (double _number)
{
    ss << _number;
    return *this;
}
Logger& Logger::operator<< (std::string msg)
{
        ss << msg;
		return *this;
}
Logger& Logger::operator <<(Logger& (*pfunc)(Logger&))
{
    return pfunc(*this);
}
void Logger::writeLogToFile()
{
    if(Logger::critical == level)
    {
        //Utility::warnning("CRITICAL", removePrefix(ss.str(), level));
        std::cout << removePrefix(ss.str(), level) << std::endl;
    }
    std::cout << removePrefix(ss.str(), level) << std::endl;
    fs << getTime() << ss.str() << std::endl;
    fs.flush();
    resetLogger();
}
//convert: 2014-7-31 17:26:57>>[CRITICAL] Test critical logger!!! >> Test critical logger!!!
std::string Logger::removePrefix(const std::string &_content, LEVEL _level)
{
    std::string tmp_string = _content;
    std::stringstream tmp_ss;
    tmp_ss << getDelimiter() << getLevelMarker(_level);
    int tmp_pos = _content.find(tmp_ss.str());
    if(tmp_pos == std::string::npos)
    {
        tmp_string = _content;
    }
    else
    {
        tmp_string = _content.substr(tmp_pos+tmp_ss.str().length());
    }
    return tmp_string;
}
void Logger::resetLogger()
{
    ss.clear();
    ss.str("");
    level = Logger::information;
}
std::string Logger::getTime()
{
    std::string date;
    std::stringstream date_stream;
    time_t t = time(0);   // get time now
    struct tm * now = localtime( & t );
    date_stream << (now->tm_year + 1900) << '-'
         << (now->tm_mon + 1) << '-'
         <<  now->tm_mday << ' '
         <<  now->tm_hour << ':'
         <<  now->tm_min << ':'
         <<  now->tm_sec;
    date = date_stream.str();
    return date;
}
std::stringstream& Logger::getStringStream()
{
    return ss;
}
std::string& Logger::getDelimiter()
{
    return delimiter;
}
void Logger::setLevel(LEVEL _level)
{
    level = _level;
}
//flags
Logger& INFO(Logger& _logger)
{
    _logger.setLevel(Logger::information);
    _logger.getStringStream() << _logger.getDelimiter() << "[INFORMATION] ";
    return _logger;
}
Logger& WARNNING(Logger& _logger)
{
    _logger.setLevel(Logger::warnning);
    _logger.getStringStream() << _logger.getDelimiter() << "[WARNNING] ";
    return _logger;
}
Logger& ERRORS(Logger& _logger)
{
    _logger.setLevel(Logger::error);
    _logger.getStringStream() << _logger.getDelimiter() << "[ERROR] ";
    return _logger;
}
Logger& CRITICAL(Logger& _logger)
{
    _logger.setLevel(Logger::critical);
    _logger.getStringStream() << _logger.getDelimiter() << _logger.getLevelMarker(Logger::critical);
    return _logger;
}
Logger& DEBUG(Logger& _logger)
{
    _logger.setLevel(Logger::debug);
    _logger.getStringStream() << _logger.getDelimiter() << "[DEBUG] ";
    return _logger;
}
Logger& END_LOGGER(Logger& _logger)
{
    _logger.writeLogToFile();
    return _logger;
}
