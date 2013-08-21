#include "alloutil/al_OmniApp.hpp"
#include "allocore/al_Allocore.hpp"
#include <assert.h>
#include <cmath>
#include <vector>
#include "fibSeqModN.h"
using namespace al;

const int G0 = 0;
const int G1 = 1;
const int NUMSEQS = 34;
const float GAP = 10.5;

static std::string host = Socket::hostName();

Listener * listener;
Nav navMaster(Vec3d(0,0,-40), 0.95);
Stereographic stereo;
#define AUDIO_BLOCK_SIZE 256

AudioScene scene(3, 3, AUDIO_BLOCK_SIZE);
const int numSpeakers = 54;
const double topEl = 30.0;
const double botEl = -30.0;
Speaker speakers[numSpeakers] = {
	Speaker( 1-1, 270.0, topEl),
	Speaker( 2-1, 300.0, topEl),
	Speaker(3-1, 330.0, topEl),
	Speaker(4-1, 0.0, topEl),
	Speaker(5-1,30.0, topEl),
	Speaker(6-1,60.0, topEl),
	Speaker(7-1,90.0, topEl),
	Speaker(8-1,120.0, topEl),
	Speaker(9-1,150.0, topEl),
	Speaker(10-1,180.0, topEl),
	Speaker(11-1,210.0, topEl),
	Speaker(12-1,240.0, topEl),
	
	Speaker(13-1, 288.006994221339, 0.0),
	Speaker(14-1, 299.02687287475, 0.0),
	Speaker(15-1, 309.69049620981, 0.0),
	Speaker(16-1, 320.070952103726, 0.0),
	Speaker(17-1, 330.234430050552, 0.0),
	Speaker(18-1, 340.240917285025, 0.0),
	Speaker(19-1, 350.145513893009, 0.0),
	Speaker(20-1, 0.0, 0.0),
	Speaker(21-1, 9.85448610699111, 0.0),
	Speaker(22-1, 19.7590827149751, 0.0),
	Speaker(23-1, 29.7655699494475, 0.0),
	Speaker(24-1, 39.9290478962744, 0.0),
	Speaker(25-1, 50.3095037901896, 0.0),
	Speaker(26-1, 60.9731271252495, 0.0),
	Speaker(27-1, 71.9930057786609, 0.0),
	Speaker(28-1, 108.006994221339, 0.0),
	Speaker(29-1, 119.02687287475, 0.0),
	Speaker(30-1, 129.69049620981, 0.0),
	Speaker(31-1, 140.070952103726, 0.0),
	Speaker(32-1, 150.234430050552, 0.0),
	Speaker(33-1, 160.240917285025, 0.0),
	Speaker(34-1, 170.145513893009, 0.0),
	Speaker(35-1, 180.0, 0.0),
	Speaker(36-1, 189.854486106991, 0.0),
	Speaker(37-1, 199.759082714975, 0.0),
	Speaker(38-1, 209.765569949448, 0.0),
	Speaker(39-1, 219.929047896274, 0.0),
	Speaker(40-1, 230.30950379019, 0.0),
	Speaker(41-1, 240.97312712525, 0.0),
	Speaker(42-1, 251.993005778661, 0.0),
	
	Speaker(43-1, 270.0, botEl),
	Speaker(44-1, 300.0, botEl),
	Speaker(45-1, 330.0, botEl),
	Speaker(46-1, 0.0, botEl),
	Speaker(47-1,30.0, botEl),
	Speaker(48-1,60.0, botEl),
	Speaker(49-1,90.0, botEl),
	Speaker(50-1,120.0, botEl),
	Speaker(51-1,150.0, botEl),
	Speaker(52-1,180.0, botEl),
	Speaker(53-1,210.0, botEl),
	Speaker(54-1,240.0, botEl),
};


struct PacketData{
	PacketData(): location_x(1), location_y(1), location_z(1), turn(1), quat_w(1), quat_x(1), quat_y(1), quat_z(1){}
	float location_x;
	float location_y;
	float location_z;
	int turn;
	float quat_w;
	float quat_x;
	float quat_y;
	float quat_z;
	
	void clear(){ location_x=0; location_y=0; location_z=0; turn=0; quat_w = 0; quat_x = 0; quat_y = 0; quat_z = 0; }
	bool valid() const { return true; }
	void print() const { printf("%g %g %g %x %g %g %g %g\n", location_x, location_y, location_z, turn, quat_w, quat_x, quat_y, quat_z); }
};

