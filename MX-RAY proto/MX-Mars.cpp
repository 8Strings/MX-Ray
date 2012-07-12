//============================================================================
// Name        : MX-Ray-proto.cpp
// Author      : Dona
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================


//============================================================================
// Name        : MX-Ray-testing-2.cpp
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
#include "marsyas/MarSystemManager.h"
#include "marsyas/Accumulator.h"
#include "marsyas/Fanout.h"
#include "marsyas/Centroid.h"
#include "marsyas/Rolloff.h"
#include "marsyas/Spectrum.h"
using namespace std;
using namespace Marsyas;

bool tline;

namespace mxray {

/**
 * Sets the the starting point for reading the samples vector
 * of audio data.
 */
mrs_natural OFFSET = 0;
mrs_natural WIN_SIZE = 512;
mrs_natural HOP_SIZE = 512;
mrs_natural MEM_SIZE = 40;
mrs_natural ACC_SIZE = 1000;
mrs_natural DOWN_SAMPLE = 1;

mrs_real start = 0.0;
mrs_real samplingRate_ = 22050 ;

mrs_bool featExtract = false;
mrs_bool stereo = false;

string wekafname = EMPTYSTRING;
string workspaceDir = EMPTYSTRING;


class MXAgent {
private:
	string filename;
public:
	MXAgent(string filename) {
		this -> filename = filename;
	}

	MXAgent(const MXAgent& agent){
		this -> filename = agent.filename;
	}

	void centroidToTxt(string sfName1) {
		cout << "Toy with centroid " << sfName1 << endl;
		MarSystemManager mng;

		MarSystem* net = mng.create("Series/net");
		net->addMarSystem(mng.create("SoundFileSource/src"));
		net->addMarSystem(mng.create("Windowing/ham"));
		net->addMarSystem(mng.create("Spectrum/spk"));
		net->addMarSystem(mng.create("PowerSpectrum/pspk"));
		net->addMarSystem(mng.create("Centroid/cntrd"));
		net->addMarSystem(mng.create("Memory/mem"));
		net->addMarSystem(mng.create("Mean/mean"));
		net->linkctrl("mrs_string/filename", "SoundFileSource/src/mrs_string/filename");
		net->updctrl("mrs_string/filename", sfName1);

		mrs_real val = 0.0;

		ofstream ofs;
		ofs.open("centroid.mpl");
		ofs << *net << endl;
		ofs.close();

		ofstream text;
		text.open("centroid.txt");


		while (net->getctrl("SoundFileSource/src/mrs_bool/hasData")->to<mrs_bool>())
		{
			net->tick();
			const mrs_realvec& src_data =
			net->getctrl("mrs_realvec/processedData")->to<mrs_realvec>();
			val = src_data(0,0);
			text << val << endl;
			//cout << val << endl;
		}
		text.close();
	}
	void extractBpm(string filename) {
		cout << "BPM extractor" << endl;
		MarSystemManager mng;
		MarSystem* blackbox = mng.create("Series", "blackbox");
		blackbox->addMarSystem(mng.create("SoundFileSource", "src"));

		/**
		 * Linkaggio dei controlli: SoundSource
		 */
		blackbox->linkctrl("mrs_string/filename", "SoundFileSource/src/mrs_string/filename");
		blackbox->linkctrl("mrs_string/labelNames", "SoundFileSource/src/mrs_string/labelNames");
		blackbox->updctrl("mrs_string/filename", filename);

		/*
		 * Creazione di un file di testo con alcune informazioni sul file:
		 * fname = name of the file courrently analyzed
		 * israte = input sample rate
		 * osrate = output sample rate
		 * duration = duration of the file in seconds
		 * size = number of audio samples contained into the file
		 * label = label for classification use
		 */

		ofstream finfo;
		finfo.open("durata.txt");

		mrs_string fname = blackbox->getctrl("mrs_string/filename")->to<mrs_string>();
		finfo << "Input file name: " << fname << endl;
		mrs_real israte = blackbox->getctrl("mrs_real/israte")->to<mrs_real>();
		finfo << "Input samplerate: " << israte << " Hz" << endl;
		mrs_real osrate = blackbox->getctrl("mrs_real/osrate")->to<mrs_real>();
		finfo << "Output samplerate: " << osrate << " Hz" << endl;
		mrs_real duration = blackbox->getctrl("SoundFileSource/src/mrs_real/duration")->to<mrs_real>();
		finfo << "Duration: " << duration <<  " seconds" << endl;
		mrs_natural size = blackbox->getctrl("SoundFileSource/src/mrs_natural/size")->to<mrs_natural>();
		finfo << "Number of samples of courrent file: " << size << endl;
		mrs_string label = blackbox->getctrl("mrs_string/labelNames")->to<mrs_string>();
		finfo << "Label name: " << label << endl;
		finfo.close();

		ofstream wavelet;
		wavelet.open("waveletbands.txt");
		wavelet << "WAVELET BANDS VALUES" << endl;
		/*
		 * Preparazione per l'estrazione dell'inviluppo del segnale audio
		 */

		//blackbox->addMarSystem(mng.create("WaveletBands", "wavelet"));
		//blackbox->addMarSystem(mng.create("FullWaveRectifier","rectifier"));
		blackbox->addMarSystem(mng.create("AutoCorrelation", "autocorrelation"));
		blackbox->updctrl("AutoCorrelation/autocorrelation/mrs_bool/aliasedOutput", false);
		blackbox->addMarSystem(mng.create("BeatHistogram", "histogram"));

		mrs_real val = 0.0;
		while (blackbox->getctrl("SoundFileSource/src/mrs_bool/hasData")->to<mrs_bool>()) {
			blackbox->tick();
			const mrs_realvec& src_data =
			blackbox->getctrl("mrs_realvec/processedData")->to<mrs_realvec>();
			val = src_data(0,0);
			wavelet << val << endl;
		}
		wavelet.close();

		delete blackbox;
		cout << "done" << endl;
	}

};

}

int main() {
	cout << "MX-Ray - Feature Extraction Test 2" << endl;
	mrs_realvec durata;
	string fileName;
	fileName = "a.mp3";
	mxray::MXAgent agent(fileName);
	//agent.centroidToTxt(fileName);
	agent.extractBpm(fileName);
}
