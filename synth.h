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

namespace rtosc{class UndoHistory;};

void setup_jack(void);
void writeNormFloat(const char *addr, float f);
void handleUpdates(std::function<void(const char *,std::string, float)> cb,
                   std::function<void()> damage_undo);
rtosc::UndoHistory *getHistory(void);

void renderWave(float *smps, unsigned nsmps, float a, float b, float c);



