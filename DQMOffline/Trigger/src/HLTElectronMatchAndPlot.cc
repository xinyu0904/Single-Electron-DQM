#ifndef DQMOffline_Trigger_HLTElectronMatchAndPlot_CC
#define DQMOffline_Trigger_HLTElectronMatchAndPlot_CC
/** \file DQMOffline/Trigger/HLTElectronMatchAndPlot.cc
 *
 */


#include "DQMOffline/Trigger/interface/HLTElectronMatchAndPlot.h"

#include "FWCore/Framework/interface/Run.h"
#include "FWCore/Framework/interface/EventSetup.h"
#include "SimDataFormats/GeneratorProducts/interface/GenEventInfoProduct.h"
#include "SimDataFormats/GeneratorProducts/interface/GenRunInfoProduct.h"
#include "FWCore/ServiceRegistry/interface/Service.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"
#include "DataFormats/Candidate/interface/CandMatchMap.h"

#include <iostream>

//////////////////////////////////////////////////////////////////////////////
//////// Namespaces and Typedefs /////////////////////////////////////////////

using namespace std;
using namespace edm;
using namespace reco;
using namespace trigger;
using namespace l1extra;
//using namespace ConversionTools;

#define debug 1
typedef std::vector<std::string> vstring;


//////////////////////////////////////////////////////////////////////////////
//////// HLTElectronMatchAndPlot Class Members ///////////////////////////////////

/// Constructor
HLTElectronMatchAndPlot::HLTElectronMatchAndPlot(const ParameterSet & pset, 
                                         string hltPath, 
                                         string moduleLabel, bool islastfilter) :
  hltProcessName_(pset.getParameter<string>("hltProcessName")),
  folderName_(pset.getParameter<string>("FolderName")),
  requiredTriggers_(pset.getUntrackedParameter<vstring>("requiredTriggers")),
  targetParams_(pset.getParameterSet("targetParams")),
  probeParams_(pset.getParameterSet("probeParams")),
  hltPath_(hltPath),
  moduleLabel_(moduleLabel),
  isLastFilter_(islastfilter),
  hasTargetRecoCuts(targetParams_.exists("recoCuts")),
  hasProbeRecoCuts(probeParams_.exists("recoCuts")),
  targetElectronSelector_(targetParams_.getUntrackedParameter<string>("recoCuts", "")),
//  targetZ0Cut_(targetParams_.getUntrackedParameter<double>("z0Cut",0.)),
//  targetD0Cut_(targetParams_.getUntrackedParameter<double>("d0Cut",0.)),
  targetptCutZ_(targetParams_.getUntrackedParameter<double>("ptCut_Z",20.)), 
  probeElectronSelector_(probeParams_.getUntrackedParameter<string>("recoCuts", "")),
//  probeZ0Cut_(probeParams_.getUntrackedParameter<double>("z0Cut",0.)),
//  probeD0Cut_(probeParams_.getUntrackedParameter<double>("d0Cut",0.)),
  triggerSelector_(targetParams_.getUntrackedParameter<string>("hltCuts","")),
  hasTriggerCuts_(targetParams_.exists("hltCuts")),
  effectiveAreas_((pset.getParameter<edm::FileInPath>("effAreasConfigFile")).fullPath())
{
  // Create std::map<string, T> from ParameterSets. 
  fillMapFromPSet(binParams_, pset, "binParams");
  fillMapFromPSet(plotCuts_, pset, "plotCuts");

  // Get the trigger level.
/*  triggerLevel_ = "L3";
  TPRegexp levelRegexp("L[1-3]");
  //  size_t nModules = moduleLabels_.size();
  TObjArray * levelArray = levelRegexp.MatchS(moduleLabel_);
  if (levelArray->GetEntriesFast() > 0) {
    triggerLevel_ = ((TObjString *)levelArray->At(0))->GetString();
  }
  delete levelArray;
*/

  // Get the pT cut by pasing the name of the HLT path.
  cutMinPt_ = 3;
  TPRegexp ptRegexp("Ele([0-9]*)");
  TObjArray * objArray = ptRegexp.MatchS(hltPath_);
  if (objArray->GetEntriesFast() >= 2) {
    TObjString * ptCutString = (TObjString *)objArray->At(1);
    cutMinPt_ = atoi(ptCutString->GetString()); //cutMinPt_ = 32;
    cutMinPt_ = ceil(cutMinPt_ * plotCuts_["minPtFactor"]);
  }
  delete objArray;

}

