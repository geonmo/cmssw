#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/Framework/interface/EDProducer.h"

#include "FWCore/Framework/interface/Event.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "DataFormats/Candidate/interface/Candidate.h"
#include "DataFormats/PatCandidates/interface/Jet.h"
#include "DataFormats/PatCandidates/interface/MET.h"

#include "DataFormats/Common/interface/View.h"

#include "CondFormats/JetMETObjects/interface/FactorizedJetCorrector.h"
#include "CondFormats/JetMETObjects/interface/JetCorrectionUncertainty.h"

#include "DataFormats/Common/interface/AssociationMap.h"

#include <memory>

using namespace edm;
using namespace std;

class KJetUncProducer : public edm::EDProducer
{
public:
  KJetUncProducer(const edm::ParameterSet& pset);
  ~KJetUncProducer() {};
  typedef std::vector<pat::Jet> Jets;
  typedef std::vector<pat::MET> METs;
  typedef edm::AssociationMap<edm::OneToValue<Jets, double> > JetToDouble;

private:
  void beginJob() {};
  void produce(edm::Event& event, const edm::EventSetup& eventSetup);
  void endJob() {};

  edm::InputTag jetLabel_, metLabel_;

  JetCorrectionUncertainty *jecUncCalculator_;
  double minPt_;

  bool isMC_;

private:
  void getJERFactor(const double jetEta, double& cJER, double& cJERUp, double& cJERDn)
  {
    if      ( jetEta < 0.5 ) { cJER = 1.052; cJERUp = 1.115; cJERDn = 0.990; }
    else if ( jetEta < 1.1 ) { cJER = 1.057; cJERUp = 1.114; cJERDn = 1.001; }
    else if ( jetEta < 1.7 ) { cJER = 1.096; cJERUp = 1.161; cJERDn = 1.032; }
    else if ( jetEta < 2.3 ) { cJER = 1.134; cJERUp = 1.228; cJERDn = 1.042; }
    else if ( jetEta < 5.0 ) { cJER = 1.288; cJERUp = 1.488; cJERDn = 1.089; }
    else { cJER = cJERUp = cJERDn = 1; }
  }

};

KJetUncProducer::KJetUncProducer(const edm::ParameterSet& pset)
{
  isMC_ = pset.getParameter<bool>("isMC");
  minPt_ = pset.getParameter<double>("minPt");

  jetLabel_ = pset.getParameter<edm::InputTag>("jet");
  metLabel_ = pset.getParameter<edm::InputTag>("met");

  // JEC correction
  edm::FileInPath jecFilePathRD(pset.getParameter<string>("jecFileRD"));
  edm::FileInPath jecFilePathMC(pset.getParameter<string>("jecFileMC"));
  if ( isMC_ ) jecUncCalculator_ = new JetCorrectionUncertainty(jecFilePathMC.fullPath());
  else jecUncCalculator_ = new JetCorrectionUncertainty(jecFilePathRD.fullPath());

  produces<JetToDouble>("up");
  produces<JetToDouble>("dn");
  produces<METs>("up");
  produces<METs>("dn");
  if ( isMC_ )
  {
    produces<JetToDouble>("res");
    produces<JetToDouble>("resUp");
    produces<JetToDouble>("resDn");
    produces<METs>("res");
    produces<METs>("resUp");
    produces<METs>("resDn");
  }
}

