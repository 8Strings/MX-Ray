/*
 *MARSYSTEM FOR FLUX METHOD TEMPO ESTIMATION
 */

#include "FluxTempo.h"

using namespace std;
using namespace Marsyas;

TempoFlux::TempoFlux(string name):MarSystem("TempoFlux",name)
{
}


TempoFlux::~TempoFlux()
{
}


MarSystem*
TempoFlux::clone() const
{
  return new ZeroCrossings(*this);
}

void
TempoFlux::myUpdate(MarControlPtr sender)
{
	(void) sender;
  MRSDIAG("TempoFlux.cpp - TempoFlux:myUpdate");
  ctrl_onSamples_->setValue((mrs_natural)1, NOUPDATE);
  ctrl_onObservations_->setValue(ctrl_inObservations_, NOUPDATE);
  ctrl_osrate_->setValue(ctrl_israte_->to<mrs_real>() / ctrl_inSamples_->to<mrs_natural>());
  ctrl_onObsNames_->setValue("TempoFlux_" + ctrl_inObsNames_->to<mrs_string>() , NOUPDATE);
}




void
TempoFlux::myProcess(realvec& in, realvec& out)
{
  mrs_natural o,t;
  zcrs_ = 1.0;

  for (o=0; o < inObservations_; o++)
    for (t = 1; t < inSamples_; t++)
      {
	if (((in(o, t-1) > 0) && (in(o,t) < 0)) ||
	    ((in(o, t-1) < 0) && (in(o,t) > 0)))
	  {
	    zcrs_++;
	  }
	out(o,0) = zcrs_ / inSamples_;
      }
}