void HLTElectronMatchAndPlot::beginRun(DQMStore::IBooker & iBooker,
				   const edm::Run& iRun, 
                                   const edm::EventSetup& iSetup)
{

  // set folder name of path in final root file(tree); 
  TPRegexp suffixPtCut("Ele[0-9]+$");
  string baseDir = folderName_;
  if (baseDir[baseDir.size() - 1] != '/') baseDir += '/';
  string pathSansSuffix = hltPath_;
  if (hltPath_.rfind("_v") < hltPath_.length())
    pathSansSuffix = hltPath_.substr(0, hltPath_.rfind("_v"));
  
  if (isLastFilter_) 
    iBooker.setCurrentFolder(baseDir + pathSansSuffix);
  else 
    iBooker.setCurrentFolder(baseDir + pathSansSuffix + "/" + moduleLabel_);
  
  // Form is book1D(name, binningType, title) where 'binningType' is used 
  // to fetch the bin settings from binParams_.
  if (isLastFilter_){
    book1D(iBooker, "hltPt", "pt", ";p_{T} of HLT object");
    book1D(iBooker, "hltEta", "eta", ";#eta of HLT object");
    book1D(iBooker, "hltPhi", "phi", ";#phi of HLT object");
    book1D(iBooker, "resolutionEta", "resolutionEta", ";#eta^{reco}-#eta^{HLT};");
    book1D(iBooker, "resolutionPhi", "resolutionPhi", ";#phi^{reco}-#phi^{HLT};");
  }
  book1D(iBooker, "deltaR", "deltaR", ";#Deltar(reco, HLT);");
  
  book1D(iBooker, "resolutionPt", "resolutionRel", 
         ";(p_{T}^{reco}-p_{T}^{HLT})/|p_{T}^{reco}|;");

  for (size_t i = 0; i < 2; i++) {

    string suffix = EFFICIENCY_SUFFIXES[i];

    book1D(iBooker, "efficiencyEta_" + suffix, "eta", ";#eta;");
    book1D(iBooker, "efficiencyPhi_" + suffix, "phi", ";#phi;");
    book1D(iBooker, "efficiencyTurnOn_" + suffix, "pt", ";p_{T};");
    book1D(iBooker, "efficiencyVertex_" + suffix, "NVertex", ";NVertex;");
   

    book2D(iBooker, "efficiencyPhiVsEta_" + suffix, "etaCoarse", 
	   "phiCoarse", ";#eta;#phi");

    if (!isLastFilter_) continue;  //this will be plotted only for the last filter
//  book1D(iBooker, string name, string binningType, string title);     
//    book1D(iBooker, "efficiencyD0_" + suffix, "d0", ";d0;");
//    book1D(iBooker, "efficiencyZ0_" + suffix, "z0", ";z0;");
    book1D(iBooker, "efficiencyCharge_" + suffix, "charge", ";charge;");
    
    book1D(iBooker, "fakerateEta_" + suffix, "eta", ";#eta;");
    book1D(iBooker, "fakerateVertex_" + suffix, "NVertex", ";NVertex;");
    book1D(iBooker, "fakeratePhi_" + suffix, "phi", ";#phi;");
    book1D(iBooker, "fakerateTurnOn_" + suffix, "pt", ";p_{T};");
    book2D(iBooker, "fakeratePhiVsEta_" + suffix, "eta", 
	   "phi", ";#eta;#phi");
    
    book1D(iBooker, "massVsmassZ_EB_" + suffix, "zMass", ";mass");
    book1D(iBooker, "massVsEtaZ_EB_" + suffix, "etaCoarse", ";#eta");
    book1D(iBooker, "massVsPtZ_EB_" + suffix, "ptCoarse", ";p_{T}");
    book1D(iBooker, "massVsVertexZ_EB_" + suffix, "NVertex", ";NVertex");
    book2D(iBooker, "massVsPhiVsEtaZ_EB_" + suffix, "etaCoarse", 
	   "phiCoarse", ";#eta;#phi");
    book1D(iBooker, "massVsSigmaIetaIetaZ_EB_" + suffix, "sigmaIetaIeta", ";#sigmaI#etaI#eta");
    book1D(iBooker, "massVsHoEZ_EB_" + suffix, "HOE", ";HoE");
    book1D(iBooker, "massVsisoPFCorrRelZ_EB_" + suffix, "isoPFCorrRel", ";isoRel");

    book1D(iBooker, "massVsmassZ_EE_" + suffix, "zMass", ";mass");
    book1D(iBooker, "massVsEtaZ_EE_" + suffix, "etaCoarse", ";#eta");
    book1D(iBooker, "massVsPtZ_EE_" + suffix, "ptCoarse", ";p_{T}");
    book1D(iBooker, "massVsVertexZ_EE_" + suffix, "NVertex", ";NVertex");
    book2D(iBooker, "massVsPhiVsEtaZ_EE_" + suffix, "etaCoarse", 
	   "phiCoarse", ";#eta;#phi");
    book1D(iBooker, "massVsSigmaIetaIetaZ_EE_" + suffix, "sigmaIetaIeta", ";#sigmaI#etaI#eta");
    book1D(iBooker, "massVsHoEZ_EE_" + suffix, "HOE", ";HoE");
    book1D(iBooker, "massVsisoPFCorrRelZ_EE_" + suffix, "isoPFCorrRel", ";isoRel");
    }
  
}