void KJetUncProducer::produce(edm::Event& event, const edm::EventSetup& eventSetup)
{
  edm::Handle<Jets> jetHandle;
  event.getByLabel(jetLabel_, jetHandle);

  edm::Handle<METs> metHandle;
  event.getByLabel(metLabel_, metHandle);

  std::auto_ptr<JetToDouble> fJECsUp(new JetToDouble);
  std::auto_ptr<JetToDouble> fJECsDn(new JetToDouble);
  std::auto_ptr<JetToDouble> fJERs(new JetToDouble);
  std::auto_ptr<JetToDouble> fJERsUp(new JetToDouble);
  std::auto_ptr<JetToDouble> fJERsDn(new JetToDouble);

  std::auto_ptr<METs> metsUp(new METs);
  std::auto_ptr<METs> metsDn(new METs);
  std::auto_ptr<METs> metsRes(new METs);
  std::auto_ptr<METs> metsResUp(new METs);
  std::auto_ptr<METs> metsResDn(new METs);

  pat::MET met = metHandle->at(0);
  const double metX = met.px(), metY = met.py();
  double metUpX = metX, metUpY = metY;
  double metDnX = metX, metDnY = metY;
  double metResX   = metX, metResY   = metY;
  double metResUpX = metX, metResUpY = metY;
  double metResDnX = metX, metResDnY = metY;

  for ( int i=0, n=jetHandle->size(); i<n; ++i )
  {
    pat::Jet jet = jetHandle->at(i);
    reco::Candidate::LorentzVector jetP4 = jet.p4();
    const double jetPt = jetP4.pt();

    // JEC and uncertainties
    jecUncCalculator_->setJetPt(jetP4.pt());
    jecUncCalculator_->setJetEta(jetP4.eta());
    const double fJECUp = 1+jecUncCalculator_->getUncertainty(true);
    jecUncCalculator_->setJetPt(jetP4.pt());
    jecUncCalculator_->setJetEta(jetP4.eta());
    const double fJECDn = 1-jecUncCalculator_->getUncertainty(false);

    math::XYZTLorentzVector jetUpP4 = jetP4*fJECUp;
    math::XYZTLorentzVector jetDnP4 = jetP4*fJECDn;

    metUpX += jetP4.px() - jetUpP4.px();
    metUpY += jetP4.py() - jetUpP4.py();
    metDnX += jetP4.px() - jetDnP4.px();
    metDnY += jetP4.py() - jetDnP4.py();

    // JER and uncertainties
    math::XYZTLorentzVector jetResUpP4, jetResDnP4;
    double fJER = 1, fJERUp = 1, fJERDn = 1;
    if ( isMC_ )
    {
      const reco::GenJet* genJet = jet.genJet();
      if ( genJet and genJet->pt() > 10 )
      {
        const math::XYZTLorentzVector& rawJetP4 = jet.correctedP4(0);
        const double rawPx = rawJetP4.px();
        const double rawPy = rawJetP4.py();

        const double genJetPt = genJet->pt();
        const double dPt = jetPt-genJetPt;

        const double jetEta = std::abs(jet.eta());
        double cJER, cJERUp, cJERDn;
        getJERFactor(jetEta, cJER, cJERUp, cJERDn);

        fJER   = max(1e-9, (genJetPt+dPt*cJER  )/jetPt);
        fJERUp = max(1e-9, (genJetPt+dPt*cJERUp)/jetPt);
        fJERDn = max(1e-9, (genJetPt+dPt*cJERDn)/jetPt);

        const double metDx   = rawPx*(1-fJER  );
        const double metDxUp = rawPx*(1-fJERUp);
        const double metDxDn = rawPx*(1-fJERDn);

        const double metDy   = rawPy*(1-fJER  );
        const double metDyUp = rawPy*(1-fJERUp);
        const double metDyDn = rawPy*(1-fJERDn);

        // Correct MET
        metDnX    += metDx  ; metDnY    += metDy  ;
        metUpX    += metDx  ; metUpY    += metDy  ;
        metResX   += metDx  ; metResY   += metDy  ;
        metResUpX += metDxUp; metResUpY += metDyUp;
        metResDnX += metDxDn; metResDnY += metDyDn;
      }
    }

    // Check acceptance after JEC and JER
    if ( jetPt*fJER < minPt_ and 
         jetUpP4.pt()*fJER < minPt_ and jetDnP4.pt()*fJER < minPt_ and
         jetPt*fJERUp < minPt_ and jetPt*fJERDn < minPt_ ) continue;

    // Put JES,JER factors
    edm::Ref<Jets> jetRef(jetHandle, i);
    fJECsUp->insert(jetRef, fJECUp);
    fJECsDn->insert(jetRef, fJECDn);
    if ( isMC_ )
    {
      fJERs->insert(jetRef, fJER);
      fJERsUp->insert(jetRef, fJERUp);
      fJERsDn->insert(jetRef, fJERDn);
    }
  }

  pat::MET metUp, metDn;
  metUp.setP4(reco::Candidate::LorentzVector(metUpX, metUpY, 0, hypot(metUpX, metUpY)));
  metDn.setP4(reco::Candidate::LorentzVector(metDnX, metDnY, 0, hypot(metDnX, metDnY)));
  metsUp->push_back(metUp);
  metsDn->push_back(metDn);

  event.put(fJECsUp, "up");
  event.put(fJECsDn, "dn");
  event.put(metsUp, "up");
  event.put(metsDn, "dn");

  if ( isMC_ )
  {
    pat::MET metRes, metResUp, metResDn;
    metRes.setP4(reco::Candidate::LorentzVector(metResX, metResY, 0, hypot(metResX, metResY)));
    metResUp.setP4(reco::Candidate::LorentzVector(metResUpX, metResUpY, 0, hypot(metResUpX, metResUpY)));
    metResDn.setP4(reco::Candidate::LorentzVector(metResDnX, metResDnY, 0, hypot(metResDnX, metResDnY)));
    metsRes->push_back(metRes);
    metsResUp->push_back(metResUp);
    metsResDn->push_back(metResDn);

    event.put(fJERs, "res");
    event.put(fJERsUp, "resUp");
    event.put(fJERsDn, "resDn");
    event.put(metsRes);
    event.put(metsResUp, "resUp");
    event.put(metsResDn, "resDn");
  }

}

DEFINE_FWK_MODULE(KJetUncProducer);

