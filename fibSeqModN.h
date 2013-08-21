#ifndef FIB_SEQ_MOD_N_H
#define FIB_SEQ_MOD_N_H

//check if all these headers are necessary
#include <cmath>
#include <vector>
#include <list>
#include <sstream>
#include "allocore/io/al_App.hpp"

#include "Gamma/SamplePlayer.h"

using namespace std;
using namespace al;


class FibSeqModN : public SoundSource {
    
  public:
  
  	FibSeqModN(Vec3f seqPos_, const int g0_, const int g1_, const int mod_) : seqPos(seqPos_), g0(g0_), g1(g1_), mod(mod_) {
  	
  		level = .1; // level of samples being played
  		
		farClip(75.0); // distance at which amplitude reaches ampFar
		ampFar(0.0); // amp at far clip

		nearClip(0.0); // distance at which amplitude hits 1.0

		rollOff(.000001); // not quite sure what this does
  	
  		currentPos = 0;
  	
      	stringstream ss;
   		ss << mod;
      	string sfile = "rim/"+ss.str()+".wav";
      	
    	play.load(sfile.c_str());

		// generate the generalized Fibonacci sequence modulo n
		if (mod == 1)  seq.push_back(0);
		else {
		  	seq.push_back(g0);
			seq.push_back(g1);
			seq.push_back(g0+g1);
			int j = 3, v;
			while (!((seq[j-2] == seq[0]) && (seq[j-1] == seq[1]))) {
				v = (seq[j-1] + seq[j-2]) % mod;
				seq.push_back(v); 
				j++;
			}
			seq.erase(seq.end()-3, seq.end()-1);
		}
		
		// calculate restricted period and number of restricted subsequences (for standard Fibonacci only)
		resPeriod = seq.size();
		for (int i = 0 ; i < (int)seq.size() ; i++) if (seq.at(i) == 0 && i > 0) {resPeriod = i; break;}
		resSubseqs = 0;
		for (int i = 0 ; i < (int)seq.size() ; i++) if (seq.at(i) == 0) {resSubseqs++;}
		
		// calculate the number of unique residues in the sequence
		list<int> f_l((int)seq.size());
		for (int i = 0 ; i < (int)seq.size() ; i++) f_l.push_back(seq[i]);
		f_l.sort(); f_l.unique();
		unique = f_l.size();
		
		// calculate apothem for regular polygon, plus cube radius
		apothem = 0.5*tan(M_PI_2-M_PI/seq.size())+0.5;
		if (mod == 1) apothem = 0.0;
		
		// calculate radius for regular polygon, plus altitude in between cubes
		radius = 1.0/(2.0*sin(M_PI/seq.size()))+sin(M_PI-(M_PI_2/seq.size())/2.0-M_PI/2.0);
		if (mod == 1) radius = sqrt(2.0)/2.0;
		
		// initialize the turn variable
		turn = 0;
	
		// add a cube to the mesh
		addCube(m, false, 0.5);
		m.primitive(Graphics::TRIANGLES);
		m.generateNormals();

	}
  
  	
  	virtual void onProcess(AudioIOData& io) {

		while (io()) {
			turn = play.pos();
			currentPos = floor((turn/176400.0)*getPeriod());
			writeSample(play()*level);
		}

	}
	
	virtual void onUpdateNav(){
		Pose p;
		p.pos(getPosition() + Vec3f(0.0, currentMemberFloat(), apothem));
		SoundSource::pose(p);
	}


    float currentMemberFloat();
    
  	void setPosition(Vec3f);
  	Vec3f getPosition();
  
    int getPeriod();
    int getUnique();
    int getModulus();
    float getApothem();
    float getRadius(); 
    int getRestrictedPeriod();
    int getRestrictedSubsequences();
    
    void draw(Graphics&);
    
    virtual ~FibSeqModN() {};
    
    static int turn; 
    
  private:
  