void HLTElectronMatchAndPlot::endRun(const edm::Run& iRun, 
                                 const edm::EventSetup& iSetup)
{

}



void HLTElectronMatchAndPlot::analyze(Handle<GsfElectronCollection>   & eleHandle,
				  Handle<double>               & rho,
				  Handle<ConversionCollection> & convs,
				  Handle<BeamSpot>         & beamSpot,
				  Handle<VertexCollection> & vertices,
				  Handle<TriggerEvent>     & triggerSummary,  
				  Handle<TriggerResults>   & triggerResults)
{
  /*
  if(gen != 0) {
    for(g_part = gen->begin(); g_part != gen->end(); g_part++){
      if( abs(g_part->pdgId()) ==  13) {
        if(!( g_part->status() ==1 || (g_part->status() ==2 && abs(g_part->pdgId())==5))) continue;
        bool GenMomExists  (true);
        bool GenGrMomExists(true);
        if( g_part->pt() < 10.0 )  continue;
        if( fabs(g_part->eta()) > 2.4 ) continue;
        int gen_id= g_part->pdgId();
        const GenParticle* gen_lept = &(*g_part);
        // get mother of gen_lept
        const GenParticle* gen_mom = static_cast<const GenParticle*> (gen_lept->mother());
        if(gen_mom==NULL) GenMomExists=false;
        int m_id=-999;
        if(GenMomExists) m_id = gen_mom->pdgId();
        if(m_id != gen_id || !GenMomExists);
        else{
          int id= m_id;
          while(id == gen_id && GenMomExists){
            gen_mom = static_cast<const GenParticle*> (gen_mom->mother());
            if(gen_mom==NULL){
              GenMomExists=false;
            }
            if(GenMomExists) id=gen_mom->pdgId();
          }
        }
        if(GenMomExists) m_id = gen_mom->pdgId();
        gen_leptsp.push_back(gen_lept);
        gen_momsp.push_back(gen_mom);
      }
    }
  }


  std::vector<GenParticle> gen_lepts;
  for(size_t i = 0; i < gen_leptsp.size(); i++) {
    gen_lepts.push_back(*gen_leptsp[i]);
  }


  */



  // Throw out this event if it doesn't pass the required triggers.
  // this is not needed anymore rejecting if there is no filter... 
///  for (size_t i = 0; i < requiredTriggers_.size(); i++) {
///    unsigned int triggerIndex = triggerResults->find(requiredTriggers_[i]);
///    if (triggerIndex < triggerResults->size() ||
///        !triggerResults->accept(triggerIndex))
///      return;
///  }
  
  
  // Select objects based on the configuration.
//  ElectronCollection targetElectrons = selectedElectrons(* eleHandle, * beamSpot, hasTargetRecoCuts, targetMuonSelector_, targetD0Cut_, targetZ0Cut_);
//  MuonCollection probeMuons = selectedMuons(* allMuons, * beamSpot, hasProbeRecoCuts, probeMuonSelector_, probeD0Cut_, probeZ0Cut_);
//  TriggerObjectCollection allTriggerObjects = triggerSummary->getObjects();
//  TriggerObjectCollection hltMuons = 
//    selectedTriggerObjects(allTriggerObjects, * triggerSummary, hasTriggerCuts_,triggerSelector_);


  GsfElectronCollection targetElectrons = selectedElectrons(* eleHandle, * beamSpot, hasTargetRecoCuts, targetElectronSelector_);
  GsfElectronCollection probeElectrons = selectedElectrons(* eleHandle, * beamSpot, hasProbeRecoCuts, probeElectronSelector_);
  TriggerObjectCollection allTriggerObjects = triggerSummary->getObjects();
  TriggerObjectCollection hltElectrons = 
    selectedTriggerObjects(allTriggerObjects, * triggerSummary, hasTriggerCuts_,triggerSelector_);
    cout<<"the size of targetElectrons = "<< targetElectrons.size() <<endl;
    cout<<"the size of hltElectrons    = "<< hltElectrons.size() <<endl;

  // Fill plots for HLT muons.
  if (isLastFilter_){
    for (size_t i = 0; i < hltElectrons.size(); i++) {
      hists_["hltPt"]->Fill(hltElectrons[i].pt());
      hists_["hltEta"]->Fill(hltElectrons[i].eta());
      hists_["hltPhi"]->Fill(hltElectrons[i].phi());
    }
  }
  // Find the best trigger object matches for the targetElectrons.
  vector<size_t> matches = matchByDeltaR(targetElectrons, hltElectrons, 
//                                         plotCuts_[triggerLevel_ + "DeltaR"]);
                                         plotCuts_["DeltaR"]);
cout<<"---------  Sijing : 111111  ---------"<<endl;
  // Fill plots for matched electrons.(Tag Electron)
  int N_tag = 0;
  int N_probe = 0;
  int N_passingprobe = 0;
  bool pairalreadyconsidered = false;
  for (size_t i = 0; i < targetElectrons.size(); i++) {

    GsfElectron & electron = targetElectrons[i];

    // WP Tight(isTightElectron selection)
    float  eta = electron.superCluster()->eta();
    float  sigmaIetaIeta = float(electron.full5x5_sigmaIetaIeta() );
    float  dEtaSeedClusterTrackAtVtx  = electron.deltaEtaSeedClusterTrackAtVtx();
    double dPhiSuperClusterTrackAtVtx = electron.deltaPhiSuperClusterTrackAtVtx();
    double HOverE = electron.hadronicOverEm();
    double EInverseMinusPInverse = abs((1.0 - electron.eSuperClusterOverP()) * (1.0/electron.ecalEnergy()));

    constexpr reco::HitPattern::HitCategory missingHitType = reco::HitPattern::MISSING_INNER_HITS;
    const unsigned mHits = electron.gsfTrack()->hitPattern().numberOfHits(missingHitType);

    bool isPassConversionVeto = ! ConversionTools::hasMatchedConversion(electron, convs, beamSpot->position() );

    // defination for relative PF. iso with EA correction
    const reco::GsfElectron::PflowIsolationVariables& pfIso = electron.pfIsolationVariables();
    const float chad = pfIso.sumChargedHadronPt;
    const float nhad = pfIso.sumNeutralHadronEt;
    const float pho = pfIso.sumPhotonEt;
    const float eA = effectiveAreas_.getEffectiveArea( abs(eta) );
    const float Rho =  (float)(*rho);
//    const double Pt = electron.pt();
//    const double isoPFValueCorrRel = chad + std::max(0.0f, nhad + pho - Rho*eA);
cout<<"HOverE = "<< HOverE <<endl;
    const double isoPFValueCorrRel = (chad + std::max(0.0f, nhad + pho - Rho*eA)) / electron.pt();

    // isTightElectron selection(for both Tag and Probe)
    if(!(( (abs(eta) <= 1.479) && sigmaIetaIeta < 0.00998 && abs(dEtaSeedClusterTrackAtVtx) < 0.00308 && abs(dPhiSuperClusterTrackAtVtx) < 0.0816 && HOverE < 0.0414 && EInverseMinusPInverse < 0.0129 && mHits <= 1 && isPassConversionVeto && isoPFValueCorrRel < 0.0588)
      || ( (abs(eta) >= 1.479) && sigmaIetaIeta < 0.0292 && (abs(dEtaSeedClusterTrackAtVtx) < 0.00605) && abs(dPhiSuperClusterTrackAtVtx) < 0.0394 && HOverE < 0.0641 && EInverseMinusPInverse < 0.0129 && mHits <= 1 && isPassConversionVeto && isoPFValueCorrRel < 0.0571))) continue;

// WP Tight selection criteria for only Tag electrons;
//     if(( (abs(eta) <= 1.479) && full5x5_sigmaIetaIeta < 0.00998 && abs(dEtaSeedClusterTrackAtVtx) < 0.00308 && abs(dPhiSuperClusterTrackAtVtx) < 0.0816 && HOverE < 0.0414 && EInverseMinusPInverse < 0.0129 && mHits <= 1 && isPassConversionVeto && isoPFValueCorrRel < 0.0588)  || ( (abs(eta) >= 1.479) && full5x5_sigmaIetaIeta < 0.0292 && (abs(dEtaSeedClusterTrackAtVtx) < 0.00605) && abs(dPhiSuperClusterTrackAtVtx) < 0.0394 && HOverE < 0.0641 && EInverseMinusPInverse < 0.0129 && mHits <= 1 && isPassConversionVeto && isoPFValueCorrRel < 0.0571) )
//    if(!isTightElectron) continue;
//    float eta2 = electron.eta();
//    cout<<"etaSuperClster = "<< eta1 <<endl;
//    cout<<"etaElectron = "<< eta2 <<endl <<"---------"<<endl;

    
    // Fill plots which are not efficiencies.
    if (matches[i] < targetElectrons.size()) {
    // Tag selections
      if( (abs(eta) <= 1.442 || (abs(eta) <=1.566 && abs(eta) <= 2.1)) && electron.pt() > 30){ 
       TriggerObject & hltElectron = hltElectrons[matches[i]];
       double ptRes = (electron.pt() - hltElectron.pt()) / electron.pt();
       hists_["resolutionPt"]->Fill(ptRes);
       hists_["deltaR"]->Fill(deltaR(electron, hltElectron));
      
       if (isLastFilter_){
	double etaRes = electron.eta() - hltElectron.eta();
	double phiRes = electron.phi() - hltElectron.phi();
	hists_["resolutionEta"]->Fill(etaRes);
	hists_["resolutionPhi"]->Fill(phiRes);
	N_tag++;
       }
      } 
    }
cout<<"---------  Sijing : 333333 ---------"<<endl; 
    // Fill numerators and denominator for efficiency plots.
    for (size_t j = 0; j < 2; j++) {

      string suffix = EFFICIENCY_SUFFIXES[j];
      
      // all the Probes have to be not in the gap;
      if ( abs(eta) > 1.442 && abs(eta) < 1.566) continue;
      if ( electron.pt() <20) continue;

      // If no match was found, then the numerator plots don't get filled.
      // numerator: passing probe, denominator: the probe;
      if (suffix == "numer" && matches[i] >= targetElectrons.size()) continue;
      if (electron.pt() > cutMinPt_) {
        hists_["efficiencyEta_" + suffix]->Fill(electron.eta());
        hists_["efficiencyPhiVsEta_" + suffix]->Fill(electron.eta(), electron.phi());
      }
      
      if (fabs(electron.eta()) < plotCuts_["maxEta"]) {
        hists_["efficiencyTurnOn_" + suffix]->Fill(electron.pt());
      }
      
      if (electron.pt() > cutMinPt_ && fabs(electron.eta()) < plotCuts_["maxEta"]) {
//        const Track * track = 0;
//        track = & * electron.gsfTrack();
//	if (track) 
          hists_["efficiencyVertex_" + suffix]->Fill(vertices->size());
          hists_["efficiencyPhi_" + suffix]->Fill(electron.phi());

	  if (isLastFilter_){
//	    double d0 = track->dxy(beamSpot->position());
//	    double z0 = track->dz(beamSpot->position());
//	    hists_["efficiencyD0_" + suffix]->Fill(d0);
//	    hists_["efficiencyZ0_" + suffix]->Fill(z0);
	    hists_["efficiencyCharge_" + suffix]->Fill(electron.charge());
	  }
//	
      }
    } // finish loop numerator / denominator...
    
    if (!isLastFilter_) continue;
    // Fill plots for tag and probe
    // Electron cannot be a tag because doesn't match an hlt electron     
    if(matches[i] >= targetElectrons.size()) continue;
    for (size_t k = 0; k < targetElectrons.size(); k++) {
      if(k == i) continue;
      GsfElectron & theProbe = targetElectrons[k];
      double mass = (electron.p4() + theProbe.p4()).M();
      float eta_P = theProbe.superCluster()->eta();
      const reco::GsfElectron::PflowIsolationVariables& pfIso_P = theProbe.pfIsolationVariables();
      const float chad_P = pfIso_P.sumChargedHadronPt;
      const float nhad_P = pfIso_P.sumNeutralHadronEt;
      const float pho_P = pfIso_P.sumPhotonEt;
      const float eA_P = effectiveAreas_.getEffectiveArea( abs(eta_P) );
//    const float Rho =  (float)(*rho);
      const double Pt_P = theProbe.pt()? theProbe.pt() : 1.;
//    const double isoPFValueCorrRel = chad + std::max(0.0f, nhad + pho - Rho*eA);
      if (electron.charge() != theProbe.charge() && !pairalreadyconsidered) {
	const double isoPFCorrRel = (chad_P + std::max(0.0f, nhad_P + pho_P - Rho*eA_P)) / Pt_P;
	if(mass > 60 && mass < 120) {
          if(electron.pt() < targetptCutZ_) continue; // pt>20
	if(eta_P < 1.442){
	  hists_["massVsmassZ_EB_denom"]->Fill(mass);
          hists_["massVsEtaZ_EB_denom"]->Fill(theProbe.eta());
          hists_["massVsPtZ_EB_denom"]->Fill(theProbe.pt());
          hists_["massVsVertexZ_EB_denom"]->Fill(vertices->size());
	  hists_["massVsPhiVsEtaZ_EB_denom"]->Fill(theProbe.eta(), theProbe.phi());
	  hists_["massVsSigmaIetaIetaZ_EB_denom"]->Fill(theProbe.full5x5_sigmaIetaIeta());
  	  hists_["massVsHoEZ_EB_denom"]->Fill(theProbe.hadronicOverEm());
          hists_["massVsisoPFCorrRelZ_EB_denom"]->Fill(isoPFCorrRel);
	}else if( eta_P > 1.566 ){
	  hists_["massVsmassZ_EE_denom"]->Fill(mass);
          hists_["massVsEtaZ_EE_denom"]->Fill(theProbe.eta());
          hists_["massVsPtZ_EE_denom"]->Fill(theProbe.pt());
          hists_["massVsVertexZ_EE_denom"]->Fill(vertices->size());
	  hists_["massVsPhiVsEtaZ_EE_denom"]->Fill(theProbe.eta(), theProbe.phi());
	  hists_["massVsSigmaIetaIetaZ_EE_denom"]->Fill(theProbe.full5x5_sigmaIetaIeta());
  	  hists_["massVsHoEZ_EE_denom"]->Fill(theProbe.hadronicOverEm());
          hists_["massVsisoPFCorrRelZ_EE_denom"]->Fill(isoPFCorrRel);
	
	}
	  N_probe++;

	  if(matches[k] < targetElectrons.size()) {
	    cout<<"mass 3 = "<< mass <<endl;
	  if(eta_P < 1.442){
	    hists_["massVsmassZ_EB_numer"]->Fill(mass);
            hists_["massVsEtaZ_EB_numer"]->Fill(theProbe.eta());
            hists_["massVsPtZ_EB_numer"]->Fill(theProbe.pt());
            hists_["massVsVertexZ_EB_numer"]->Fill(vertices->size());

	    hists_["massVsPhiVsEtaZ_EB_numer"]->Fill(theProbe.eta(), theProbe.phi());
	    hists_["massVsSigmaIetaIetaZ_EB_numer"]->Fill(theProbe.full5x5_sigmaIetaIeta());
            hists_["massVsHoEZ_EB_numer"]->Fill(theProbe.hadronicOverEm());
	    hists_["massVsisoPFCorrRelZ_EB_numer"]->Fill(isoPFCorrRel);
	  } else if(eta_P > 1.566){
	    hists_["massVsmassZ_EE_numer"]->Fill(mass);
            hists_["massVsEtaZ_EE_numer"]->Fill(theProbe.eta());
            hists_["massVsPtZ_EE_numer"]->Fill(theProbe.pt());
            hists_["massVsVertexZ_EE_numer"]->Fill(vertices->size());

	    hists_["massVsPhiVsEtaZ_EE_numer"]->Fill(theProbe.eta(), theProbe.phi());
	    hists_["massVsSigmaIetaIetaZ_EE_numer"]->Fill(theProbe.full5x5_sigmaIetaIeta());
            hists_["massVsHoEZ_EE_numer"]->Fill(theProbe.hadronicOverEm());
	    hists_["massVsisoPFCorrRelZ_EE_numer"]->Fill(isoPFCorrRel);
	  
	  }
	    //	    if( suffix == "numer") N_passingprobe++;
	    N_passingprobe++;
	  }  
          pairalreadyconsidered = true;
        }
      }
    } // End loop over denominator and numerator.
  } // End loop over targetElectrons.
  cout<<"N_tag = "<< N_tag <<endl; 
  cout<<"N_probe = "<< N_probe <<endl; 
  cout<<"N_passingprobe = "<< N_passingprobe <<endl; 
cout<<"---------  Sijing : cout END---------"<<endl;
  
  if (!isLastFilter_) return;
  // Plot fake rates (efficiency for HLT objects to not get matched to RECO).
  vector<size_t> hltMatches = matchByDeltaR(hltElectrons, targetElectrons,
//                                            plotCuts_[triggerLevel_ + "DeltaR"]);
                                            plotCuts_["DeltaR"]);
  for (size_t i = 0; i < hltElectrons.size(); i++) {
    TriggerObject & hltElectron = hltElectrons[i];
//    float eta_hlt = hltElectron.eta();
    bool isFake = hltMatches[i] > hltElectrons.size();
    for (size_t j = 0; j < 2; j++) {

      // defination: Line 6 file(line 55)
      string suffix = EFFICIENCY_SUFFIXES[j]; 
      // If match is found, then numerator plots should not get filled
      if (suffix == "numer" && ! isFake) continue;
      hists_["fakerateVertex_" + suffix]->Fill(vertices->size());
      hists_["fakerateEta_" + suffix]->Fill(hltElectron.eta());
      hists_["fakeratePhi_" + suffix]->Fill(hltElectron.phi());
      hists_["fakerateTurnOn_" + suffix]->Fill(hltElectron.pt());
      hists_["fakeratePhiVsEta_" + suffix]->Fill(hltElectron.eta(), hltElectron.phi());
    } // End loop over numerator and denominator.
  } // End loop over hltElectrons.
  

} // End analyze() method.



