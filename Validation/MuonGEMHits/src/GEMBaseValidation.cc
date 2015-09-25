#include "Validation/MuonGEMHits/interface/GEMBaseValidation.h"
#include "DataFormats/Common/interface/Handle.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"
#include "DataFormats/GEMDigi/interface/GEMDigiCollection.h"
#include "DQMServices/Core/interface/DQMStore.h"

using namespace std;
GEMBaseValidation::GEMBaseValidation( const edm::ParameterSet& ps)
{
  nBinZR_ = ps.getUntrackedParameter< std::vector<double> >("nBinGlobalZR") ;
  RangeZR_ = ps.getUntrackedParameter< std::vector<double> >("RangeGlobalZR");
  nBinXY_ = ps.getUntrackedParameter< int >("nBinGlobalXY",360) ;

  regionLabel.push_back("-1");
  regionLabel.push_back("1" );


  layerLabel.push_back("1");
  layerLabel.push_back("2");
}

const GEMGeometry* GEMBaseValidation::initGeometry(edm::EventSetup const & iSetup) {
  const GEMGeometry* GEMGeometry_ = nullptr;
  try {
    edm::ESHandle<GEMGeometry> hGeom;
    iSetup.get<MuonGeometryRecord>().get(hGeom);
    GEMGeometry_ = &*hGeom;
  }
  catch( edm::eventsetup::NoProxyException<GEMGeometry>& e) {
    edm::LogError("MuonGEMBaseValidation") << "+++ Error : GEM geometry is unavailable on event loop. +++\n";
    return nullptr;
  }
  nregion  = GEMGeometry_->regions().size();
  nstation = GEMGeometry_->regions()[0]->stations().size() ;
  nstationForLabel = GEMGeometry_->regions()[0]->stations().size() ;
  npart    = GEMGeometry_->regions()[0]->stations()[0]->superChambers()[0]->chambers()[0]->etaPartitions().size();

  return GEMGeometry_;
}

string GEMBaseValidation::getSuffixName(int region, int station, int layer){
  if ( region == -1 ) region =0 ;
  else if ( region >1 ) std::cout<<"Name)Alert! Region must be -1 or 1 : "<<region<<" "<<station<<" "<<layer<<std::endl;
  return string("_r")+regionLabel[region]+"_st"+getStationLabel(station)+"_l"+layerLabel[layer-1];
}
string GEMBaseValidation::getSuffixName(int region, int station){
  if ( region == -1 ) region =0 ;
  else if ( region >1 ) std::cout<<"Name)Alert! Region must be -1 or 1 : "<<region<<" "<<station<<std::endl;
  return string("_r")+regionLabel[region]+"_st"+getStationLabel(station);
}
string GEMBaseValidation::getSuffixName(int region){
  if ( region == -1 ) region =0 ;
  else if ( region >1 ) std::cout<<"Name)Alert! Region must be -1 or 1 : "<<region<<std::endl;
  return string("_r")+regionLabel[region];
}

string GEMBaseValidation::getSuffixTitle(int region, int station, int layer){
  if ( region == -1 ) region =0 ;
  else if ( region >1 ) std::cout<<"Title)Alert! Region must be -1 or 1 : "<<region<<" "<<station<<" "<<layer<<std::endl;
  return string("Region ")+regionLabel[region]+" Station "+getStationLabel(station)+" Layer "+layerLabel[layer-1];
}
string GEMBaseValidation::getSuffixTitle(int region, int station){
  if ( region == -1 ) region =0 ;
  else if ( region >1 ) std::cout<<"Title)Alert! Region must be -1 or 1 : "<<region<<" "<<station<<std::endl;
  return string("Region ")+regionLabel[region]+" Station "+getStationLabel(station);
}
string GEMBaseValidation::getSuffixTitle(int region){
  if ( region == -1 ) region =0 ;
  else if ( region >1 ) std::cout<<"Title)Alert! Region must be -1 or 1 : "<<region<<std::endl;
  return string("Region ")+regionLabel[region];
}