    gam::SamplePlayer<float, gam::ipl::Linear, gam::phsInc::Loop> play;
	vector<int> seq;
	const int g0, g1, mod;
	int unique, resPeriod, resSubseqs, currentPos;
    Vec3f seqPos;
    float apothem, radius, level;

    Mesh m;

};

int FibSeqModN::turn = 0;

float FibSeqModN::currentMemberFloat() {
	return (float)seq[currentPos];
}

void FibSeqModN::setPosition(Vec3f seqPosition) {
	this->seqPos = seqPosition;
}
Vec3f FibSeqModN::getPosition() {
	return seqPos;
}
int FibSeqModN::getPeriod() {
	return seq.size();
}
int FibSeqModN::getUnique() {
	return unique;
}
int FibSeqModN::getModulus() {
	return mod;
}
float FibSeqModN::getApothem() {
	return apothem;
}
float FibSeqModN::getRadius() {
	return radius;
}
int FibSeqModN::getRestrictedPeriod() {
	return resPeriod;
}
int FibSeqModN::getRestrictedSubsequences() {
	return resSubseqs;
}


void FibSeqModN::draw(Graphics& g) {
    	
    	
    	
	// this sets the placement of the entire seq
	g.pushMatrix();
	g.translate(seqPos);
	
	// this sets the placement of all the cubes
	g.pushMatrix();
	g.translate(0.0, 0.0, apothem);
		
	float t_fl = 360.0*(turn/(44100.0*4.0));
		
	for (int i = 0; i < (int)seq.size(); i++) {
		g.pushMatrix();
		g.translate(0.0, seq[i], -apothem);
		g.rotate(((float)i/seq.size())*360.0+(360.0/seq.size())/2-t_fl, 0.0, 1.0, 0.0);
		g.translate(0.0, 0, apothem);
		if (t_fl-((float)i/seq.size())*360.0 >= 0.0 && t_fl-((float)i/seq.size())*360.0 < 360.0/seq.size()) {
			g.color(HSV(0, 1, 1));
		}
		else g.color(HSV(0, 0, 1));
		g.draw(m);
		g.popMatrix();
	}
	
	g.popMatrix();
	g.popMatrix();
	
}


// Helper functions:

vector< vector<FibSeqModN*> > categorizeByRestrictedSubseqs(vector<FibSeqModN*> seqs) {

	vector< vector<FibSeqModN*> > categorizedSeqs(3);
	
	for (int i = 0 ; i < (int)seqs.size() ; i++) {
		if (seqs.at(i)->getRestrictedSubsequences() == 1) categorizedSeqs.at(0).push_back(seqs.at(i));
		else if (seqs.at(i)->getRestrictedSubsequences() == 2) categorizedSeqs.at(1).push_back(seqs.at(i));
		else if (seqs.at(i)->getRestrictedSubsequences() == 4) categorizedSeqs.at(2).push_back(seqs.at(i));
	}
	
	return categorizedSeqs;
}

vector<FibSeqModN*> orderByPeriodLength(vector<FibSeqModN*> seqs) {

	vector<FibSeqModN*> reorderedSeqs;

	vector<int> periods;
	for (int i = 0 ; i < (int)seqs.size() ; i++) periods.push_back(seqs.at(i)->getPeriod());
	sort(periods.begin(), periods.end());
	
	vector<int>::iterator it;
    it = unique(periods.begin(), periods.end());
	periods.resize( distance(periods.begin(),it) ); 
	
	for (int i = 0 ; i < (int)periods.size() ; i++) {
		for (int j = 0 ; j < (int)seqs.size() ; j++) {
			if (seqs.at(j)->getPeriod() == periods.at(i)) reorderedSeqs.push_back(seqs.at(j));
		} 
	}

	return reorderedSeqs;

}

float getLargestRadius(vector<FibSeqModN*> seqs) {
	
	float largest = 0.0;
	
	for (int i = 0 ; i < (int)seqs.size() ; i++) {
		if (seqs.at(i)->getRadius() > largest) largest = seqs.at(i)->getRadius();
	}

	return largest;
	
}
#endif