// Method to fill binning parameters from a vector of doubles.
void 
HLTElectronMatchAndPlot::fillEdges(size_t & nBins, float * & edges, 
                               const vector<double>& binning) 
{
  if (binning.size() < 3) {
    if(debug == 1)    cout<<"the size of binning =  "<< binning.size() <<endl;
    LogWarning("HLTElectronVal") << "Invalid binning parameters!"; 
    return;
  }

  // Fixed-width binning.
  if (binning.size() == 3) {
    nBins = binning[0];
    edges = new float[nBins + 1];
    const double min = binning[1];
    const double binwidth = (binning[2] - binning[1]) / nBins;
    for (size_t i = 0; i <= nBins; i++) edges[i] = min + (binwidth * i);
  } 

  // Variable-width binning.
  else {
    nBins = binning.size() - 1;
    edges = new float[nBins + 1];
    for (size_t i = 0; i <= nBins; i++) edges[i] = binning[i];
  }

}



// This is an unorthodox method of getting parameters, but cleaner in my mind
// It looks for parameter 'target' in the pset, and assumes that all entries
// have the same class (T), filling the values into a std::map.
template <class T>
void 
HLTElectronMatchAndPlot::fillMapFromPSet(map<string, T> & m, 
                                     const ParameterSet& pset, string target) 
{

  // Get the ParameterSet with name 'target' from 'pset'
  // target: "binParams" and "plotCuts"
  ParameterSet targetPset;
  if (pset.existsAs<ParameterSet>(target, true))        // target is tracked
    targetPset = pset.getParameterSet(target);
  else if (pset.existsAs<ParameterSet>(target, false))  // target is untracked
    targetPset = pset.getUntrackedParameterSet(target);
  
  // Get the parameter names from targetPset
  vector<string> names = targetPset.getParameterNames();
  vector<string>::const_iterator iter;
  
  for (iter = names.begin(); iter != names.end(); ++iter) {
    if (targetPset.existsAs<T>(* iter, true))           // target is tracked
      m[* iter] = targetPset.getParameter<T>(* iter);
    else if (targetPset.existsAs<T>(* iter, false))     // target is untracked
      m[* iter] = targetPset.getUntrackedParameter<T>(* iter);
  }

}



