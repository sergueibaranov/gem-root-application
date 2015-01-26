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


/**
* ... Threshold Scan ROOT based application, could be used for analisys of XDAQ GEM data ...
*/

/*! \file */
/*! 
  \mainpage Threshold Scan ROOT based application.

  VFAT2 data reading example for Threshold Scan XDAQ GEM application.
  \section intro_sec Introduction

  This is the introduction.

  \section install_sec Installation
  \author Sergey.Baranov@cern.ch
*/

using namespace std;

//! GEM VFAT2 Data class.
/*!
  \brief VFAT2Data
  contents VFAT2 GEM data format 
  \author Sergey.Baranov@cern.ch
*/

class VFAT2Data {
  public:

    //! VFAT2 Channel data.
    /*!
      contents VFAT2 128 channels data in two 64 bits words.
    */

    struct ChannelData {
      uint64_t lsData;  /*!<lsData value, bits from 1to64. */ 
      uint64_t msData;  /*!<msData value, bits from 65to128. */
      double delVT;     /*!<delVT = deviceVT2-deviceVT1, Threshold Scan needs this value. */
    };

    //! GEM Event Data Format (one chip data)
    /*! 
      Uncoding of VFAT2 data for one chip, data format.
      \image html vfat2.data.format.png
      \author Sergey.Baranov@cern.ch
    */

    struct VFATEvent {
      uint16_t BC;      /*!<Banch Crossing number "BC" 16 bits, : 1010:4 (control bits), BC:12 */
      uint16_t EC;      /*!<Event Counter "EC" 16 bits: 1100:4(control bits) , EC:8, Flags:4 */
      uint32_t bxExp;   
      uint16_t bxNum;   /*!<Event Number & SBit, 16 bits : bxNum:6, SBit:6 */
      uint16_t ChipID;  /*!<ChipID 16 bits, 1110:4 (control bits), ChipID:12 */
      ChannelData data; /*!<ChannelData channels data */
      uint16_t crc;     /*!<Checksum number, CRC:16 */
    };    

    /*
    struct GEMEvent {
      uint32_t header1;
      std::vector<VFATEvent> vfats;
      uint32_t trailer1;
    } GEMEvent;
    */
   
    //! Application header struct
    /*!
      \brief AppHeader contens Threshold scan parameters
    */

    struct AppHeader {  
      int minTh;     /*!<minTh minimal threshold value. */ 
      int maxTh;     /*!<maxTh maximal threshold value. */ 
      int stepSize;  /*!<stepSize threshold ste size value. */
    };

    //! Print Event, "hex" format.
    /*! 
      Print VFAT2 event.
    */

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

    //! Print ChipID.
    /*! 
      Print ChipID "hex" number and control bits "1110"
    */

    bool PrintChipID(int event, const VFATEvent& ev, const ChannelData& ch){
      if( event<0 ) return(false);
      cout << "\nevent " << event << endl;
      uint8_t bitsE = ((ev.ChipID&0xF000)>>12);
      showbits(bitsE);
      cout << hex << "1110 0x0" << ((ev.ChipID&0xF000)>>12) << " ChipID 0x" << (ev.ChipID&0x0FFF) << dec << endl;
    };

    //! Read 1-128 channels data
    /*!
    reading two 64 bits words (lsData & msData) with data from all channels for one VFAT2 chip 
    */

    bool readData(ifstream& inpf, int event, ChannelData& ch){
      if(event<0) return(false);
      inpf >> hex >> ch.lsData;
      inpf >> hex >> ch.msData;
      inpf >> ch.delVT;
      return(true);
    };	  

    //! Read GEM event data
    /*!
      reading GEM VFAT2 data (BC,EC,bxNum,ChipID,(lsData & msData), crc.
    */

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

    //! read Threshold scan header.
    /*!
      reading of Threshold Scan setup header
    */

    bool readHeader(ifstream& inpf, AppHeader& ah){
      inpf >> ah.minTh;
      inpf >> ah.maxTh;
      inpf >> ah.stepSize;
      return(true);
    };	  

    //! showbits function.
    /*!
    show bits function, needs for debugging
    */

    void showbits(uint8_t x)
    { int i; 
        for(i=(sizeof(uint8_t)*8)-1; i>=0; i--)
          (x&(1<<i))?putchar('1'):putchar('0');
        printf("\n");
    };
};

//! root function.
/*!
https://root.cern.ch/drupal/content/documentation
*/

TROOT root("",""); // static TROOT object

//! main function.
/*!
C++ any documents
*/

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
