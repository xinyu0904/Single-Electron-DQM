import FWCore.ParameterSet.Config as cms

#from DQM.HLTEvF.HLTEventInfoClient_cfi import *

#from DQMOffline.Trigger.GeneralHLTOfflineClient_cff import *
from DQMOffline.Trigger.EgHLTOfflineClient_cfi import *
from DQMOffline.Trigger.MuonPostProcessor_cff import *
from DQMOffline.Trigger.ElectronPostProcessor_cff import * #Sijing
#from DQMOffline.Trigger.BPAGPostProcessor_cff import *
from DQMOffline.Trigger.JetMETHLTOfflineClient_cfi import *
#from DQMOffline.Trigger.TnPEfficiencyPostProcessor_cff import *
from DQMOffline.Trigger.HLTTauPostProcessor_cfi import *
from DQMOffline.Trigger.DQMOffline_HLT_Cert_cff import *
from DQMOffline.Trigger.HLTInclusiveVBFClient_cfi import *
from DQMOffline.Trigger.FSQHLTOfflineClient_cfi import  *
from DQMOffline.Trigger.HILowLumiHLTOfflineClient_cfi import  *

from DQMOffline.Trigger.TrackingMonitoring_Client_cff import *
from DQMOffline.Trigger.TrackingMonitoringPA_Client_cff import *

from DQMOffline.Trigger.ExoticaMonitoring_Client_cff import *

hltOfflineDQMClient = cms.Sequence(
#    hltGeneralSeqClient *
    egHLTOffDQMClient *
    hltMuonPostVal *
    hltElectronPostVal * #Sijing
    jetMETHLTOfflineClient *
    fsqClient *
    HiJetClient * 
    #tagAndProbeEfficiencyPostProcessor *
    HLTTauPostSeq *
    dqmOfflineHLTCert *
    hltInclusiveVBFClient *
    exoticaClient
    )
