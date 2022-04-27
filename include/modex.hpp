#ifndef MODEX_H
#define MODEX_H

#include "nexus.hpp"
#include "config.hpp"
#include <vector>

class ModEx {

    private:
        int currentPulse = 1;
    public:
        Config cfg;
        std::string out;
        std::string dataDir;
        int expStart;
        double progress;
        int totalPulses = 0;

        ModEx(Config cfg_) : cfg(cfg_) {}
        ModEx() = default;
        
        bool process();
        bool epochPulses(std::vector<Pulse> &pulses);
        bool extrapolatePulseTimes(std::string start_run, double start, bool backwards, bool forwards, double periodDuration, PulseDefinition pulseDefinition, std::vector<Pulse> &pulses);
        bool binPulsesToRuns(std::vector<Pulse> &pulses);
};

#endif // MODEX_H