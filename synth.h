#pragma once
#include <string>
#include <functional>
#include <map>

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

namespace rtosc{class UndoHistory;class MidiMappernRT;};

void setup_jack(void);
void writeNormFloat(const char *addr, float f);
void handleUpdates(std::function<void(const char *,std::string, float)> cb,
                   std::function<void()> damage_undo,
                   std::function<void()> damage_midi);
rtosc::UndoHistory *getHistory(void);
std::map<std::string,std::string> getMidiMap(void);
void tryMap(std::string);
void tryMap(std::string, bool coarse);
void delMap(std::string);
void delMap(std::string, bool fine);
rtosc::MidiMappernRT *getMidiMapper(void);

void renderWave(float *smps, unsigned nsmps, float a, float b, float c);