// A generic method to find the best deltaR matches from 2 collections.
template <class T1, class T2> 
vector<size_t> 
HLTElectronMatchAndPlot::matchByDeltaR(const vector<T1> & collection1, 
                                   const vector<T2> & collection2,
                                   const double maxDeltaR) 
{

  const size_t n1 = collection1.size();
  const size_t n2 = collection2.size();

  vector<size_t> result(n1, -1);
  vector<vector<double> > deltaRMatrix(n1, vector<double>(n2, NOMATCH));

  for (size_t i = 0; i < n1; i++)
    for (size_t j = 0; j < n2; j++) {
      deltaRMatrix[i][j] = deltaR(collection1[i], collection2[j]);
    }

  // Run through the matrix n1 times to make sure we've found all matches.
  for (size_t k = 0; k < n1; k++) {
    size_t i_min = -1;
    size_t j_min = -1;
    double minDeltaR = maxDeltaR;
    // find the smallest deltaR
    for (size_t i = 0; i < n1; i++)
      for (size_t j = 0; j < n2; j++)
        if (deltaRMatrix[i][j] < minDeltaR) {
          i_min = i;
          j_min = j;
          minDeltaR = deltaRMatrix[i][j];
        }
    // If a match has been made, save it and make those candidates unavailable.
    if (minDeltaR < maxDeltaR) {
      result[i_min] = j_min;
      deltaRMatrix[i_min] = vector<double>(n2, NOMATCH);
      for (size_t i = 0; i < n1; i++)
        deltaRMatrix[i][j_min] = NOMATCH;
    }
  }

  return result;

}