string GEMBaseValidation::getStationLabel(int i) {
  vector<string> stationLabel;
  if ( nstationForLabel == 2) { 
    string stationLabel[] = {"1","2"};
    return stationLabel[i-1];
  }
  else if ( nstationForLabel ==3 ) {
    string stationLabel[] = {"1","2s","2l"};
    return stationLabel[i-1];
  }
  else {
    std::cout<<"Something is wrong"<<std::endl;
    return "";
  }
}




GEMBaseValidation::~GEMBaseValidation() {
}

TH2F* GEMBaseValidation::getSimpleZR() {
    std::vector<double> xbins_vector;
    //std::vector<const char*> xbins_label;
    double station1_xmin = RangeZR_[ 0 ];
    double station1_xmax = RangeZR_[ 1 ];
    double station2_xmin = RangeZR_[ 4 ];
    double station2_xmax = RangeZR_[ 5 ];

    for( double i= station1_xmin-1 ; i< station2_xmax+1; i=i+0.25  ) {
      if ( i > station1_xmax+1 && i<station2_xmin-1 ) continue; 
      xbins_vector.push_back(i);
    }
    TH2F* simpleZR_templ = new TH2F("","", xbins_vector.size()-1, (double*)&xbins_vector[0], 50,120,330);
    return simpleZR_templ;
}

MonitorElement* GEMBaseValidation::BookHistZR( DQMStore::IBooker& ibooker, const char* name, const char* label, unsigned int region_num, unsigned int station_num, unsigned int layer_num) {
  string hist_name, hist_title;
  if ( layer_num == 0 || layer_num==1 ) {
    hist_name  = name+string("_zr") + getSuffixName(region_num, station_num+1, layer_num+1);
    hist_title = label+string(" occupancy : region")+getSuffixTitle( region_num, station_num+1, layer_num+1)+" ; globalZ[cm] ; globalR[cm]";
  }
  else {
    hist_name  = name+string("_zr") + getSuffixName(region_num, station_num+1);
    hist_title = label+string(" occupancy : region")+getSuffixTitle( region_num, station_num+1)+" ; globalZ[cm] ; globalR[cm]";
  }
  LogDebug("GEMBaseValidation")<<hist_name<<" "<<hist_title<<std::endl;
  int xbin = (int)nBinZR_[station_num]; 
  int ybin = (int)nBinZR_[ nBinZR_.size()/2+station_num];
  double xmin = 0;
  double xmax = 0; 
  double ymin = 0;
  double ymax = 0;
  ymin = RangeZR_[ RangeZR_.size()/2 + station_num*2 + 0]; 
  ymax = RangeZR_[ RangeZR_.size()/2 + station_num*2 + 1]; 
  if ( region_num ==0 ) {
    xmin = -RangeZR_[ station_num*2 + 1];
    xmax = -RangeZR_[ station_num*2 + 0];
  }
  else {
    xmin = RangeZR_[ station_num*2 + 0];
    xmax = RangeZR_[ station_num*2 + 1];
  }
  return ibooker.book2D( hist_name, hist_title, xbin, xmin, xmax, ybin,ymin, ymax);
}

MonitorElement* GEMBaseValidation::BookHistXY( DQMStore::IBooker& ibooker, const char* name, const char* label, unsigned int region_num, unsigned int station_num, unsigned int layer_num) {
  string hist_name, hist_title;
  if ( layer_num == 0 || layer_num==1 ) {
    hist_name  = name+string("_xy") + getSuffixName( region_num, station_num+1, layer_num+1) ;
    hist_title = label+string(" occupancy : ")+getSuffixTitle( region_num, station_num+1, layer_num+1 )+ " ; globalX [cm]; globalY[cm]"; 
  }
  else {
    hist_name  = name+string("_xy") + getSuffixName( region_num, station_num+1);
    hist_title = label+string(" occupancy : region")+getSuffixTitle( region_num, station_num+1) +" ; globalX [cm]; globalY[cm]";
  } 
  return ibooker.book2D( hist_name, hist_title, nBinXY_, -360,360,nBinXY_,-360,360); 
}

