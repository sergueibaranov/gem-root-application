#ifndef __CINT__
#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <sstream>
#include <vector>
#include <cstdint>

#include <TFile.h>
#include <TNtuple.h>
#include <TH2.h>
#include <TProfile.h>
#include <TCanvas.h>
#include <TFrame.h>
#include <TROOT.h>
#include <TSystem.h>
#include <TRandom3.h>
#include <TBenchmark.h>
#include <TInterpreter.h>
#include <TApplication.h>
#include <TString.h>

using namespace std;

class VFAT2Data {
  public:

    struct ChannelData {
      uint64_t lsData;  // ch1to64
      uint64_t msData;  // ch65to128
      double delVT;   // deviceVT2-deviceVT1, Threshold Scan needs
    };

    struct VFATEvent {
      uint16_t BC;      // 1010:4, BC:12 
      uint16_t EC;      // 1100:4, EC:8, Flags:4
      uint32_t bxExp;   // :28
      uint16_t bxNum;   // :6, Sbit:6
      uint16_t ChipID;  // 1110, :12
      ChannelData data;
      uint16_t crc;     // :16
    };    

    struct GEMEvent {
      uint32_t header1;
      std::vector<VFATEvent> vfats;
      uint32_t trailer1;
    } GEMEvent;
        
    struct AppHeader {  
      int minTh;
      int maxTh;
      int stepSize;
    };

    bool Print(int event, const VFATEvent& ev, const ChannelData& ch){
      if( event<0 ) return(false);
      cout << "\nevent " << event << endl;
      cout << hex << ev.BC << dec << endl;
      cout << hex << ev.EC << dec << endl;
      cout << hex << ev.bxExp << dec << endl;
      cout << hex << ev.bxNum << dec << endl;
      cout << hex << ev.ChipID << dec << endl;
      cout << hex << ch.lsData << dec << endl;
      cout << hex << ch.msData << dec << endl;
      cout << hex << ev.crc << dec << endl;
      cout << ch.delVT << endl;
    };

    bool PrintChipID(int event, const VFATEvent& ev, const ChannelData& ch){
      if( event<0 ) return(false);
      cout << "\nevent " << event << endl;
      uint8_t bitsE = ((ev.ChipID&0xF000)>>12);
      showbits(bitsE);
      cout << hex << "1110 0x0" << ((ev.ChipID&0xF000)>>12) << " ChipID 0x" << (ev.ChipID&0x0FFF) << dec << endl;
    };

    bool readData(ifstream& inpf, int event, ChannelData& ch){
      if(event<0) return(false);
      inpf >> hex >> ch.lsData;
      inpf >> hex >> ch.msData;
      inpf >> ch.delVT;
      return(true);
    };	  

    bool readEvent(ifstream& inpf, int event, VFATEvent& ev, ChannelData& ch){
      if(event<0) return(false);
        inpf >> hex >> ev.BC;
        inpf >> hex >> ev.EC;
        inpf >> hex >> ev.bxExp;
        inpf >> hex >> ev.bxNum;
        inpf >> hex >> ev.ChipID;
        readData (inpf, event, ch);
        inpf >> hex >> ev.crc;
      return(true);
      };	  

    bool readHeader(ifstream& inpf, AppHeader& ah){
      inpf >> ah.minTh;
      inpf >> ah.maxTh;
      inpf >> ah.stepSize;
      return(true);
    };	  

    void showbits(uint8_t x)
    { int i; 
        for(i=(sizeof(uint8_t)*8)-1; i>=0; i--)
          (x&(1<<i))?putchar('1'):putchar('0');
        printf("\n");
    };
};



TROOT root("",""); // static TROOT object

int main(int argc, char** argv)
#else
TFile* thldread(Int_t get=0)
#endif
{ cout<<"---> Main()"<<endl;

#ifndef __CINT__
  TApplication App("App", &argc, argv);
#endif

  VFAT2Data data;
  VFAT2Data::ChannelData ch;
  VFAT2Data::VFATEvent ev;
  VFAT2Data::AppHeader ah;

  string file="ThresholdScan.dat";

  ifstream inpf(file.c_str());
  if(!inpf.is_open()) {
    cout << "\nThe file: " << file.c_str() << " is missing.\n" << endl;
    return 0;
  };

  /* Threshould Analysis Histograms */
  const TString filename = "thldread.root";

  TFile* hfile = NULL;
  hfile = new TFile(filename,"RECREATE","Threshold Scan ROOT file with histograms");

  // read Scan Header 
  data.readHeader(inpf, ah);

  Int_t nBins = ((ah.maxTh - ah.minTh) + 1)/ah.stepSize;

  cout << " minTh " << ah.minTh << " maxTh " << ah.maxTh << " nBins " << nBins << endl;

  TH1F* histo = new TH1F("allchannels", "Threshold scan for all channels", nBins, (Double_t)ah.minTh-0.5,(Double_t)ah.maxTh+0.5 );

  histo->SetFillColor(48);

  // Create a new canvas.
  TCanvas *c1 = new TCanvas("c1","Dynamic Filling Example",50,50,500,500);

  c1->SetFillColor(42);
  c1->GetFrame()->SetFillColor(21);
  c1->GetFrame()->SetBorderSize(6);
  c1->GetFrame()->SetBorderMode(-1);
  c1->Divide(1,1);

  // Booking of 128 histograms for each VFAT2 channel 
  stringstream histName, histTitle;
  TH1F* histos[128];

  for (unsigned int hi = 0; hi < 128; ++hi) {
    
    histName.clear();
    histName.str(std::string());
    histTitle.clear();
    histTitle.str(std::string());

    histName  << "channel"<<(hi+1);
    histTitle << "Threshold scan for channel "<<(hi+1);
    histos[hi] = new TH1F(histName.str().c_str(), histTitle.str().c_str(), nBins, (Double_t)ah.minTh-0.5,(Double_t)ah.maxTh+0.5);
  }

  Int_t ieventMax=100000;
  const Int_t kUPDATE = 1000;
  uint8_t bits1110 = 0x00;

  for(int ievent=0; ievent<ieventMax; ievent++){

    if(inpf.eof()) break;
    if(!inpf.good()) break;

    data.readEvent(inpf, ievent, ev, ch);
    //data.Print(ievent, ev, ch);
    //data.PrintChipID(ievent, ev, ch);

    // cout << "delVT " << ch.delVT << " " << dec << (ch.lsData||ch.msData) << dec << endl;

    histo->Fill(ch.delVT, (ch.lsData||ch.msData));

    //I think it would be nice to time this...
    for (int chan = 0; chan < 128; ++chan) {
      if (chan < 64)
	histos[chan]->Fill(ch.delVT,((ch.lsData>>chan))&0x1);
      else
	histos[chan]->Fill(ch.delVT,((ch.msData>>(chan-64)))&0x1);
    }

    if (ievent%kUPDATE == 0 && ievent != 0) {
      cout << " ievent " << ievent << " ievent%kUPDATE " << ievent%kUPDATE << endl;
      c1->cd(1);
      histo->Draw();
      c1->Update();
    }

  }
  inpf.close();

  // Save all objects in this file
  hfile->Write();
  cout<<"=== hfile->Write()"<<endl;

#ifndef __CINT__
     App.Run();
#endif

#ifdef __CINT__
   return hfile;
#else
   return 0;
#endif

}