GsfElectronCollection
HLTElectronMatchAndPlot::selectedElectrons(const GsfElectronCollection & eleHandle, 
                                   const BeamSpot & beamSpot,
                                   bool hasRecoCuts,
                                   const StringCutObjectSelector<reco::GsfElectron> &selector)
//                                   double d0Cut, double z0Cut
{
  
  // If there is no selector (recoCuts does not exists), return an empty collection. 
  if (!hasRecoCuts)
    return GsfElectronCollection();
  GsfElectronCollection reducedElectrons;
/*
  for (auto const& mu : allMuons){
    const Track * track = 0;
    if (mu.isTrackerMuon()) track = & * mu.innerTrack();
    else if (mu.isStandAloneMuon()) track = & * mu.outerTrack();
    if (track && selector(mu) &&
        fabs(track->dxy(beamSpot.position())) < d0Cut &&
        fabs(track->dz(beamSpot.position())) < z0Cut)
      reducedMuons.push_back(mu);
  } 
*/
  for (auto const& ele  : eleHandle){


    const Track * track = 0;
//    if (ele.isTightEletron()) track = & * ele.GsfTrack();
    track = & * ele.gsfTrack();
    if( track && selector(ele) )
      reducedElectrons.push_back(ele);
  } 
  return reducedElectrons;
}



