#pragma once
#include <string>
#include <functional>

class Parameter
{
    void refresh(){};
    void map(){};
    void mapFine(){};
    bool mapped(){return false;}
};

class ParameterValue
{
    std::string format(const char * /*string*/){return "empty";};
    template<class T, class Q>
    T as(Q q);

};

class Synth
{
    Parameter operator[](const char *) {return Parameter();}
};


void setup_jack(void);
void write_norm_float(const char *addr, float f);
void handleUpdates(std::function<void(std::string, float)> cb);


