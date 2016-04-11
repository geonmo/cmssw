import FWCore.ParameterSet.Config as cms

from Validation.MuonGEMHits.MuonGEMHits_cfi import *
from Validation.MuonGEMDigis.MuonGEMDigis_cfi import *
from Validation.MuonGEMRecHits.MuonGEMRecHits_cfi import *

from Validation.MuonME0Validation.MuonME0Hits_cfi import *
from Validation.MuonME0Validation.MuonME0Digis_cfi import *
from Validation.MuonME0Validation.MuonME0RecHits_cfi import *

gemSimValid = cms.Sequence(gemSimHitValidation*gemDigiValidation*gemRecHitsValidation)
me0SimValid = cms.Sequence(me0HitsValidation*me0DigiValidation*me0RecHitsValidation)
