//============================================================================
// Name        : TestBlocks.cpp
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

void featExtract (string filename, string genre, string outfile) {
	clock_t start, end;
	double time;
	start = clock();
	MarSystemManager mng;

	MarSystem* net =mng.create("Series", "net");
	net->addMarSystem(mng.create("SoundFileSource", "src"));
	net->addMarSystem(mng.create("Windowing", "ham"));
	net->addMarSystem(mng.create("Spectrum", "spk"));
	net->addMarSystem(mng.create("PowerSpectrum", "pspk"));
	MarSystem* fan = mng.create("Fanout", "fan");
	fan->addMarSystem(mng.create("Centroid", "centro"));
	fan->addMarSystem(mng.create("Rolloff", "rolloff"));
	fan->addMarSystem(mng.create("Flux", "flux"));
	fan->addMarSystem(mng.create("MFCC", "MFCC"));
	net->addMarSystem(fan);
	net->addMarSystem(mng.create("TextureStats", "TextureStats"));

	MarSystem* acc = mng.create("Accumulator", "acc");
	acc->addMarSystem(net);

	MarSystem* statistics = mng.create("Fanout", "statistics");
	statistics->addMarSystem(mng.create("Mean", "mn"));
	statistics->addMarSystem(mng.create("StandardDeviation", "std"));

	MarSystem* total = mng.create("Series", "total");
	total->addMarSystem(acc);
	total->updControl("Accumulator/acc/mrs_natural/nTimes", 1000);
	total->addMarSystem(statistics);

	total->addMarSystem(mng.create("Annotator", "ann"));
	total->addMarSystem(mng.create("WekaSink", "wsink"));

	total->updControl("WekaSink/wsink/mrs_natural/nLabels", 11);
	total->updControl("WekaSink/wsink/mrs_natural/downsample", 1);
	total->updControl("WekaSink/wsink/mrs_string/labelNames",
			"newmetal,metal,rock,jazz,pop,hiphop,classical,reggae,blues,country, disco,");
	total->updControl("WekaSink/wsink/mrs_string/filename", outfile);

	net->updControl("SoundFileSource/src/mrs_string/filename", filename);
	net->linkControl("mrs_bool/hasData", "SoundFileSource/src/mrs_bool/hasData");


	total->updControl("mrs_natural/inSamples", 1024);

	Collection l;
	l.read(filename);
	if (genre == "newmetal") {
		total->updControl("Annotator/ann/mrs_natural/label", 0);
	}
	else if (genre == "metal") {
		total->updControl("Annotator/ann/mrs_natural/label", 1);
		}
	else if (genre == "rock") {
			total->updControl("Annotator/ann/mrs_natural/label", 2);
		}
	else if (genre == "jazz") {
			total->updControl("Annotator/ann/mrs_natural/label", 3);
		}
	else if (genre == "pop") {
			total->updControl("Annotator/ann/mrs_natural/label", 4);
		}
	else if (genre == "hiphop") {
			total->updControl("Annotator/ann/mrs_natural/label", 5);
		}
	else if (genre == "classical") {
			total->updControl("Annotator/ann/mrs_natural/label", 6);
		}
	else if (genre == "reggae") {
			total->updControl("Annotator/ann/mrs_natural/label", 7);
		}
	else if (genre == "blues") {
			total->updControl("Annotator/ann/mrs_natural/label", 8);
		}
	else if (genre == "country") {
				total->updControl("Annotator/ann/mrs_natural/label", 9);
			}
	else if (genre == "disco") {
					total->updControl("Annotator/ann/mrs_natural/label", 10);
				}
	else {
		cout << "Choose one of the music genres listed please...." << endl;
		return;
	}



	for (size_t i=0; i < l.size(); ++i)
	{
		total->updControl("Accumulator/acc/Series/net/SoundFileSource/src/mrs_string/filename", l.entry(i));
		/* if (i==0)
			  total->updControl("Accumulator/acc/Series/playbacknet/AudioSink/dest/mrs_bool/initAudio", true);
		*/
		cout << "Processing " << l.entry(i) << endl;
		total->tick();
		//cout << "i = " << i << endl;
	}
	end = clock();
	time = (double)(end - start) / CLOCKS_PER_SEC ;
	cout << "Execution time: " << time << " seconds." << endl;
	delete total;
	return;
}

int main() {
	cout << "MX-Ray training set creator:" << endl;
	cout << "Choose one of the following genre labels: " << endl;
	cout << "------------------------------------------" << endl;
	cout << " GENRES LIST                              " << endl;
	cout << "------------------------------------------" << endl;
	cout << " 1) newmetal                              " << endl;
	cout << " 2) metal                                 " << endl;
	cout << " 3) rock                              	   " << endl;
	cout << " 4) jazz                              	   " << endl;
	cout << " 5) pop                             	   " << endl;
	cout << " 6) hiphop                                " << endl;
	cout << " 7) classical                             " << endl;
	cout << " 8) reggae                                " << endl;
	cout << " 9) blues                                 " << endl;
	cout << "------------------------------------------" << endl;
	string fileName;
	string genre;
	string outFile;
	cout << "Enter the filename: " << endl;
	cin >> fileName;
	cout << "Enter the genre" << endl;
	cin >> genre;
	cout << "Enter output filename: " << endl;
	cin >> outFile;
	//fileName = "newmetal.mf";
	//genre = "newmetal";
	//outFile = "newmetalMFCC.arff";
	featExtract(fileName, genre, outFile);
	cout << "Done." << endl;
	return 0;
}
