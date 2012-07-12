//============================================================================
// Name        : SoundAnalisys.cpp
// Author      : Dona
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <iostream>
#include <string>
#include <cstdlib>
#include <math.h>
#include <fstream>
#include <time.h>
#include <cstdio>
#include <algorithm>
#include <string.h>
#include <iomanip>


#include "marsyas/MarSystemManager.h"
using namespace std;
using namespace Marsyas;

void tempoWavelet(string sfName) {

	float length = 1000.0f;
	long duration = 1000 * 44100;
	mrs_natural offset;

	mrs_real srate = 0.0;
	clock_t start, end;
	double time;
	start = clock();
	cout << "Tempo extraction: " << sfName << endl;
	MarSystemManager mng;

	MarSystem* net = mng.create("Series", "net");
	net->addMarSystem(mng.create("SoundFileSource", "src"));
	net->addMarSystem(mng.create("Stereo2Mono", "s2m"));
	net->addMarSystem(mng.create("DownSampler", "startds"));
	net->addMarSystem(mng.create("ShiftInput", "shift"));
	net->addMarSystem(mng.create("WaveletPyramid", "wave"));

	/*
    * FANOUT IMPLICITO
	*/

	net->addMarSystem(mng.create("WaveletBands", "wavelet"));
	net->addMarSystem(mng.create("FullWaveRectifier", "rect"));
	net->addMarSystem(mng.create("OnePole", "lpf"));
	net->addMarSystem(mng.create("Norm", "normalizer"));
	{
		net->addMarSystem(mng.create("Gain", "gain"));
		net->updControl("Gain/gain/mrs_real/gain", 0.05);
	}

	/*
	 * FANIN IMPLICITO
	 */

	net->addMarSystem(mng.create("Sum", "sum"));
	net->addMarSystem(mng.create("DownSampler", "ds"));
	net->addMarSystem(mng.create("AutoCorrelation", "corr"));
	net->addMarSystem(mng.create("Peaker", "pkr"));
	net->addMarSystem(mng.create("MaxArgMax", "argmax"));
	net->addMarSystem(mng.create("PeakPeriods2BPM", "p2bpm"));
	net->addMarSystem(mng.create("BeatHistogramFromPeaks", "histo"));

	net->addMarSystem(mng.create("HarmonicEnhancer", "harm"));

	mrs_natural ifactor = 8;

	/*
	* LINK DEI CONTROLLI
	*/

	net->updControl("DownSampler/startds/mrs_natural/factor", ifactor);
	net->updControl("SoundFileSource/src/mrs_string/filename", sfName);
	srate = net->getctrl("SoundFileSource/src/mrs_real/osrate")->to<mrs_real>();

	/*
	 * UPDATE DEI CONTROLLI PER IL FILE D'INGRESSO CON HOPSIZE/WINSIZE
	 */
	mrs_natural winSize = (mrs_natural)((srate / 22050.0) * 2 * 65536);
	mrs_natural hopSize = winSize / 16;

	offset = (mrs_natural)(start * srate);
	duration = (mrs_natural)(length * srate);

	net->updControl("mrs_natural/inSamples", hopSize);
	net->updControl("SoundFileSource/src/mrs_natural/pos", offset);

	net->updControl("MaxArgMax/argmax/mrs_natural/nMaximums", 5);

	net->updControl("ShiftInput/shift/mrs_natural/winSize", winSize);

	/*
	 * FILTRO DI BANCHI WAVELET PER ESTRAZIONE INVILUPPO
	 */

	net->updControl("WaveletPyramid/wave/mrs_bool/forward", true);
	net->updControl("OnePole/lpf/mrs_real/alpha", 0.99f);

	mrs_natural factor = 32;

	net->updControl("DownSampler/ds/mrs_natural/factor", factor);

	srate = net->getctrl("DownSampler/startds/mrs_real/osrate")->to<mrs_natural>();

	/*
	 * Prelevatore di picco con risoluzione da 4 BPM a 60BPM, a partire da 50 BPM fino a 250
	 */

	mrs_natural pkinS = net->getctrl("Peaker/pkr/mrs_natural/onSamples")->to<mrs_natural>();
	mrs_real peakSpacing = ((mrs_natural)(srate * 60.0 / (factor * 62.0))) / (pkinS * 1.0);

	mrs_natural peakStart = (mrs_natural)(srate * 60.0 / (factor * 230.0));
	mrs_natural peakEnd = (mrs_natural)(srate * 60.0 / (factor * 30.0));

	net->updControl("Peaker/pkr/mrs_real/peakSpacing", peakSpacing);
	net->updControl("Peaker/pkr/mrs_real/peakStrength", 0.5);
	net->updControl("Peaker/pkr/mrs_natural/peakStart", peakStart);
	net->updControl("Peaker/pkr/mrs_natural/peakEnd", peakEnd);
	net->updControl("Peaker/pkr/mrs_real/peakGain", 2.0);

	net->updControl("BeatHistogramFromPeaks/histo/mrs_natural/startBin", 0);
	net->updControl("BeatHistogramFromPeaks/histo/mrs_natural/endBin", 230);

	/*
	 * PREPARAZIONE DEI VETTORI PER IL PROCESSING
	 */

	realvec iwin(net->getctrl("mrs_natural/inObservations")->to<mrs_natural>(),
			net->getctrl("mrs_natural/inSamples")->to<mrs_natural>());
	realvec estimate(net->getctrl("mrs_natural/onObservations")->to<mrs_natural>(),
			net->getctrl("mrs_natural/onSamples")->to<mrs_natural>());

	mrs_natural onSamples;

	int numPlayed = 0;
	mrs_natural wc = 0;
	mrs_natural samplesPlayed = 0;
	mrs_natural repeatId = 1;

	onSamples = net->getctrl("ShiftInput/shift/mrs_natural/onSamples")->to<mrs_natural>();

	while (net->getctrl("SoundFileSource/src/mrs_bool/hasData")->to<mrs_bool>())
	{
		net->process(iwin, estimate);

		numPlayed++;
		if (samplesPlayed > repeatId * duration) {
			net->updControl("SoundFileSource/src/mrs_natural/pos", offset);
			repeatId++;
		}
		wc++;
		samplesPlayed += onSamples;

	}

	/*
	 * CALCOLO DELLA FASE
	 */

	MarSystem* net1 = mng.create("Series", "net1");
	net1->addMarSystem(mng.create("SoundFileSource", "src1"));
	net1->addMarSystem(mng.create("FullWaveRectifier", "rect1"));

	/*
	 * ALTRO FANIN IMPLICITO
	 */

	net1->addMarSystem(mng.create("Sum", "sum1"));
	net1->addMarSystem(mng.create("DownSampler", "ds1"));
	net1->updControl("SoundFileSource/src1/mrs_string/filename", sfName);

	srate = net1->getctrl("SoundFileSource/src1/mrs_real/osrate")->to<mrs_real>();

	/*
	 * UPDATE DEI CONTROLLI
	 */

	winSize = (mrs_natural)(srate / 22050.0) * 8 * 65536 ;

	net1->updControl("mrs_natural/inSamples", winSize);
	net1->updControl("SoundFileSource/src1/mrs_natural/pos", 0);

	/*
	 * SETTING DEI CONTROLLI DEL BANCO DI FILTRI WAELET PER ESTRARRE L'INVILUPPO
	 */

	factor = 4;
	net1->updControl("DownSampler/ds1/mrs_natural/factor", factor);

	realvec iwin1(net1->getctrl("mrs_natural/inObservations")->to<mrs_natural>(),
					  net1->getctrl("mrs_natural/inSamples")->to<mrs_natural>());
		realvec estimate1(net1->getctrl("mrs_natural/onObservations")->to<mrs_natural>(),
						  net1->getctrl("mrs_natural/onSamples")->to<mrs_natural>());


		net1->process(iwin1, estimate1);



		mrs_real s1 = estimate(0);
		mrs_real s2 = estimate(2);
		mrs_real t1 = estimate(1);
		mrs_real t2 = estimate(3);

		mrs_natural p1 = (mrs_natural)((int)((srate * 60.0) / (factor * t1)+0.5));
		mrs_natural p2 = (mrs_natural)((int)((srate * 60.0) / (factor * t2)+0.5));


		mrs_real mx = 0.0;
		mrs_natural imx = 0;
		mrs_real sum = 0.0;




		for (mrs_natural i = 0; i < p1; ++i)
	    {
			sum = 0.0;
			sum += estimate1(0,i);

			sum += estimate1(0,i+p1);
			sum += estimate1(0,i+p1-1);
			sum += estimate1(0,i+p1+1);

			sum += estimate1(0,i+2*p1);
			sum += estimate1(0,i+2*p1-1);
			sum += estimate1(0,i+2*p1+1);


			sum += estimate1(0,i+3*p1);
			sum += estimate1(0,i+3*p1-1);
			sum += estimate1(0,i+3*p1+1);

			if (sum > mx)
			{
				mx = sum;
				imx = i;
			}
	    }

		mrs_real ph1 = (imx * factor * 1.0) / srate;

		for (mrs_natural i = 0; i < p2; ++i)
	    {
			sum = 0.0;
			sum += estimate1(0,i);

			sum += estimate1(0,i+p2);
			sum += estimate1(0,i+p2-1);
			sum += estimate1(0,i+p2+1);

			sum += estimate1(0,i+2*p2);
			sum += estimate1(0,i+2*p2-1);
			sum += estimate1(0,i+2*p2+1);

			sum += estimate1(0,i+3*p2);
			sum += estimate1(0,i+3*p2-1);
			sum += estimate1(0,i+3*p2+1);


			if (sum > mx)
			{
				mx = sum;
				imx = i;
			}
	    }

		mrs_real ph2 = (imx * factor * 1.0) / srate;





		mrs_real st = s1 / (s1 + s2);


		cout << "Estimated tempo = " << t1 << endl;

		cout << sfName << " " << t1 << " " << s1 << endl;




	cout << "Processing finished." << endl;
	end = clock();
	time = (double)(end - start)/CLOCKS_PER_SEC;
	cout << "Elaborated in " << time << endl;
	delete net;
	delete net1;
}

