#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/EDFilter.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/ESHandle.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/Utilities/interface/InputTag.h"

#include "DataFormats/Common/interface/Handle.h"
#include "DataFormats/VertexReco/interface/Vertex.h"
#include "DataFormats/VertexReco/interface/VertexFwd.h"
#include "DataFormats/TrackReco/interface/Track.h"
#include "DataFormats/PatCandidates/interface/Muon.h"

#include "RecoVertex/KalmanVertexFit/interface/KalmanVertexFitter.h"
#include "DataFormats/Candidate/interface/VertexCompositeCandidate.h"
#include "CommonTools/CandUtils/interface/AddFourMomenta.h"

#include "TrackingTools/Records/interface/TransientTrackRecord.h"
#include "TrackingTools/PatternTools/interface/ClosestApproachInRPhi.h"
#include "TrackingTools/TransientTrack/interface/TransientTrackBuilder.h"

//#define DEBUGPLOT

#ifdef DEBUGPLOT
#include "TH1F.h"
#include "TH2F.h"
#include "FWCore/ServiceRegistry/interface/Service.h"
#include "CommonTools/UtilAlgos/interface/TFileService.h"
#endif


class KJpsiToMuMuProducer : public edm::EDFilter
{
public:
  KJpsiToMuMuProducer(const edm::ParameterSet& pset);
  ~KJpsiToMuMuProducer() {};

  bool filter(edm::Event& event, const edm::EventSetup& eventSetup);

private:
  bool isGoodTrack(const reco::TrackRef& track, const GlobalPoint& pvPoint) const;
  reco::TransientTrack GetTransientTrack( edm::ESHandle<TransientTrackBuilder> theB, pat::Muon muon, bool& trig);

private:
  constexpr static double muonMass_ = 0.1056583715;
  edm::EDGetTokenT<std::vector<pat::Muon> > muonToken_;
  edm::EDGetTokenT<reco::VertexCollection> goodPrimaryVertexToken_;

  unsigned int pdgId_;
  double rawMassMin_, rawMassMax_, massMin_, massMax_;

  double cut_minPt_, cut_maxEta_;
  double cut_trackChi2_, cut_trackSignif_, cut_DCA_;
  int cut_trackNHit_;
  double cut_vertexChi2_, cut_minLxy_, cut_maxLxy_, cut_vtxSignif_;

  //double muonDPt_, muonDR_;

  unsigned int minNumber_, maxNumber_;

  //const TrackerGeometry* trackerGeom_;
#ifdef DEBUGPLOT
  TH1F* hRawMass_, * hFitMass_;
  TH2F* hRawMassVsFitMass_;
#endif
};
KJpsiToMuMuProducer::KJpsiToMuMuProducer(const edm::ParameterSet& pset)
{
  muonToken_ = consumes<std::vector<pat::Muon> >(pset.getParameter<edm::InputTag>("src"));
  goodPrimaryVertexToken_ = consumes<reco::VertexCollection>(pset.getParameter<edm::InputTag>("primaryVertex"));

  edm::ParameterSet trackPSet = pset.getParameter<edm::ParameterSet>("track");
  cut_minPt_ = trackPSet.getParameter<double>("minPt");
  cut_maxEta_ = trackPSet.getParameter<double>("maxEta");
  cut_trackChi2_ = trackPSet.getParameter<double>("chi2");
  cut_trackNHit_  = trackPSet.getParameter<int>("nHit");
  cut_trackSignif_ = trackPSet.getParameter<double>("signif");
  cut_DCA_ = trackPSet.getParameter<double>("DCA");

  edm::ParameterSet vertexPSet = pset.getParameter<edm::ParameterSet>("vertex");
  cut_vertexChi2_ = vertexPSet.getParameter<double>("chi2");
  cut_minLxy_ = vertexPSet.getParameter<double>("minLxy");
  cut_maxLxy_ = vertexPSet.getParameter<double>("maxLxy");
  cut_vtxSignif_ = vertexPSet.getParameter<double>("signif");

  pdgId_ = pset.getParameter<unsigned int>("pdgId");
  rawMassMin_ = pset.getParameter<double>("rawMassMin");
  rawMassMax_ = pset.getParameter<double>("rawMassMax");
  massMin_ = pset.getParameter<double>("massMin");
  massMax_ = pset.getParameter<double>("massMax");

  minNumber_ = pset.getParameter<unsigned int>("minNumber");
  maxNumber_ = pset.getParameter<unsigned int>("maxNumber");

  produces<reco::VertexCompositeCandidateCollection>();
  produces<std::vector<double> >("lxy");
  produces<std::vector<double> >("l3D");

#ifdef DEBUGPLOT
  edm::Service<TFileService> fs;
  hRawMass_ = fs->make<TH1F>("hRawMass", "raw mass;Raw mass (GeV/c^{2});Entries", 100, 0, 5);
  hFitMass_ = fs->make<TH1F>("hFitMass", "fit mass;Fit mass (GeV/c^{2});Entries", 100, 0, 5);
  hRawMassVsFitMass_ = fs->make<TH2F>("hRawMassVsFitMass", "raw vs fit;Raw mass (GeV/c^{2};Fit mass (GeV/c^{2}", 100, 0, 5, 100, 0, 5);
#endif
}

