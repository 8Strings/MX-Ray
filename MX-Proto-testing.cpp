//============================================================================
// Name        : MX-Proto-testing.cpp
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
#include "marsyas/Centroid.h"
using namespace std;
using namespace Marsyas;

bool tline;

mrs_natural offset = 0;

mrs_natural memSize = 40;
mrs_natural winSize = 512;
mrs_natural hopSize = 512;
mrs_natural accSize = 1000;
mrs_real samplingRate_ = 22050.0;
mrs_natural accSize_ = 1000;
mrs_real start = 0.0;
mrs_natural downSample = 1;

mrs_bool featExtract_ = false;
mrs_bool stereo = false;

string wekafname = EMPTYSTRING;
string workspaceDir = EMPTYSTRING;



void centroNet(string file) {
	MarSystemManager mng;

	//creazione della rete globale di estrazione delle features

	MarSystem* extractor = mng.create("Series", "extractor");

	cout << "Creation of the basic chain.....OK" << endl;

	MarSystem* centro = mng.create("Series", "centro");

	//Creazione di una sound source

	MarSystem* src = mng.create("SoundFileSource", "src");
	centro -> addMarSystem(src);
	//Settaggio della frequenza di campionamento a 44100 Hz
	centro -> updctrl("mrs_real/israte", 44100.0);

	//supponiamo che le feature estratte siano stereo
	MarSystem* stereoFeatures = mng.create("StereoFeatures", "stereoFeat");
	MarSystem* centroide = mng.create("Centroid", "centroide");
	stereoFeatures -> addMarSystem(centroide);
	//MarSystem* stereoFeatures = mng.create("StereoFeatures", "stereoFeat");
	centro -> addMarSystem(stereoFeatures);

	//aggiunta della text memory
	centro -> addMarSystem(mng.create("TextureStats", "tStats"));
	centro -> updctrl("TextureStats/tStats/mrs_natural/memSize", memSize);
	centro -> updctrl("TextureStats/tStats/mrs_bool/reset", true);

	//aggiunta della rete al blocco principale
	//si linkano i controlli al blocco principale, piu facile
	extractor -> addMarSystem(centro);
	extractor -> linkctrl("Series/centro/SoundFileSource/src/mrs_string/filename", "mrs_string/filename");
	extractor -> linkctrl("mrs_bool/hasData", "Series/centro/SoundFileSource/src/mrs_bool/hasData");
	extractor -> linkctrl("mrs_natural/pos", "Series/centro/SoundFileSource/src/mrs_natural/pos");
	extractor -> linkctrl("mrs_real/duration", "Series/centro/SoundFileSource/src/mrs_real/duration");
	extractor -> linkctrl("mrs_string/currentlyPlaying", "Series/centro/SoundFileSource/src/mrs_string/currentlyPlaying");
	extractor -> linkctrl("mrs_natural/currentLabel", "Series/centro/SoundFileSource/src/mrs_natural/currentLabel");
	extractor -> linkctrl("mrs_natural/nLabels", "Series/centro/SoundFileSource/src/mrs_natural/nLabels");
	extractor -> linkctrl("mrs_string/labelNames", "Series/centro/SoundFileSource/src/mrs_string/labelNames");

	//labeling, output su weka
	extractor -> addMarSystem(mng.create("Annotator", "ann"));
	if (wekafname != EMPTYSTRING)
		extractor -> addMarSystem(mng.create("WekaSink", "wsink"));

	extractor -> linkctrl("Annotator/ann/mrs_natural/label", "mrs_natural/currentLabel");

	//collegamento con WekaSink
	if (wekafname != EMPTYSTRING) {
		extractor -> linkctrl("WekaSink/wsink/mrs_string/currentlyPlaying", "mrs_string/currentlyPlaying");
		extractor -> linkctrl("WekaSink/wsink/mrs_string/labelNames", "mrs_string/labelNames");
		extractor -> linkctrl("WekaSink/wsink/mrs_natural/nLabels", "mrs_natural/nLabels");
	}

	// configurazione della sorgente
	extractor -> updctrl("mrs_natural/inSamples", hopSize);
	centro -> updctrl("StereoFeatures/stereoFeat/mrs_natural/winSize", winSize);
	extractor -> updctrl("mrs_natural/pos", offset);

	mrs_real lenght = -1.0;

	extractor -> updctrl("mrs_real/duration", lenght);

	//caricare la collezione di file creabile attraverso il comando bextract da riga di comando
	if (workspaceDir != EMPTYSTRING)
		extractor -> updctrl("mrs_string/filename", workspaceDir + file);
	else
		extractor -> updctrl("mrs_string/filename", file);

	//setup WekaSink per scrittura su file, va fatto dopo tutti gli update delle variabili

	if (wekafname != EMPTYSTRING){
		extractor -> updctrl("WekaSink/wsink/mrs_natural/downsample", downSample);
		if (workspaceDir != EMPTYSTRING) {
			wekafname = workspaceDir + wekafname;
		}
		extractor -> updctrl("WekaSink/wsink/mrs_string/filename", wekafname);
	}

	//Blocco di processing principale
	MarControlPtr ctrl_hasData = extractor -> getctrl("mrs_bool/hasData");
	MarControlPtr ctrl_currentlyPlaying = extractor -> getctrl("mrs_string/currentlyPlaying");
	mrs_string previouslyPlaying, currentlyPlaying;

	int n = 0;

	vector<string> processedFiles;
	map<string, realvec> processedFeatures;

	realvec fvec;

	while (ctrl_hasData -> to<mrs_bool>()) {
		extractor -> tick();
		fvec = extractor -> getctrl("Annotator/annotator/mrs_realvec/processedData") -> to<mrs_realvec>() ;
		cout << "Feature 0: " << fvec(0) << endl;
		currentlyPlaying = ctrl_currentlyPlaying -> to<mrs_string>();
		if (currentlyPlaying != previouslyPlaying) {
			cout << "Processed: " << n << " - " << currentlyPlaying << endl;
			n++;
		}
		previouslyPlaying = currentlyPlaying;
	}

	cout << "Extraction Completed" << endl;
	delete extractor;
	return;
}

int main() {
	cout << "Centroid extraction test" << endl; // prints Centroid extraction test
	string fileName;
	fileName = "drum.wav";

	memSize = 100;
	winSize = 512;
	hopSize = 512;
	accSize = 1000;
	tline = false;
	downSample = 1;
	wekafname = "out.arff";



	centroNet(fileName);
}
