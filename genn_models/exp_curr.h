#pragma once

// GeNN includes
#include "modelSpec.h"

namespace BoBRobotics {
namespace GeNNModels {

//----------------------------------------------------------------------------
// BoBRobotics::GeNNModels::ExpCurr
//----------------------------------------------------------------------------
//! Current-based synapse model with direct current injection
class ExpCurr : public PostsynapticModels::Base
{
public:
    DECLARE_MODEL(ExpCurr, 1, 0);

    SET_DECAY_CODE("$(inSyn)*=$(expDecay);");

    SET_CURRENT_CONVERTER_CODE("$(init) * $(inSyn)");

    SET_PARAM_NAMES({"tau"});

    SET_DERIVED_PARAMS({
        {"expDecay", [](const std::vector<double> &pars, double dt){ return std::exp(-dt / pars[0]); }},
        {"init", [](const std::vector<double> &pars, double dt){ return (pars[0] * (1.0 - std::exp(-dt / pars[0]))) * (1.0 / dt); }}});
};
IMPLEMENT_MODEL(ExpCurr);
} // GeNNModels
} // BoBRobotics
