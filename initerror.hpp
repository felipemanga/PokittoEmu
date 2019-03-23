#pragma once

class InitError : public std::exception
{
    std::string msg;
public:
    InitError( const std::string &_msg ) : exception(), msg(_msg){}
    virtual ~InitError() throw() {}
    virtual const char * what() const throw() {
        return msg.c_str();
    }
};
