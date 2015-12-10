#!/usr/bin/env python
### Made by Geonmo Ryu
### If you want this scripts, you need to 
### 1) pull request geonmo-cmssw/gem-sim-validation 
### 2) git cms-addpkg Validation/ and git cms-addpkg Geometry 

import os,sys
from optparse import OptionParser

### Pre-defined patchs.

fixMissingContents = '''
# To save all contents
process.FEVTDEBUGHLToutput.outputCommands.append('keep *_generatorSmeared_*_*')
process.FEVTDEBUGHLToutput.outputCommands.append('keep *_ak4GenJets_*_*')
process.FEVTDEBUGHLToutput.outputCommands.append('keep *_simCscTriggerPrimitiveDigis_*_*')
'''
noBkgNoise = '''
# Manual customization to switch off background hits. Suggested by Piet.
process.simMuonGEMDigis.digitizeOnlyMuons      = cms.bool(True)  # default: false
process.simMuonGEMDigis.doBkgNoise             = cms.bool(False) # default: true   
process.simMuonGEMDigis.doNoiseCLS             = cms.bool(False) # default: true
'''

drawDetailPlot = '''
process.gemSimHitValidation.detailPlot = cms.bool(True)
process.gemSimTrackValidation.detailPlot = cms.bool(True)
process.gemStripValidation.detailPlot = cms.bool(True)
process.gemPadValidation.detailPlot = cms.bool(True)
process.gemCoPadValidation.detailPlot = cms.bool(True)
process.gemDigiTrackValidation.detailPlot = cms.bool(True)
process.gemGeometryChecker.detailPlot = cms.bool(True)
process.gemRecHitsValidation.detailPlot = cms.bool(True)
process.gemRecHitTrackValidation.detailPlot = cms.bool(True)
'''

### setup Option parser.

parser = OptionParser()
parser.add_option("-c","--condition",   action="store",   dest="condition",   default="auto:run2_design", help="Setup GlobalTag. Default is [auto:run2_design]", metavar="GLOBALTAG" )
parser.add_option("-m","--magField",    action="store",   dest="magField",    default="38T_PostLS1", help="Magentic Field. Default is [38T_PostLS1]")
parser.add_option("-g","--geometry",    action="store",   dest="geometry",    default="Extended2015MuonGEMDev,Extended2015MuonGEMDevReco", help="Set the geometry(include GEM). Default : [Extended2015MuonGEMDev,Extended2015MuonGEMDevReco]")
parser.add_option("-u","--customise",   action="store",   dest="customise",   default="SLHCUpgradeSimulations/Configuration/gemCustom.customise2023,Geometry/GEMGeometry/gemGeometryCustoms.custom_%s")
#parser.add_option("-1","--10",          action="store_",   dest="version",     default="GE21_v7")
parser.add_option("-n","--maxEvents",   action="store",   dest="maxEvents",   default="100", help="Number of Events. Default is [100]")
parser.add_option("--era",              action="store",   dest="era",         default="Run2_25ns", help="--era options for cmsDrvier. Default is [Run2_25ns]")
parser.add_option("-t","--detailPlot",  action="store_true",   dest="detailPlot",   default=False, help="Turn on to draw detailPlot")
parser.add_option("-b","--noBkgNoise",  action="store_true",   dest="noBkgNoise",   default=False, help="Turn on to remove BkgNoise")

(options, args) = parser.parse_args()

### Run cmsDriver.

geos = ["GE21_v7", "GE21_v7_10deg"]
for geo in geos :
  customise = options.customise%(geo)
  digiCustom = "SimMuon/GEMDigitizer/customizeGEMDigi.customize_digi_addGEM_muon_only,SLHCUpgradeSimulations/Configuration/fixMissingUpgradeGTPayloads.fixRPCConditions,"+customise+",SLHCUpgradeSimulations/Configuration/me0Customs.customise_Digi"
  pythonFile = ["SingleMuPt100_cfi_GEM-SIM-DIGI_Extended2015_%s_cfg.py"%(geo), 
                "valid_%s_cfg.py"%(geo), 
                "harvest_%s_cfg.py"%(geo)]
  common_options = "--conditions %s --magField %s --geometry %s --eventcontent FEVTDEBUGHLT  -n %s --no_exec --era %s"%(options.condition, options.magField, options.geometry, options.maxEvents, options.era)
  #### cmsDriver command
  digi    = "cmsDriver.py SingleMuPt100_cfi -s GEN,SIM,DIGI,L1 %s --customise %s --datatier GEN-SIM-DIGI --fileout out_digi.root --python_filename %s"%(common_options, digiCustom, pythonFile[0])
  valid   = "cmsDriver.py valid -s VALIDATION:genvalid_all %s --customise %s --datatier GEN-SIM-RECO --filein out_local_reco.root --fileout out_valid.root --python_filename %s"%(common_options, customise, pythonFile[1])
  harvest = "cmsDriver.py harvest -s HARVESTING:genHarvesting %s --customise %s --datatier GEN-SIM-RECO --filein out_valid.root --fileout out_valid.root --python_filename %s"%(common_options, customise, pythonFile[2])
  print digi
  os.system(digi)
  print valid
  os.system(valid)
  print harvest
  os.system(harvest)
  print

  print "Now, apply the patches as option"
  print "Add missing collection"
  with open(pythonFile[0],'a') as file:
    file.write(fixMissingContents)
  if ( options.noBkgNoise) :
    print "Apply no Backgroud noise for GEM efficiency plots."
    with open(pythonFile[0],'a') as file:
      file.write(noBkgNoise)

  if ( options.detailPlot ) : 
    print "detailPlots option is enable. Add detailPlot options."
    with open(pythonFile[1],'a') as file:
      file.write(drawDetailPlot)