// global variables for state synchronization
float l_x, l_y, l_z, q_w, q_x, q_y, q_z;
int t;

struct OSCHandler : public osc::PacketHandler{
	void onMessage(osc::Message& m){
	
		assert(m.typeTags() == "fffiffff");
		assert(m.addressPattern() == "/test");

		std::string s;
		PacketData d;

		d.clear();
		m >> d.location_x >> d.location_y >> d.location_z >> d.turn >> d.quat_w >> d.quat_x >> d.quat_y >> d.quat_z;

		l_x = d.location_x;
		l_y = d.location_y;
		l_z = d.location_z;
		t = d.turn;
		q_w = d.quat_w;
		q_x = d.quat_x;
		q_y = d.quat_y;
		q_z = d.quat_z;
		
		assert(d.valid());
	}
} handler;


struct MyApp : OmniApp {
  bool iAmTheMaster;
  Material material;
  Light light;

  vector<Vec3f> seqPosition;
  vector<FibSeqModN*> fibSeqModN;
  vector< vector<FibSeqModN*> > reorderedSeqs;
	
  vector< vector<int> > seqs;
	
  vector<osc::Send*> s;
  osc::Recv* r;
	
  PacketData data;

  MyApp() {
  
    cout << "My hostname is " << host << endl;
    if (host == "bossanova.local") {
    
    	audioIO().deviceOut(AudioDevice("AF12 x5b"));
    	audioIO().channelsOut(60);
    
    	audioIO().print();
    
		cout << "I am the master!" << endl;
		iAmTheMaster = true;
	
		s.resize(12);
		s[0] = new osc::Send(11111, "spherez01.mat.ucsb.edu");
		s[1] = new osc::Send(11111, "spherez02.mat.ucsb.edu");
		s[2] = new osc::Send(11111, "spherez03.mat.ucsb.edu");
		s[3] = new osc::Send(11111, "spherez04.mat.ucsb.edu");
		s[4] = new osc::Send(11111, "spherez05.mat.ucsb.edu");
		s[5] = new osc::Send(11111, "spherez07.mat.ucsb.edu");
		s[6] = new osc::Send(11111, "gr01.mat.ucsb.edu");
		s[7] = new osc::Send(11111, "gr04.mat.ucsb.edu");
		s[8] = new osc::Send(11111, "gr05.mat.ucsb.edu");
		s[9] = new osc::Send(11111, "gr06.mat.ucsb.edu");
		s[10] = new osc::Send(11111, "gr08.mat.ucsb.edu");
		s[11] = new osc::Send(11111, "photon.mat.ucsb.edu");
		  
    } else {
		cout << "I am not the master." << endl;
		iAmTheMaster = false;

		r = new osc::Recv(11111);
		r->handler(handler);

		r->timeout(0.1); 
		r->start();
    
    }


    lens().far(600.0); 
    nav().pos(0, 0, 0);

	for (int i = 0 ; i < NUMSEQS ; i++) {
		seqPosition.push_back(Vec3f(0.0, 0.0, 0.0));
		fibSeqModN.push_back(new FibSeqModN(seqPosition[i], G0, G1, i+1));
	}

	reorderedSeqs = categorizeByRestrictedSubseqs(fibSeqModN);

	reorderedSeqs.at(0) = orderByPeriodLength(reorderedSeqs.at(0));
	reorderedSeqs.at(1) = orderByPeriodLength(reorderedSeqs.at(1));
	reorderedSeqs.at(2) = orderByPeriodLength(reorderedSeqs.at(2));

	float radius0 = 0.0;
	float largestRadius0 = getLargestRadius(reorderedSeqs.at(0))+getLargestRadius(reorderedSeqs.at(1));
	for (int i = 0 ; i < (int)reorderedSeqs.at(0).size() ; i++) {
		reorderedSeqs.at(0).at(i)->setPosition(Vec3f(-largestRadius0, 0.0, radius0));
		if (i < (int)reorderedSeqs.at(0).size()-1) radius0 += reorderedSeqs.at(0).at(i)->getRadius()+reorderedSeqs.at(0).at(i+1)->getRadius()+GAP;
		scene.addSource(*reorderedSeqs.at(0).at(i));
	}

	float radius1 = 0.0;
	for (int i = 0 ; i < (int)reorderedSeqs.at(1).size() ; i++) {
		reorderedSeqs.at(1).at(i)->setPosition(Vec3f(0.0, 0.0, radius1));
		if (i < (int)reorderedSeqs.at(1).size()-1) radius1 += reorderedSeqs.at(1).at(i)->getRadius()+reorderedSeqs.at(1).at(i+1)->getRadius()+GAP;
		scene.addSource(*reorderedSeqs.at(1).at(i));
	}

	float radius2 = 0.0;
	float largestRadius2 = getLargestRadius(reorderedSeqs.at(1))+getLargestRadius(reorderedSeqs.at(2));
	for (int i = 0 ; i < (int)reorderedSeqs.at(2).size() ; i++) {
		reorderedSeqs.at(2).at(i)->setPosition(Vec3f(largestRadius2, 0.0, radius2));
		if (i < (int)reorderedSeqs.at(2).size()-1) radius2 += reorderedSeqs.at(2).at(i)->getRadius()+reorderedSeqs.at(2).at(i+1)->getRadius()+GAP;
		scene.addSource(*reorderedSeqs.at(2).at(i));
	}

	listener = scene.createListener(2);

	listener->numSpeakers(numSpeakers);
	for(int i=0; i<numSpeakers; ++i){
		listener->speakerPos(
			i,
			speakers[i].deviceChannel,
			speakers[i].azimuth,
			speakers[i].elevation
		);
	}

	gam::Sync::master().spu(audioIO().fps());
	initWindow();
	initAudio();
  }