bool KJpsiToMuMuProducer::filter(edm::Event& event, const edm::EventSetup& eventSetup)
{
  using namespace reco;
  using namespace edm;
  using namespace std;

  typedef reco::VertexCompositeCandidateCollection VCCandColl;

  std::auto_ptr<VCCandColl> decayCands(new VCCandColl);
  std::auto_ptr<std::vector<double> > decayLengths(new std::vector<double>);
  std::auto_ptr<std::vector<double> > decayLengths3D(new std::vector<double>);
  
  edm::Handle< reco::VertexCollection >  goodPVHandle;
  event.getByToken(goodPrimaryVertexToken_ , goodPVHandle);

  reco::Vertex goodPV;
  if ( goodPVHandle->size() >0 ) goodPV = goodPVHandle->at(0);
  else return false;

  const double pvx = goodPV.position().x();
  const double pvy = goodPV.position().y();
  const double pvz = goodPV.position().z();


  edm::ESHandle<TransientTrackBuilder> theB;
  eventSetup.get<TransientTrackRecord>().get("TransientTrackBuilder",theB);

  edm::Handle<std::vector<pat::Muon> > muonHandle;
  event.getByToken(muonToken_, muonHandle);

  for ( int i=0, n=muonHandle->size(); i<n; ++i )
  {
    const pat::Muon& muon1 = muonHandle->at(i);
    if ( muon1.charge() >= 0 ) continue;
    bool trigger = false;
    bool& trig = trigger;
    auto transTrack1 = GetTransientTrack(theB, muon1, trig);
    if ( trig ) continue;
    if ( !transTrack1.impactPointTSCP().isValid() ) continue;
    FreeTrajectoryState ipState1 = transTrack1.impactPointTSCP().theState();

    for ( int j=0; j<n; ++j )
    {
      const pat::Muon& muon2 = muonHandle->at(j);
      if ( muon2.charge() <= 0 ) continue;
      auto transTrack2= GetTransientTrack(theB, muon2, trig);
      if ( trig ) continue;
      if ( !transTrack2.impactPointTSCP().isValid() ) continue;
      FreeTrajectoryState ipState2 = transTrack2.impactPointTSCP().theState();

      // Measure distance between tracks at their closest approach
      ClosestApproachInRPhi cApp;
      cApp.calculate(ipState1, ipState2);
      if ( !cApp.status() ) continue;
      const float dca = fabs(cApp.distance());
      if ( dca < 0. || dca > cut_DCA_ ) continue;
      GlobalPoint cxPt = cApp.crossingPoint();
      if (std::hypot(cxPt.x(), cxPt.y()) > 120. || std::abs(cxPt.z()) > 300.) continue;
      TrajectoryStateClosestToPoint caState1 = transTrack1.trajectoryStateClosestToPoint(cxPt);
      TrajectoryStateClosestToPoint caState2 = transTrack2.trajectoryStateClosestToPoint(cxPt);
      if ( !caState1.isValid() or !caState2.isValid() ) continue;

      const double rawEnergy = std::hypot(caState1.momentum().mag(), muonMass_)
                             + std::hypot(caState2.momentum().mag(), muonMass_);
      const double rawMass = sqrt(rawEnergy*rawEnergy - (caState1.momentum()+caState2.momentum()).mag2());

#ifdef DEBUGPLOT
      hRawMass_->Fill(rawMass);
#endif
      if ( rawMassMin_ > rawMass or rawMassMax_ < rawMass ) continue;

      // Build Vertex
      std::vector<TransientTrack> transTracks;
      transTracks.push_back(transTrack1);
      transTracks.push_back(transTrack2);
      KalmanVertexFitter fitter(true);
      TransientVertex transVertex = fitter.vertex(transTracks);

      if ( !transVertex.isValid() or transVertex.totalChiSquared() < 0. ) continue;

      const reco::Vertex vertex = transVertex;
      if ( vertex.normalizedChi2() > cut_vertexChi2_ ) continue;

      std::vector<TransientTrack> refittedTracks;
      if ( transVertex.hasRefittedTracks() ) refittedTracks = transVertex.refittedTracks();

      typedef ROOT::Math::SMatrix<double, 3, 3, ROOT::Math::MatRepSym<double, 3> > SMatrixSym3D;
      typedef ROOT::Math::SVector<double, 3> SVector3;

      GlobalPoint vtxPos(vertex.x(), vertex.y(), vertex.z());

      //SMatrixSym3D totalCov = beamSpotHandle->rotatedCovariance3D() + vertex.covariance();
      SMatrixSym3D totalCov = goodPV.covariance() + vertex.covariance();
      SVector3 distanceVectorXY(vertex.x() - pvx, vertex.y() - pvy, 0.);

      double rVtxMag = ROOT::Math::Mag(distanceVectorXY);
      double sigmaRvtxMag = sqrt(ROOT::Math::Similarity(totalCov, distanceVectorXY)) / rVtxMag;
      if( rVtxMag < cut_minLxy_ or rVtxMag > cut_maxLxy_ or rVtxMag / sigmaRvtxMag < cut_vtxSignif_ ) continue;

      SVector3 distanceVector3D(vertex.x() - pvx, vertex.y() - pvy, vertex.z() - pvz);
      const double rVtxMag3D = ROOT::Math::Mag(distanceVector3D);

      // Cuts finished, now we create the candidates and push them back into the collections.

      std::auto_ptr<TrajectoryStateClosestToPoint> traj1;
      std::auto_ptr<TrajectoryStateClosestToPoint> traj2;

      if ( refittedTracks.empty() )
      {
        traj1.reset(new TrajectoryStateClosestToPoint(transTrack1.trajectoryStateClosestToPoint(vtxPos)));
        traj2.reset(new TrajectoryStateClosestToPoint(transTrack2.trajectoryStateClosestToPoint(vtxPos)));
      }
      else
      {
        TransientTrack* refTrack1 = 0, * refTrack2 = 0;
        for ( std::vector<TransientTrack>::iterator refTrack = refittedTracks.begin();
              refTrack != refittedTracks.end(); ++refTrack )
        {
          if ( refTrack->track().charge() < 0 ) refTrack1 = &*refTrack;
          else refTrack2 = &*refTrack;
        }
        if ( refTrack1 == 0 or refTrack2 == 0 ) continue;
        traj1.reset(new TrajectoryStateClosestToPoint(refTrack1->trajectoryStateClosestToPoint(vtxPos)));
        traj2.reset(new TrajectoryStateClosestToPoint(refTrack2->trajectoryStateClosestToPoint(vtxPos)));
      }
      if( !traj1->isValid() or !traj2->isValid() ) continue;

      GlobalVector mom1(traj1->momentum());
      GlobalVector mom2(traj2->momentum());
      GlobalVector mom(mom1+mom2);

      //cleanup stuff we don't need anymore
      traj1.reset();
      traj2.reset();

      Particle::Point vtx(vertex.x(), vertex.y(), vertex.z());
      const Vertex::CovarianceMatrix vtxCov(vertex.covariance());
      double vtxChi2(vertex.chi2());
      double vtxNdof(vertex.ndof());

      const double candE1 = hypot(mom1.mag(), muonMass_);
      const double candE2 = hypot(mom2.mag(), muonMass_);

      Particle::LorentzVector candLVec(mom.x(), mom.y(), mom.z(), candE1+candE2);
#ifdef DEBUGPLOT
      hFitMass_->Fill(candLVec.mass());
      hRawMassVsFitMass_->Fill(rawMass, candLVec.mass());
#endif
      if ( massMin_ > candLVec.mass() or massMax_ < candLVec.mass() ) continue;

      // Match to muons
      pat::Muon newLep1(muon1);
      pat::Muon newLep2(muon2);
      newLep1.setP4(reco::Candidate::LorentzVector(mom1.x(), mom1.y(), mom1.z(), candE1));
      newLep2.setP4(reco::Candidate::LorentzVector(mom2.x(), mom2.y(), mom2.z(), candE2));
      VertexCompositeCandidate* cand = new VertexCompositeCandidate(0, candLVec, vtx, vtxCov, vtxChi2, vtxNdof);
      cand->addDaughter(newLep1);
      cand->addDaughter(newLep2);

      cand->setPdgId(pdgId_);
      AddFourMomenta addP4;
      addP4.set(*cand);

      decayCands->push_back(*cand);
      decayLengths->push_back(rVtxMag);
      decayLengths3D->push_back(rVtxMag3D);

    }
  }

  const unsigned int nCands = decayCands->size();
  event.put(decayCands);
  event.put(decayLengths, "lxy");
  event.put(decayLengths3D, "l3D");

  return (nCands >= minNumber_ and nCands <= maxNumber_);
}

reco::TransientTrack KJpsiToMuMuProducer::GetTransientTrack( edm::ESHandle<TransientTrackBuilder> theB, pat::Muon muon, bool& trig) {
  reco::TrackRef track;
  if ( muon.isGlobalMuon() ) track = muon.globalTrack(); 
  else if ( muon.isTrackerMuon() ) track = muon.innerTrack();
  else { 
      trig = true;
      return reco::TransientTrack(); 
  }
  return theB->build( track) ; 

}
 
typedef KJpsiToMuMuProducer KJpsiMuMuProducer;
#include "FWCore/Framework/interface/MakerMacros.h"
DEFINE_FWK_MODULE(KJpsiMuMuProducer);

