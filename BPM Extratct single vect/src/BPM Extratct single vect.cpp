//============================================================================
// Name        : BPM.cpp
// Author      : Dona
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <cstdlib>
#include <string>
#include <time.h>
#include <math.h>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <time.h>
#include <cstdio>

#include "marsyas/MarSystemManager.h"
#include "marsyas/Accumulator.h"
#include "marsyas/Fanout.h"
#include "marsyas/Collection.h"

using namespace std;
using namespace Marsyas;

mrs_real extractBpm (string sfName) {
	cout << "Onsets function calculation: " << sfName << endl;
	MarSystemManager mng;

	/*
	 * ONSET FUNCTION: Estrazione del flusso dell'energia spettrale del segnale audio
	 * La catena è composta da una sorgente sonora, una funzione di finestratura,
	 * due blocchi per computare la potenza spettrale ed in fine il blocco per calcolare
	 * il flusso.
	 */
	MarSystem* beatextractor = mng.create("Series", "beatextractor");

	MarSystem* onset_strength = mng.create ("Series", "ostrenght");

	MarSystem* acc = mng.create("Accumulator", "acc");
	MarSystem* onsets = mng.create("Series", "onsets");
	MarSystem* src = mng.create("SoundFileSource", "src");
	MarSystem* s2m = mng.create("Stereo2Mono", "s2m");
	MarSystem* si1 = mng.create("ShiftInput", "si");
	MarSystem* windowing = mng.create("Windowing", "windows");
	MarSystem* spectrum = mng.create("Spectrum", "spectrum");
	MarSystem* powspec = mng.create("PowerSpectrum", "power");
	MarSystem* flux = mng.create("Flux", "flux");

	onsets->addMarSystem(src);
	onsets->addMarSystem(s2m);
	onsets->addMarSystem(si1);
	onsets->addMarSystem(windowing);
	onsets->addMarSystem(spectrum);
	onsets->addMarSystem(powspec);
	onsets->addMarSystem(flux);

	//aggiunta della rete per il calcolo del flusso all'accumulatore
	acc->addMarSystem(onsets);

	//aggiunta del sistema flusso estrattore al blocco superiore
	onset_strength->addMarSystem(acc);
	onset_strength->addMarSystem(mng.create("ShiftInput", "si2"));

	//aggiunta al componente principale
	beatextractor->addMarSystem(onset_strength);

	/*
	 * SECONDO SOTTOSISTEMA: TEMPO INDUCTION
	 * Questa parte del sistema di occupa della tempo induction
	 */

	MarSystem* timeind = mng.create("FlowThru", "timeind" );

	timeind->addMarSystem(mng.create("Filter", "filt1"));
	timeind->addMarSystem(mng.create("Reverse", "reverse"));
	timeind->addMarSystem(mng.create("Filter", "filt2"));
	timeind->addMarSystem(mng.create("Reverse", "reverse"));
	timeind->addMarSystem(mng.create("AutoCorrelation", "acr"));
	timeind->addMarSystem(mng.create("BeatHistogram", "histo"));

	MarSystem* hfanout = mng.create("Fanout", "hfanout");
	hfanout->addMarSystem(mng.create("Gain", "id1"));
	hfanout->addMarSystem(mng.create("TimeStretch", "tsc1"));

	timeind->addMarSystem(hfanout);
	timeind->addMarSystem(mng.create("Sum", "hsum"));
	timeind->addMarSystem(mng.create("Peaker", "peaker1"));
	timeind->addMarSystem(mng.create("MaxArgMax", "mxr1"));

	//aggiunta al beatextractor
	beatextractor->addMarSystem(timeind);
	beatextractor->addMarSystem(mng.create("BeatPhase","beatphase"));
	beatextractor->addMarSystem(mng.create("Gain", "id"));

	/*
	 * DEFINIZIONE ED ASSEGNAMENTO DEI PARAMETRI
	 */

	//Sorgente sonora
	onsets->updControl("SoundFileSource/src/mrs_string/filename", sfName);
	onsets->linkControl("mrs_bool/hasData", "SoundFileSource/src/mrs_bool/hasData");

	//parametri per le finestrature e per l'accumulatore
	mrs_natural winSize = 256;
	mrs_natural hopSize = 128;
	mrs_natural bwinSize = 2048;
	mrs_natural bhopSize = 128;

	onset_strength->updControl("Accumulator/acc/mrs_natural/nTimes", bhopSize);
	onset_strength->updControl("ShiftInput/si2/mrs_natural/winSize",bwinSize);



	//Coefficienti per il filtro

	mrs_realvec bcoeffs (1,3);
	bcoeffs(0) = 0.0564;
	bcoeffs(1) = 0.1129;
	bcoeffs(2) = 0.0564;
	mrs_realvec acoeffs (1,3);
	acoeffs(0) = 1.0000;
	acoeffs(1) = -1.2247;
	acoeffs(2) = 0.4504;

	timeind->updControl("Filter/filt1/mrs_realvec/ncoeffs", bcoeffs);
	timeind->updControl("Filter/filt2/mrs_realvec/ncoeffs", bcoeffs);
	timeind->updControl("Filter/filt1/mrs_realvec/dcoeffs", acoeffs);
	timeind->updControl("Filter/filt2/mrs_realvec/dcoeffs", acoeffs);

	/*
	 * CONFIGURAZIONE DEL PEAKER DEI BEAT:
	 * peakNeighbors = numero di picchi in prossimità
	 * peakSpacing = spaziatura dei picchi tra di loro in percentuale
	 * 				 rispetto alla lunghezza del vettore
	 * peakStart = punto di partenza dei picchi (in samples)
	 * peakEnd = punto finale dei picchi (in samples)
	 */

	timeind->updControl("Peaker/peaker1/mrs_natural/peakNeighbors", 40);
	timeind->updControl("Peaker/peaker1/mrs_real/peakSpacing", 0.1);
	timeind->updControl("Peaker/peaker1/mrs_natural/peakStart", 200);
	timeind->updControl("Peaker/peaker1/mrs_natural/peakEnd", 640);

	/*
	 * RICERCA DEL MASSIMO TRAMITE INTERPOLAZIONE
	 * nMaximums = 2 cerca due massimi
	 */

	timeind->updControl("MaxArgMax/mxr1/mrs_natural/interpolation", 1);
	timeind->updControl("Peaker/peaker1/mrs_natural/interpolation", 1);
	timeind->updControl("MaxArgMax/mxr1/mrs_natural/nMaximums", 2);

	/*
	 * COMPUTAZIONE SPETTRO DI AMPIEZZA (MAGNITUDE)
	 * il tipo di spettro specificato è "magnitude" ovvero ampiezza
	 * il flusso viene calcolare con modalità DixonDAFX06
	 */
	onset_strength->updControl("Accumulator/acc/Series/onsets/PowerSpectrum/power/mrs_string/spectrumType", "magnitude");
	onset_strength->updControl("Accumulator/acc/Series/onsets/Flux/flux/mrs_string/mode", "DixonDAFX06");

	/*
	 * CONFIGURAZIONE DEL BEAT HISTOGRAM CALCULATOR:
	 * startBin = "finestra" di partenza
	 * endBin = "finestra" finale
	 * factor = fattore di finestratura
	 */
	timeind->updControl("BeatHistogram/histo/mrs_natural/startBin", 0);
	timeind->updControl("BeatHistogram/histo/mrs_natural/endBin", 800);
	timeind->updControl("BeatHistogram/histo/mrs_real/factor", 16.0);

	/*
	 * Aggiunta di Timestretching e compressione
	 */

	timeind->updControl("Fanout/hfanout/TimeStretch/tsc1/mrs_real/factor", 0.5);
	timeind->updControl("Fanout/hfanout/Gain/id1/mrs_real/gain", 2.0);
	timeind->updControl("AutoCorrelation/acr/mrs_real/magcompress", 0.65);

	/*
	 * INPUT SHIFTER
	 */

	onset_strength->updControl("Accumulator/acc/Series/onsets/ShiftInput/si/mrs_natural/winSize", winSize);
	onset_strength->updControl("Accumulator/acc/Series/onsets/SoundFileSource/src/mrs_string/filename", sfName);

	/*
	 * Configurazione finale beat extractor:
	 * Il numero di campioni in ingresso viene settato alla dimensione della finestra di valutazione
	 */

	beatextractor->updControl("mrs_natural/inSamples", hopSize);

	/*
	 * Qui inizia la parte di processing vera a propria:
	 * Vengono creati due vettori, uno per i battiti primari, uno per i secondari
	 * bin indica il numero del bin attualmente in processing (tra 0 e 800)
	 */

	vector<mrs_real> bpms;
	vector<mrs_real> secondary_bpms;
	mrs_real bin;

	/*
	 * SETTAGGIO DELLE FINESTRE E DEI SALTI PER IL BEAT EXTRACTOR
	 */

	beatextractor->updControl("BeatPhase/beatphase/mrs_natural/bhopSize", bhopSize);
	beatextractor->updControl("BeatPhase/beatphase/mrs_natural/bwinSize", bwinSize);

	/*
	 * SETTAGGIO DEL NUMERO DI EXTRATICK PER LA RETE = dimensione della finestra/salto
	 */

	int extra_ticks = bwinSize/bhopSize;
	mrs_realvec tempos(2);
	mrs_realvec tempo_scores(2);
	tempo_scores.setval(0.0);

	/*
	 * PROCESSING
	 */



	while (1)
	{
		beatextractor->tick();
		mrs_realvec estimate = beatextractor->getctrl("FlowThru/timeind/MaxArgMax/mxr1/mrs_realvec/processedData")->to<mrs_realvec>();

		bin = estimate(1) * 0.25;

		tempos(0) = bin;
		tempos(1) = estimate(3) * 0.25;

		beatextractor->updControl("BeatPhase/beatphase/mrs_realvec/tempos", tempos);
		beatextractor->updControl("BeatPhase/beatphase/mrs_realvec/tempo_scores", tempo_scores);
		bpms.push_back(beatextractor->getctrl("BeatPhase/beatphase/mrs_real/phase_tempo")->to<mrs_real>());

		secondary_bpms.push_back(estimate(3) * 0.25);

		if (!beatextractor->getctrl("Series/onset_strength/Accumulator/acc/Series/onset/SoundFileSource/src/mrs_bool/hasData")->to<mrs_bool>()) {
			extra_ticks --;
		}

		if (extra_ticks == 0)
			break;
	}
	mrs_real bpm_estimate = bpms[bpms.size()-1-extra_ticks];
	mrs_real secondary_bpm_estimate;
	secondary_bpm_estimate = secondary_bpms[bpms.size()-1-extra_ticks];

	mrs_real error = secondary_bpm_estimate - bpm_estimate;
	cout << "bpm_estimate = " << bpm_estimate << " BPM, with an error of: "
			<< abs(error) << endl;
	return bpm_estimate;
	delete onsets;
}

void arffFileCreator(mrs_real value) {
	ofstream bpms;
	bpms.open("bpm.arff");
	bpms << "%Created by MX-Ray" << endl;
	bpms << "@Relation bpm.arff" << endl;
	bpms << "@attribute Bpm real" << endl;
	bpms << "@attribute output {Molto veloce,veloce,grintosa,lenta}" << endl;
	bpms << endl;
	bpms << endl;
	bpms << value << ",label" << endl;
	bpms.close();
	return;
}


int main() {
	cout << "Try..." << endl; // prints
	string filename = "a.mp3";
	bool single_file;
	string si, no, intake;
	si = "si";
	no = "no";
	cout << "Single file?" << endl;
	cin >> intake ;
	if (intake == si) {
		mrs_real bpm;
		bpm = extractBpm(filename);
		cout << "Ritmo = " << bpm << " BPM" << endl;
		arffFileCreator(bpm);
		cout << "End." << endl;
	}
	else if (intake == no) {
		cout << "Altro algoritmo" << endl;

	}
	else cout << "Fuck" << endl;
	return 0;
}