void tempoFlux(string sfName) {
	clock_t start, end;
	double extime;
	start = clock();
	MarSystemManager mng;

	MarSystem *beatTracker = mng.create("Series/beatTracker");


	MarSystem *onset_strength = mng.create("Series/onset_strength");
	MarSystem *accum = mng.create("Accumulator/accum");
	MarSystem *fluxnet = mng.create("Series/fluxnet");
	fluxnet->addMarSystem(mng.create("SoundFileSource", "src"));
	fluxnet->addMarSystem(mng.create("Stereo2Mono", "s2m"));
	fluxnet->addMarSystem(mng.create("ShiftInput", "si"));
	fluxnet->addMarSystem(mng.create("Windowing", "windowing1"));
	fluxnet->addMarSystem(mng.create("Spectrum", "spk"));
	fluxnet->addMarSystem(mng.create("PowerSpectrum", "pspk"));
	fluxnet->addMarSystem(mng.create("Flux", "flux"));
	accum->addMarSystem(fluxnet);

	onset_strength->addMarSystem(accum);
	onset_strength->addMarSystem(mng.create("ShiftInput/si2"));
	beatTracker->addMarSystem(onset_strength);

	MarSystem *tempoInduction = mng.create("FlowThru/tempoInduction");
	tempoInduction->addMarSystem(mng.create("Filter", "filt1"));
	tempoInduction->addMarSystem(mng.create("Reverse", "reverse"));
	tempoInduction->addMarSystem(mng.create("Filter", "filt2"));
	tempoInduction->addMarSystem(mng.create("Reverse", "reverse"));

	tempoInduction->addMarSystem(mng.create("AutoCorrelation", "acr"));
	tempoInduction->addMarSystem(mng.create("BeatHistogram", "histo"));

	MarSystem* hfanout = mng.create("Fanout", "hfanout");
	hfanout->addMarSystem(mng.create("Gain", "id1"));
	hfanout->addMarSystem(mng.create("TimeStretch", "tsc1"));

	tempoInduction->addMarSystem(hfanout);
	tempoInduction->addMarSystem(mng.create("Sum", "hsum"));
	tempoInduction->addMarSystem(mng.create("Peaker", "pkr1"));
	tempoInduction->addMarSystem(mng.create("MaxArgMax", "mxr1"));

	beatTracker->addMarSystem(tempoInduction);
	beatTracker->addMarSystem(mng.create("BeatPhase/beatphase"));
	beatTracker->addMarSystem(mng.create("Gain/id"));
	mrs_natural winSize = 256;
	mrs_natural hopSize = 128;
	mrs_natural  bwinSize = 2048;
	mrs_natural bhopSize = 128;

	onset_strength->updControl("Accumulator/accum/mrs_natural/nTimes", bhopSize);
	onset_strength->updControl("ShiftInput/si2/mrs_natural/winSize",bwinSize);


	realvec bcoeffs(1,3);
	bcoeffs(0) = 0.0564;
	bcoeffs(1) = 0.1129;
	bcoeffs(2) = 0.0564;
	tempoInduction->updControl("Filter/filt1/mrs_realvec/ncoeffs", bcoeffs);

	tempoInduction->updControl("Filter/filt2/mrs_realvec/ncoeffs", bcoeffs);
	realvec acoeffs(1,3);
	acoeffs(0) = 1.0000;
	acoeffs(1) = -1.2247;
	acoeffs(2) = 0.4504;
	tempoInduction->updControl("Filter/filt1/mrs_realvec/dcoeffs", acoeffs);
	tempoInduction->updControl("Filter/filt2/mrs_realvec/dcoeffs", acoeffs);

	tempoInduction->updControl("Peaker/pkr1/mrs_natural/peakNeighbors", 40);
	tempoInduction->updControl("Peaker/pkr1/mrs_real/peakSpacing", 0.1);
	tempoInduction->updControl("Peaker/pkr1/mrs_natural/peakStart", 200);
	tempoInduction->updControl("Peaker/pkr1/mrs_natural/peakEnd", 640);


	tempoInduction->updControl("MaxArgMax/mxr1/mrs_natural/interpolation", 1);
	tempoInduction->updControl("Peaker/pkr1/mrs_natural/interpolation", 1);
	tempoInduction->updControl("MaxArgMax/mxr1/mrs_natural/nMaximums", 2);

	onset_strength->updControl("Accumulator/accum/Series/fluxnet/PowerSpectrum/pspk/mrs_string/spectrumType", "magnitude");
	onset_strength->updControl("Accumulator/accum/Series/fluxnet/Flux/flux/mrs_string/mode", "DixonDAFX06");

	tempoInduction->updControl("BeatHistogram/histo/mrs_natural/startBin", 0);
	tempoInduction->updControl("BeatHistogram/histo/mrs_natural/endBin", 800);
	tempoInduction->updControl("BeatHistogram/histo/mrs_real/factor", 16.0);


	tempoInduction->updControl("Fanout/hfanout/TimeStretch/tsc1/mrs_real/factor", 0.5);
	tempoInduction->updControl("Fanout/hfanout/Gain/id1/mrs_real/gain", 2.0);
	tempoInduction->updControl("AutoCorrelation/acr/mrs_real/magcompress", 0.65);

	onset_strength->updControl("Accumulator/accum/Series/fluxnet/ShiftInput/si/mrs_natural/winSize", winSize);
	onset_strength->updControl("Accumulator/accum/Series/fluxnet/SoundFileSource/src/mrs_string/filename", sfName);
	beatTracker->updControl("mrs_natural/inSamples", hopSize);




	vector<mrs_real> bpms;
	vector<mrs_real> secondary_bpms;
	mrs_real bin;

	beatTracker->updControl("BeatPhase/beatphase/mrs_natural/bhopSize", bhopSize);
	beatTracker->updControl("BeatPhase/beatphase/mrs_natural/bwinSize", bwinSize);

	int extra_ticks = bwinSize/bhopSize;
	mrs_realvec tempos(2);
	mrs_realvec tempo_scores(2);
	tempo_scores.setval(0.0);

	while (1)
	{
		beatTracker->tick();
		mrs_realvec estimate = beatTracker->getctrl("FlowThru/tempoInduction/MaxArgMax/mxr1/mrs_realvec/processedData")->to<mrs_realvec>();

		bin = estimate(1) * 0.25;

		tempos(0) = bin;
		tempos(1) = estimate(3) * 0.25;
		tempos(2) = estimate(5) * 0.25;

		beatTracker->updControl("BeatPhase/beatphase/mrs_realvec/tempos", tempos);
		beatTracker->updControl("BeatPhase/beatphase/mrs_realvec/tempo_scores", tempo_scores);
		bpms.push_back(beatTracker->getctrl("BeatPhase/beatphase/mrs_real/phase_tempo")->to<mrs_real>());

		secondary_bpms.push_back(estimate(3) * 0.25);
		// cout << "phase_tempo = " << beatTracker->getctrl("BeatPhase/beatphase/mrs_real/phase_tempo")->to<mrs_real>() << endl;

		if (!beatTracker->getctrl("Series/onset_strength/Accumulator/accum/Series/fluxnet/SoundFileSource/src/mrs_bool/hasData")->to<mrs_bool>())
		{
			extra_ticks --;
		}

		if (extra_ticks == 0)
			break;
	}



	mrs_real bpm_estimate = bpms[bpms.size()-1-extra_ticks];
	mrs_real secondary_bpm_estimate;
	secondary_bpm_estimate = secondary_bpms[bpms.size()-1-extra_ticks];

	cout << "bpm_estimate = " << bpm_estimate << endl;
	cout << "secondary_bpm_estimate = " << secondary_bpm_estimate << endl;

	delete beatTracker;

	end = clock();
	extime = (double)(end - start)/CLOCKS_PER_SEC;
	cout << "Time extraction in " << extime << "seconds" << endl;
}



int main() {
	cout << "Sound File Analysis and parameters extraction." << endl;
	string fileName;
		fileName = "03 Empty And Alone.mp3";
		//tempoWavelet(fileName);
		tempoFlux(fileName);
	return 0;
}

