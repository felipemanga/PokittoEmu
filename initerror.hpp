#pragma once

class InitError : public std::exception
{
    std::string msg;
public:
    InitError() : exception(), msg( SDL_GetError() ){}
    InitError( const std::string & );
    virtual ~InitError() throw() {}
    virtual const char * what() const throw() {
        return msg.c_str();
    }
};