TriggerObjectCollection
HLTElectronMatchAndPlot::selectedTriggerObjects(
  const TriggerObjectCollection & triggerObjects,
  const TriggerEvent & triggerSummary,
  bool hasTriggerCuts,
  const StringCutObjectSelector<TriggerObject> triggerSelector)
{
  if ( !hasTriggerCuts) return TriggerObjectCollection();

  InputTag filterTag(moduleLabel_, "", hltProcessName_);
  size_t filterIndex = triggerSummary.filterIndex(filterTag);

  TriggerObjectCollection selectedObjects;

  if (filterIndex < triggerSummary.sizeFilters()) {
    const Keys &keys = triggerSummary.filterKeys(filterIndex);
    for (size_t j = 0; j < keys.size(); j++ ){
      TriggerObject foundObject = triggerObjects[keys[j]];
      if (triggerSelector(foundObject))
        selectedObjects.push_back(foundObject);
    }
  }

  return selectedObjects;

}

/*
bool HLTElectronMatchAndPlot::isoCut()
{

  bool iso = false;
  if
  //      ! ConversionTools::hasMatchedConversion(electron, convs, beamSpot->position());
}
*/


void HLTElectronMatchAndPlot::book1D(DQMStore::IBooker & iBooker, string name, 
				 string binningType, string title)
{

  /* Properly delete the array of floats that has been allocated on
   * the heap by fillEdges.  Avoid multiple copies and internal ROOT
   * clones by simply creating the histograms directly in the DQMStore
   * using the appropriate book1D method to handle the variable bins
   * case. */ 

  size_t nBins; 
  float * edges = 0; 
//  cout<<"book1D function "<< name <<endl;
  fillEdges(nBins, edges, binParams_[binningType]);

  hists_[name] = iBooker.book1D(name, title, nBins, edges);
  if (hists_[name])
    if (hists_[name]->getTH1F()->GetSumw2N())
      hists_[name]->getTH1F()->Sumw2();

  if (edges)
    delete [] edges;

}



void
HLTElectronMatchAndPlot::book2D(DQMStore::IBooker & iBooker, string name, 
			    string binningTypeX, string binningTypeY, 
			    string title) 
{
  
  /* Properly delete the arrays of floats that have been allocated on
   * the heap by fillEdges.  Avoid multiple copies and internal ROOT
   * clones by simply creating the histograms directly in the DQMStore
   * using the appropriate book2D method to handle the variable bins
   * case. */ 

  size_t  nBinsX;
  float * edgesX = 0;
//  cout<<"book2D function "<< name <<endl;
  fillEdges(nBinsX, edgesX, binParams_[binningTypeX]);

  size_t  nBinsY;
  float * edgesY = 0;
  fillEdges(nBinsY, edgesY, binParams_[binningTypeY]);

  hists_[name] = iBooker.book2D(name.c_str(), title.c_str(),
				nBinsX, edgesX, nBinsY, edgesY);
  if (hists_[name])
    if (hists_[name]->getTH2F()->GetSumw2N())
      hists_[name]->getTH2F()->Sumw2();

  if (edgesX)
    delete [] edgesX;
  if (edgesY)
    delete [] edgesY;

}
#endif

