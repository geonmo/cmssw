#include "Validation/MuonGEMHits/interface/GEMTrackMatch.h"
#include "DataFormats/Common/interface/Handle.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"
#include "DataFormats/GEMDigi/interface/GEMDigiCollection.h"
#include "DQMServices/Core/interface/DQMStore.h"
#include <TMath.h>
#include <TH1F.h>

GEMTrackMatch::GEMTrackMatch(DQMStore* dbe, edm::EDGetToken& simTracks, edm::EDGetToken& simVertices, edm::ParameterSet cfg)
{
   cfg_= cfg; 
   dbe_= dbe;
   useRoll_ = 1 ;
   etaRangeForPhi = cfg_.getUntrackedParameter< std::vector<double> >("EtaRangeForPhi");
   simTracksToken_ = simTracks;
   simVerticesToken_ = simVertices;
}


GEMTrackMatch::~GEMTrackMatch() {
}
std::pair<double,double> GEMTrackMatch::getEtaRange( int station, int chamber ) 
{
  if( theGEMGeometry != nullptr) {
    auto& ch = theGEMGeometry->regions()[0]->stations()[station-1]->rings()[0]->superChambers()[chamber-1]->chambers()[0];
    auto& roll1 = ch->etaPartitions()[0]; //.begin();
    auto& roll2 = ch->etaPartitions()[ch->nEtaPartitions()-1];
    const BoundPlane& bSurface1(roll1->surface());    
    const BoundPlane& bSurface2(roll2->surface());    
    
    auto& parameters1( roll1->specs()->parameters());
    //float bottomEdge1(parameters1[0]);
    //float topEdge1(parameters1[1]);
    float height1(parameters1[2]);

    auto& parameters2( roll2->specs()->parameters());
    //float bottomEdge2(parameters2[0]);
    //float topEdge2(parameters2[1]);
    float height2(parameters2[2]);

    LocalPoint lTop1( 0., height1, 0.);
    GlobalPoint gTop1(bSurface1.toGlobal(lTop1));
    LocalPoint lBottom1( 0., -height1, 0.);
    GlobalPoint gBottom1(bSurface1.toGlobal(lBottom1));

    LocalPoint lTop2( 0., height2, 0.);
    GlobalPoint gTop2(bSurface2.toGlobal(lTop2));
    LocalPoint lBottom2( 0., -height2, 0.);
    GlobalPoint gBottom2(bSurface2.toGlobal(lBottom2));
    
    //std::cout<<"roll1 top : "<<gTop1.eta()<<"  bottom :"<<gBottom1.eta()<<std::endl;
    //std::cout<<"roll2 top : "<<gTop2.eta()<<"  bottom :"<<gBottom2.eta()<<std::endl;

    double eta1 = fabs(gTop1.eta())    - 0.01;
    double eta2 = fabs(gBottom2.eta()) + 0.01;

    return std::make_pair(eta1,eta2);
  }
  else { std::cout<<"Failed to get geometry information"<<std::endl;
    return std::make_pair(0,0);
  }

}

bool GEMTrackMatch::isSimTrackGood(const SimTrack &t)
{

  // SimTrack selection
  if (t.noVertex())   return false; 
  if (t.noGenpart()) return false;
  if (std::abs(t.type()) != 13) return false; // only interested in direct muon simtracks
  if (t.momentum().pt() < minPt_ ) return false;
  const float eta(std::abs(t.momentum().eta()));
  if (eta > maxEta_ || eta < minEta_ ) return false; // no GEMs could be in such eta
  return true;
}


void GEMTrackMatch::buildLUT()
{

  const int maxChamberId_ = theGEMGeometry->regions()[0]->stations()[0]->superChambers().size();
  edm::LogInfo("GEMTrackMatch")<<"max chamber "<<maxChamberId_<<"\n";
  
  std::vector<int> pos_ids;
  pos_ids.push_back(GEMDetId(1,1,1,1,maxChamberId_,useRoll_).rawId());

  std::vector<int> neg_ids;
  neg_ids.push_back(GEMDetId(-1,1,1,1,maxChamberId_,useRoll_).rawId());

  // VK: I would really suggest getting phis from GEMGeometry
  
  std::vector<float> phis;
  phis.push_back(0.);
  for(int i=1; i<maxChamberId_+1; ++i)
  {
    pos_ids.push_back(GEMDetId(1,1,1,1,i,useRoll_).rawId());
    neg_ids.push_back(GEMDetId(-1,1,1,1,i,useRoll_).rawId());
    phis.push_back(i*10.);
  }
  positiveLUT_ = std::make_pair(phis,pos_ids);
  negativeLUT_ = std::make_pair(phis,neg_ids);
}


void GEMTrackMatch::setGeometry(const GEMGeometry* geom)
{
  theGEMGeometry = geom;
	 bool isEvenOK = true;
	 bool isOddOK  = true;
   useRoll_=1;
	 if ( theGEMGeometry->etaPartition( GEMDetId(1,1,1,1,1,1) ) == nullptr) isOddOK = false;
	 if ( theGEMGeometry->etaPartition( GEMDetId(1,1,1,1,2,1) ) == nullptr) isEvenOK = false;
	 if ( !isEvenOK || !isOddOK) useRoll_=2;

  const auto top_chamber = static_cast<const GEMEtaPartition*>(theGEMGeometry->idToDetUnit(GEMDetId(1,1,1,1,1,useRoll_))); 
  const int nEtaPartitions(theGEMGeometry->chamber(GEMDetId(1,1,1,1,1,useRoll_))->nEtaPartitions());
  const auto bottom_chamber = static_cast<const GEMEtaPartition*>(theGEMGeometry->idToDetUnit(GEMDetId(1,1,1,1,1,nEtaPartitions)));
  const float top_half_striplength = top_chamber->specs()->specificTopology().stripLength()/2.;
  const float bottom_half_striplength = bottom_chamber->specs()->specificTopology().stripLength()/2.;
  const LocalPoint lp_top(0., top_half_striplength, 0.);
  const LocalPoint lp_bottom(0., -bottom_half_striplength, 0.);
  const GlobalPoint gp_top = top_chamber->toGlobal(lp_top);
  const GlobalPoint gp_bottom = bottom_chamber->toGlobal(lp_bottom);

  radiusCenter_ = (gp_bottom.perp() + gp_top.perp())/2.;
  chamberHeight_ = gp_top.perp() - gp_bottom.perp();

  buildLUT();
}  


std::pair<int,int> GEMTrackMatch::getClosestChambers(int region, float phi)
{
  const int maxChamberId_ = theGEMGeometry->regions()[0]->stations()[0]->superChambers().size();
  auto& phis(positiveLUT_.first);
  auto upper = std::upper_bound(phis.begin(), phis.end(), phi);
  auto& LUT = (region == 1 ? positiveLUT_.second : negativeLUT_.second);
  return std::make_pair(LUT.at(upper - phis.begin()), (LUT.at((upper - phis.begin() + 1)%maxChamberId_)));
}

std::pair<double, double> GEMTrackMatch::getEtaRangeForPhi( int station ) 
{
	std::pair<double, double> range;
	if( station== 0 )      range = std::make_pair( etaRangeForPhi[0],etaRangeForPhi[1]) ; 
	else if( station== 1 ) range = std::make_pair( etaRangeForPhi[2],etaRangeForPhi[3]) ; 
	else if( station== 2 ) range = std::make_pair( etaRangeForPhi[4],etaRangeForPhi[5]) ; 

	return range;
}

