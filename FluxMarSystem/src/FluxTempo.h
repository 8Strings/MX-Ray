/*
 * FluxTempo.h
 *
 *  Created on: 12/lug/2010
 *      Author: gio
 */

#ifndef MARSYAS_ZEROCROSSINGS_H
#define MARSYAS_ZEROCROSSINGS_H

#include "MarSystem.h"

namespace Marsyas
{
/**
    \class ZeroCrossings
	\ingroup Analysis
    \brief Time-domain ZeroCrossings

    Basically counts the number of times the input
signal crosses the zero line.
*/


class TempoFlux: public MarSystem
{
private:
  mrs_real zcrs_;

	void myUpdate(MarControlPtr sender);

public:
  TempoFlux(std::string name);

  ~TempoFlux();
  MarSystem* clone() const;

  void myProcess(realvec& in, realvec& out);
};

}//namespace marsyas

#endif