  virtual ~MyApp() {}

  virtual void onAnimate(al_sec dt) {

    if (iAmTheMaster) {
    
		data.location_x = nav().pos()[0];
		data.location_y = nav().pos()[1];
		data.location_z = nav().pos()[2];
		data.turn = FibSeqModN::turn;
		data.quat_w = nav().quat()[0];
		data.quat_x = nav().quat()[1];
		data.quat_y = nav().quat()[2];
		data.quat_z = nav().quat()[3];
		
		for (int i = 0 ; i < 12 ; i++) {
			s[i]->clear();
			s[i]->beginBundle(0);
			s[i]->beginMessage("/test");
			*s[i] << data.location_x << data.location_y << data.location_z << data.turn << data.quat_w << data.quat_x << data.quat_y << data.quat_z;
			s[i]->endMessage();
			s[i]->endBundle();
			s[i]->send();
		}
		
    } else {
    	  
		nav().pos(l_x, l_y, l_z);
		FibSeqModN::turn = t;
		nav().quat()[0] = q_w;
		nav().quat()[1] = q_x;
		nav().quat()[2] = q_y;
		nav().quat()[3] = q_z;
    		
    }
  }
  
  virtual void onDraw(Graphics& g) {
	material();
	light();
	
	light.pos(nav().pos());
	
	for (int i = 0 ; i < (int)reorderedSeqs.at(0).size() ; i++) reorderedSeqs.at(0).at(i)->draw(g);
	for (int i = 0 ; i < (int)reorderedSeqs.at(1).size() ; i++) reorderedSeqs.at(1).at(i)->draw(g);
	for (int i = 0 ; i < (int)reorderedSeqs.at(2).size() ; i++) reorderedSeqs.at(2).at(i)->draw(g);
		
  }

  virtual void onSound(al::AudioIOData& io) {
    if (iAmTheMaster) {
    
    	int numFrames = io.framesPerBuffer();

		for (int i = 0 ; i < (int)reorderedSeqs.at(0).size() ; i++) {
			io.frame(0);
			reorderedSeqs.at(0).at(i)->onUpdateNav();
			reorderedSeqs.at(0).at(i)->onProcess(io);
		}
		for (int i = 0 ; i < (int)reorderedSeqs.at(1).size() ; i++) {
			io.frame(0);
			reorderedSeqs.at(1).at(i)->onUpdateNav();
			reorderedSeqs.at(1).at(i)->onProcess(io);
		}
		for (int i = 0 ; i < (int)reorderedSeqs.at(2).size() ; i++) {
			io.frame(0);
			reorderedSeqs.at(2).at(i)->onUpdateNav();
			reorderedSeqs.at(2).at(i)->onProcess(io);
		}
		
		navMaster.step(0.5);

		listener->pose(nav());

		scene.encode(numFrames, io.framesPerSecond());
		scene.render(&io.out(0,0), numFrames);
		
	}
  }

  virtual bool onKeyDown(const Keyboard& k){
    return true;
  }
};

int main(int argc, char * argv[]) {

    MyApp().start();
    return 0;
}
