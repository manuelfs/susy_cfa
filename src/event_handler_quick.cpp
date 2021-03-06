#include "event_handler_quick.hpp"

#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <limits>
#include <algorithm>

#include "TString.h"
#include "TLorentzVector.h"
#include "TVector3.h"
#include "TVector2.h"
#include "TFile.h"
#include "TROOT.h"
#include "TH1F.h"
#include "fastjet/ClusterSequence.hh"

#include "event_handler_base.hpp"
#include "phys_objects.hpp"
#include "utilities.hpp"
#include "small_tree_quick.hpp"
#include "timer.hpp"

using namespace std;
using namespace fastjet;

//const double CSVCuts[] = {0.244, 0.679, 0.898}; //Run 1 CSV
const double CSVCuts[] = {0.605, 0.890, 0.970};   //Run 2 CSV+IVF

namespace{
  const float fltmax = numeric_limits<float>::max();
}

event_handler_quick::event_handler_quick(const string &file_name):
  event_handler_base(file_name){
}

void event_handler_quick::ReduceTree(int num_entries, const TString &out_file_name, int num_total_entries){
  TFile *puweights = TFile::Open("pu_weights.root");
  TH1F * h_wpu =  static_cast<TH1F*>(puweights->Get("sf"));

  TFile out_file(out_file_name, "recreate");
  out_file.cd();

  small_tree_quick tree;
  float xsec = cross_section(out_file_name);
  float luminosity = 1000.;

  vector<TString> trig_name;
  //Leaving 50ns triggers in comments
  trig_name.push_back("HLT_PFHT350_PFMET100_JetIdCleaned_v");           // 0 trig_name.push_back("HLT_PFHT350_PFMET100_NoiseCleaned_v");		  // 0
  trig_name.push_back("HLT_Mu15_IsoVVVL_PFHT350_PFMET50_v");            // 1 trig_name.push_back("HLT_Mu15_IsoVVVL_PFHT350_PFMET70_v");		  // 1
  trig_name.push_back("HLT_Mu15_IsoVVVL_PFHT600_v");			  // 2
  trig_name.push_back("HLT_Mu15_IsoVVVL_BTagCSV0p72_PFHT400_v");	  // 3
  trig_name.push_back("HLT_Mu15_IsoVVVL_PFHT350_v");                    // 4 trig_name.push_back("HLT_Mu15_PFHT300_v");				  // 4
  trig_name.push_back("HLT_Ele15_IsoVVVL_PFHT350_PFMET50_v");           // 5 trig_name.push_back("HLT_Ele15_IsoVVVL_PFHT350_PFMET70_v");		  // 5
  trig_name.push_back("HLT_Ele15_IsoVVVL_PFHT600_v");			  // 6
  trig_name.push_back("HLT_Ele15_IsoVVVL_BTagCSV0p72_PFHT400_v");	  // 7
  trig_name.push_back("HLT_Ele15_IsoVVVL_PFHT350_v");                   // 8 trig_name.push_back("HLT_Ele15_PFHT300_v");				  // 8
  trig_name.push_back("HLT_DoubleMu8_Mass8_PFHT300_v");			  // 9
  trig_name.push_back("HLT_DoubleEle8_CaloIdM_TrackIdM_Mass8_PFHT300_v"); // 10
  trig_name.push_back("HLT_PFHT475_v");					  // 11
  trig_name.push_back("HLT_PFHT800_v");					  // 12
  trig_name.push_back("HLT_PFMET120_NoiseCleaned_Mu5_v");		  // 13
  trig_name.push_back("HLT_PFMET170_NoiseCleaned_v");			  // 14
  trig_name.push_back("HLT_DoubleIsoMu17_eta2p1_v");			  // 15
  trig_name.push_back("HLT_Mu17_TrkIsoVVL_v");				  // 16
  trig_name.push_back("HLT_IsoMu17_eta2p1_v");				  // 17
  trig_name.push_back("HLT_IsoMu20_v");					  // 18
  trig_name.push_back("HLT_IsoMu24_eta2p1_v");				  // 19
  trig_name.push_back("HLT_IsoMu27_v");					  // 20
  trig_name.push_back("HLT_Mu50_v");					  // 21
  trig_name.push_back("HLT_Ele27_eta2p1_WPLoose_Gsf_v");		  // 22
  trig_name.push_back("HLT_Ele32_eta2p1_WPLoose_Gsf_v");		  // 23
  trig_name.push_back("HLT_Ele105_CaloIdVT_GsfTrkIdT_v");		  // 24
  trig_name.push_back("HLT_DoubleEle33_CaloIdL_GsfTrkIdVL_MW_v");	  // 25
  trig_name.push_back("HLT_DoubleEle24_22_eta2p1_WPLoose_Gsf_v");	  // 26

  // This is to save only events in data for which we store the trigger
  if(isData() && yes_trig.size()==0) yes_trig=trig_name; 
  

  Timer timer(num_entries, 1.);
  timer.Start();
  for(int entry = 0; entry < num_entries; ++entry){
    timer.Iterate();
    GetEntry(entry);

    // Skipping events in the 17Jul2015 re-RECO
    if(out_file_name.Contains("Run2015B-PromptReco") && run() < 251585) continue;

    /////////JSON////////
    bool golden(PassesJSONCut("golden"));
    if(isData() && !(PassesJSONCut("dcs") || golden)) continue;	// Only saving events with good DCS or golden. DCS not necessarily strict superset of golden now.
    tree.json_golden() = golden; // Golden JSON
    //tree.json_dcs()=PassesJSONCut("dcs"); // DCS JSON

     ///////// Triggers ///////
    vector<bool> trig_decision;
    vector<float> trig_prescale;
    if(!GetTriggerInfo(trig_name, trig_decision, trig_prescale)) continue;
    tree.trig()=trig_decision;
    tree.trig_prescale()=trig_prescale;

    //cout<<endl<<endl<<"Entry "<<entry<<endl;
    for(unsigned ind(0); ind<standalone_triggerobject_collectionname()->size(); ind++){
      TString name(standalone_triggerobject_collectionname()->at(ind));
      float objpt(standalone_triggerobject_pt()->at(ind));
      if(name.Contains("MET")||name.Contains("HT")||name.Contains("Combined")||name.Contains("Mu")||
      	 name.Contains("Ele")||name.Contains("Egamma"))
	// cout<<name<<": pt "<<objpt<<", energy "<<standalone_triggerobject_energy()->at(ind)
	//     <<", et "<<standalone_triggerobject_et()->at(ind)<<endl;

      if(name=="hltPFMETProducer::HLT") tree.onmet() = objpt;
      if(name=="hltPFHT::HLT") tree.onht() = objpt; // There's 2 of these, and we want the last one
      if(name=="hltL3MuonCandidates::HLT" && tree.onmaxmu()<objpt) tree.onmaxmu() = objpt;
      if(name=="hltEgammaCandidates::HLT" && tree.onmaxel()<objpt) tree.onmaxel() = objpt;
    }

    tree.event() = event();
    tree.lumiblock() = lumiblock();
    tree.run() = run();
    if(isData()) tree.weight() = 1.;
    else tree.weight() = Sign(weight())*xsec*luminosity / static_cast<double>(num_total_entries);

    tree.npv() = Npv();
    if(isData()) tree.wpu() = 1.;
    else tree.wpu() = h_wpu->GetBinContent(h_wpu->FindBin(tree.npv()));
    
    for(size_t bc(0); bc<PU_bunchCrossing()->size(); ++bc){
      if(PU_bunchCrossing()->at(bc)==0){
        tree.ntrupv() = PU_NumInteractions()->at(bc);
        tree.ntrupv_mean() = PU_TrueNumInteractions()->at(bc);
        break;
      }
    }
    ///////////// MET //////////////////
    tree.met() = met_corr();
    tree.met_phi() = met_phi_corr();
    tree.met_mini() = pfType1mets_default_et()->at(0);
    tree.met_mini_phi() = pfType1mets_default_et()->at(0);
    tree.mindphin_metjet() = GetMinDeltaPhiMETN(3, 50., 2.4, 30., 2.4, true);

    int nnus = 0, nnus_fromw = 0;
    double genmetx = 0., genmety = 0., genmetx_fromw = 0, genmety_fromw = 0;
      for(size_t imc = 0; imc < mc_final_id()->size(); ++imc){
      int id = abs(TMath::Nint(mc_final_id()->at(imc)));
      double px = mc_final_pt()->at(imc)*cos(mc_final_phi()->at(imc));
      double py = mc_final_pt()->at(imc)*sin(mc_final_phi()->at(imc));
      if(id==12 || id==14 || id==16 || id==18
         || id==1000012 || id==1000014 || id==1000016
         || id==1000022 || id==1000023 || id==1000025 || id==1000035 || id==1000039){

        genmetx += px;
        genmety += py;
	nnus++;

	bool nufromw=false;
	GetMom(mc_final_id()->at(imc),mc_final_mother_id()->at(imc),mc_final_grandmother_id()->at(imc),mc_final_ggrandmother_id()->at(imc), nufromw);

	if(nufromw){
	  genmetx_fromw += px;
	  genmety_fromw += py;
	  nnus_fromw++;
	}
      }
    }
    tree.gen_met() = AddInQuadrature(genmetx, genmety);
    tree.gen_met_phi() = atan2(genmety, genmetx);
    tree.ntrunus() = nnus;

    tree.gen_met_fromw() = AddInQuadrature(genmetx_fromw, genmety_fromw);
    tree.gen_met_phi_fromw() = atan2(genmety_fromw, genmetx_fromw);
    tree.ntrunus_fromw() = nnus_fromw;

    // MET filters
    tree.pass_hbhe()    = static_cast<bool>(HBHENoisefilter_decision());
    tree.pass_cschalo() = static_cast<bool>(cschalofilter_decision());
    tree.pass_eebadsc() = static_cast<bool>(eebadscfilter_decision());

    bool one_good_pv(false);
    for(unsigned ipv(0); ipv < pv_z()->size(); ipv++){
      const double pv_rho(sqrt(pv_x()->at(ipv)*pv_x()->at(ipv) + pv_y()->at(ipv)*pv_y()->at(ipv)));
      if(pv_ndof()->at(ipv)>4 && fabs(pv_z()->at(ipv))<24. && pv_rho<2.0 && pv_isFake()->at(ipv)==0){
	one_good_pv = true;
	break;
      }
    } // Loop over vertices
    tree.pass_goodv() = one_good_pv && static_cast<bool>(goodVerticesfilter_decision());
    // pass_jets() is true if all jets in the event (not matched to leptons) pass loose ID
    vector<int> sig_electrons = GetElectrons(true, true);
    vector<int> sig_muons = GetMuons(true, true);
    tree.pass_jets() = AllGoodJets(sig_electrons, sig_muons, phys_objects::MinJetPt , fltmax);

    tree.pass() = tree.pass_hbhe()&&tree.pass_goodv()&&tree.pass_cschalo()&&tree.pass_eebadsc()&&tree.pass_jets();


    TLorentzVector lepmax_p4(0., 0., 0., 0.), lepmax_p4_reliso(0., 0., 0., 0.);
    short lepmax_chg = 0, lepmax_chg_reliso = 0;
    vector<int> sig_electrons_reliso = GetElectrons(true, false);
    vector<int> sig_muons_reliso = GetMuons(true, false);
    vector<int> veto_electrons = GetElectrons(false, true);
    vector<int> veto_muons = GetMuons(false, true);
    vector<int> veto_electrons_reliso = GetElectrons(false, false);
    vector<int> veto_muons_reliso = GetMuons(false, false);
    tree.nels() = sig_electrons.size(); tree.nvels() = veto_electrons.size();
    tree.nels_reliso() = sig_electrons_reliso.size(); tree.nvels_reliso() = veto_electrons_reliso.size();
    tree.nmus() = sig_muons.size(); tree.nvmus() = veto_muons.size();
    tree.nmus_reliso() = sig_muons_reliso.size(); tree.nvmus_reliso() = veto_muons_reliso.size();

    int els_tru_prompt = 0;

    for(size_t index(0); index<els_pt()->size(); index++) {
      if (els_pt()->at(index) > MinVetoLeptonPt && IsVetoIdElectron(index)) {
        tree.els_sigid().push_back(IsSignalIdElectron(index));
        tree.els_tight().push_back(IsIdElectron(index, kTight, false));
        tree.els_ispf().push_back(els_isPF()->at(index));
        tree.els_pt().push_back(els_pt()->at(index));
        tree.els_eta().push_back(els_eta()->at(index));
        tree.els_sceta().push_back(els_scEta()->at(index));
        tree.els_phi().push_back(els_phi()->at(index));
        tree.els_charge().push_back(TMath::Nint(els_charge()->at(index)));
        tree.els_mt().push_back(GetMT(els_pt()->at(index), els_phi()->at(index),
                                      met_corr(), met_phi_corr()));
        tree.els_d0().push_back(els_d0dum()->at(index)
                                -pv_x()->at(0)*sin(els_tk_phi()->at(index))
                                +pv_y()->at(0)*cos(els_tk_phi()->at(index)));
        tree.els_dz().push_back(els_vz()->at(index)-pv_z()->at(0));

	for(size_t iel(index+1); iel<els_pt()->size(); iel++) {
	  if(IsVetoIdElectron(index)   && IsVetoIdElectron(iel))   Setllmass(tree, index, iel, 11, false);
	  if(IsSignalIdElectron(index) && IsSignalIdElectron(iel)) Setllmass(tree, index, iel, 11, true);
	}

        // MC truth
        bool fromW = false;
        int mcmomID;
        float deltaR;
	double els_mc_pt, els_mc_phi;
        int mcID = GetTrueElectron(static_cast<int>(index), mcmomID, fromW, deltaR, els_mc_pt, els_mc_phi);
        tree.els_tru_id().push_back(mcID);
        tree.els_tru_momid().push_back(mcmomID);
        tree.els_tru_tm().push_back(abs(mcID)==pdtlund::e_minus && fromW);
        tree.els_tru_dr().push_back(deltaR);
	tree.els_tru_pt().push_back(els_mc_pt);
	tree.els_tru_phi().push_back(els_mc_phi);

	if(abs(mcID)==pdtlund::e_minus && fromW && IsSignalElectron(index,true)){
	  els_tru_prompt++;
	  tree.els_genmt().push_back(GetMT(els_mc_pt, els_mc_phi,
					   tree.gen_met(), tree.gen_met_phi()));
	  tree.els_genmt_fromw().push_back(GetMT(els_mc_pt, els_mc_phi,
				           tree.gen_met_fromw(), tree.gen_met_phi_fromw()));
	}

        // Isolation
        tree.els_reliso().push_back(GetElectronIsolation(index, false));
        SetMiniIso(tree, index, 11);

        // Max pT lepton
        if(els_pt()->at(index) > lepmax_p4.Pt() && IsSignalIdElectron(index) && tree.els_miniso().back()<0.1){
          lepmax_chg = Sign(els_charge()->at(index));
          lepmax_p4 = TLorentzVector(els_px()->at(index), els_py()->at(index),
                                     els_pz()->at(index), els_energy()->at(index));
        }
        if(els_pt()->at(index) > lepmax_p4_reliso.Pt() && IsSignalElectron(index)){
          lepmax_chg_reliso = Sign(els_charge()->at(index));
          lepmax_p4_reliso = TLorentzVector(els_px()->at(index), els_py()->at(index),
                                            els_pz()->at(index), els_energy()->at(index));
        }
      }
    } // Loop over els

    tree.nels_tru_prompt() = els_tru_prompt;

    int mus_tru_prompt = 0;

    for(size_t index(0); index<mus_pt()->size(); index++) {
      if (mus_pt()->at(index) > MinVetoLeptonPt && IsVetoIdMuon(index)) {
        tree.mus_sigid().push_back(IsSignalIdMuon(index));
        tree.mus_tight().push_back(IsIdMuon(index, kTight));
        tree.mus_pt().push_back(mus_pt()->at(index));
        tree.mus_eta().push_back(mus_eta()->at(index));
        tree.mus_phi().push_back(mus_phi()->at(index));
        tree.mus_charge().push_back(TMath::Nint(mus_charge()->at(index)));
        tree.mus_mt().push_back(GetMT(mus_pt()->at(index), mus_phi()->at(index),
                                      met_corr(), met_phi_corr()));
        tree.mus_d0().push_back(mus_tk_d0dum()->at(index)
                                -pv_x()->at(0)*sin(mus_tk_phi()->at(index))
                                +pv_y()->at(0)*cos(mus_tk_phi()->at(index)));
        tree.mus_dz().push_back(mus_tk_vz()->at(index)-pv_z()->at(0));
	for(size_t imu(index+1); imu<mus_pt()->size(); imu++) {
	  if(IsVetoIdMuon(index)   && IsVetoIdMuon(imu))   Setllmass(tree, index, imu, 13, false);
	  if(IsSignalIdMuon(index) && IsSignalIdMuon(imu)) Setllmass(tree, index, imu, 13, true);
	}

        // MC truth
        bool fromW = false;
        int mcmomID;
        float deltaR;
	double mus_mc_pt, mus_mc_phi;
	int mcID = GetTrueMuon(static_cast<int>(index), mcmomID, fromW, deltaR, mus_mc_pt, mus_mc_phi);
        tree.mus_tru_id().push_back(mcID);
        tree.mus_tru_momid().push_back(mcmomID);
        tree.mus_tru_tm().push_back(abs(mcID)==pdtlund::mu_minus && fromW);
        tree.mus_tru_dr().push_back(deltaR);
	tree.mus_tru_pt().push_back(mus_mc_pt);
	tree.mus_tru_phi().push_back(mus_mc_phi);

        // Isolation
        tree.mus_reliso().push_back(GetMuonIsolation(index, false));
        SetMiniIso(tree, index, 13);

	if(abs(mcID)==pdtlund::mu_minus && fromW && IsSignalMuon(index,true)){ 
	  mus_tru_prompt++;
	  tree.mus_genmt().push_back(GetMT(mus_mc_pt, mus_mc_phi,
					   tree.gen_met(), tree.gen_met_phi()));
	  tree.mus_genmt_fromw().push_back(GetMT(mus_mc_pt, mus_mc_phi,
					   tree.gen_met_fromw(), tree.gen_met_phi_fromw()));
	}

        // Max pT lepton
        if(mus_pt()->at(index) > lepmax_p4.Pt() && IsSignalIdMuon(index) && tree.mus_miniso().back()<0.2){
          lepmax_chg = Sign(mus_charge()->at(index));
          lepmax_p4 = TLorentzVector(mus_px()->at(index), mus_py()->at(index),
                                     mus_pz()->at(index), mus_energy()->at(index));
        }
        if(mus_pt()->at(index) > lepmax_p4_reliso.Pt() && IsSignalMuon(index)){
          lepmax_chg_reliso = Sign(mus_charge()->at(index));
          lepmax_p4_reliso = TLorentzVector(mus_px()->at(index), mus_py()->at(index),
                                            mus_pz()->at(index), mus_energy()->at(index));
        }
      }
    } // Loop over mus

    tree.nmus_tru_prompt() = mus_tru_prompt;

    tree.nleps() = tree.nels() + tree.nmus();
    tree.nvleps() = tree.nvels() + tree.nvmus();

    tree.nleps_reliso() = tree.nels_reliso() + tree.nmus_reliso();
    

    if(lepmax_p4.Pt()>0.){
      tree.lep_pt() = lepmax_p4.Pt();
      tree.lep_phi() = lepmax_p4.Phi();
      tree.lep_eta() = lepmax_p4.Eta();
      tree.lep_charge() = lepmax_chg;
      tree.st() = lepmax_p4.Pt()+met_corr();

      float wx = mets_et()*cos(mets_phi()) + lepmax_p4.Px();
      float wy = mets_et()*sin(mets_phi()) + lepmax_p4.Py();
      float wphi = atan2(wy, wx);

      tree.dphi_wlep() = DeltaPhi(wphi, lepmax_p4.Phi());
      tree.mt() = GetMT(lepmax_p4.Pt(), lepmax_p4.Phi(), met_corr(), met_phi_corr());
    }

    if(lepmax_p4_reliso.Pt()>0.){
      tree.lep_pt_reliso() = lepmax_p4_reliso.Pt();
      tree.lep_phi_reliso() = lepmax_p4_reliso.Phi();
      tree.lep_eta_reliso() = lepmax_p4_reliso.Eta();
      tree.lep_charge_reliso() = lepmax_chg_reliso;
      tree.st_reliso() = lepmax_p4_reliso.Pt()+met_corr();

      float wx = mets_et()*cos(mets_phi()) + lepmax_p4_reliso.Px();
      float wy = mets_et()*sin(mets_phi()) + lepmax_p4_reliso.Py();
      float wphi = atan2(wy, wx);

      tree.dphi_wlep_reliso() = DeltaPhi(wphi, lepmax_p4_reliso.Phi());
      tree.mt_reliso() = GetMT(lepmax_p4_reliso.Pt(), lepmax_p4_reliso.Phi(), met_corr(), met_phi_corr());
    }

    // vector<mc_particle> parts = GetMCParticles();
    // vector<size_t> moms = GetMoms(parts);
    // tree.mc_type() = TypeCode(parts, moms);

    //**************** No HF variables ***************//
    //float metnohf(mets_NoHF_et()), metnohfphi(mets_NoHF_phi()), metnohfsumet(mets_NoHF_sumEt());
    float metnohf(mets_et()), metnohfphi(mets_phi()), metnohfsumet(mets_sumEt());

    tree.met_nohf() = metnohf;
    tree.met_nohf_phi() = metnohfphi;
    tree.met_nohf_sumEt() = metnohfsumet; 

    float met_hf_x = met_corr()*cos(met_phi_corr()) - metnohf*cos(metnohfphi);
    float met_hf_y = met_corr()*sin(met_phi_corr()) - metnohf*sin(metnohfphi);    
    tree.met_hf() = sqrt(met_hf_x*met_hf_x + met_hf_y*met_hf_y); 
    tree.met_hf_phi() = atan2(met_hf_y,met_hf_x);

    vector<int> good_jets_nohf = GetJets(sig_electrons, sig_muons, phys_objects::MinJetPt , 3.0);
    vector<int> good_jets_eta5 = GetJets(sig_electrons, sig_muons, phys_objects::MinJetPt , 5.0);

    float ht_nohf = GetHT(good_jets_nohf, MinJetPt);
    float ht_eta5 = GetHT(good_jets_eta5, MinJetPt);
    tree.ht_hf() = ht_eta5 - ht_nohf;
    
    int njets_nohf = GetNumJets(good_jets_nohf, MinJetPt);
    int njets_eta5 = GetNumJets(good_jets_eta5, MinJetPt);
    tree.njets_nohf() = njets_nohf;
    tree.njets_hf() = njets_eta5 - njets_nohf;

    tree.hfjet() = (njets_eta5 - njets_nohf)>0;

    //**************** No HF variables ***************//

    vector<int> good_jets = GetJets(sig_electrons, sig_muons, phys_objects::MinJetPt , 2.4);
    vector<int> good_jets_reliso = GetJets(sig_electrons_reliso, sig_muons_reliso, phys_objects::MinJetPt , 2.4);
    tree.njets() = GetNumJets(good_jets, MinJetPt);
    tree.nbl() = GetNumJets(good_jets, MinJetPt, CSVCuts[0]);
    tree.nbm() = GetNumJets(good_jets, MinJetPt, CSVCuts[1]);
    tree.nbt() = GetNumJets(good_jets, MinJetPt, CSVCuts[2]);
    tree.ht() = GetHT(good_jets, MinJetPt);
    tree.ht_reliso() = GetHT(good_jets_reliso, MinJetPt);
    tree.mht() = GetMHT(good_jets, MinJetPt);

    vector<int> good_jets40 = GetJets(sig_electrons, sig_muons, 40. , 2.4);
    tree.njets40() = GetNumJets(good_jets40, MinJetPt);
    tree.nbl40() = GetNumJets(good_jets40, MinJetPt, CSVCuts[0]);
    tree.nbm40() = GetNumJets(good_jets40, MinJetPt, CSVCuts[1]);
    tree.nbt40() = GetNumJets(good_jets40, MinJetPt, CSVCuts[2]);
    tree.ht40() = GetHT(good_jets40, MinJetPt);

    vector<int> dirty_jets = GetJets(vector<int>(0), vector<int>(0), 30., 5.0);
    tree.jets_pt().resize(dirty_jets.size());
    tree.jets_eta().resize(dirty_jets.size());
    tree.jets_phi().resize(dirty_jets.size());
    tree.jets_m().resize(dirty_jets.size());
    tree.jets_csv().resize(dirty_jets.size());
    tree.jets_id().resize(dirty_jets.size());
    tree.jets_islep().resize(dirty_jets.size());
    for(size_t idirty = 0; idirty < dirty_jets.size(); ++idirty){
      int ijet = dirty_jets.at(idirty);
      tree.jets_pt().at(idirty) = jets_corr_p4().at(ijet).Pt();
      tree.jets_eta().at(idirty) = jets_corr_p4().at(ijet).Eta();
      tree.jets_phi().at(idirty) = jets_corr_p4().at(ijet).Phi();
      tree.jets_m().at(idirty) = jets_corr_p4().at(ijet).M();
      tree.jets_csv().at(idirty) = jets_btag_inc_secVertexCombined()->at(ijet);
      tree.jets_id().at(idirty) = jets_parton_Id()->at(ijet);
      tree.jets_islep().at(idirty) = !(find(good_jets.begin(), good_jets.end(), ijet) != good_jets.end());
    }
    tree.mht_ra2b() = GetMHT(dirty_jets, 30);

    vector<int> ra2b_jets = GetJets(vector<int>(0), vector<int>(0), 30., 2.4);
    tree.njets_ra2b() = GetNumJets(ra2b_jets, MinJetPt);
    tree.nbm_ra2b() = GetNumJets(ra2b_jets, MinJetPt, CSVCuts[1]);
    tree.ht_ra2b() = GetHT(ra2b_jets, MinJetPt);
    
    
    vector<int> hlt_jets = GetJets(vector<int>(0), vector<int>(0), 40., 3.0);
    tree.ht_hlt() = GetHT(hlt_jets, 40);


    vector<int> mj_jets;
    vector<bool> mj_jets_islep;
    map<size_t,vector<size_t> > mu_matches, el_matches;
    GetMatchedLeptons(sig_muons, sig_electrons, mu_matches, el_matches);
    for(unsigned ijet(0); ijet<jets_corr_p4().size(); ijet++) {
      if(mu_matches.find(ijet) != mu_matches.end() || el_matches.find(ijet) != el_matches.end()){
	mj_jets.push_back(static_cast<int>(ijet));
	mj_jets_islep.push_back(true);
      } // If lepton in jet
      // if(mu_matches.find(ijet) != mu_matches.end() && el_matches.find(ijet) != el_matches.end()) {
      // 	size_t iel = el_matches[ijet][0];
      // 	size_t imu = mu_matches[ijet][0];
      // 	cout<<entry<<": Both els and mus match: jet p=("<<jets_pt()->at(ijet)<<","<<jets_eta()->at(ijet)
      // 	    <<","<<jets_phi()->at(ijet)<<"). el p = ("<<els_pt()->at(iel)<<","<<els_eta()->at(iel)
      // 	    <<","<<els_phi()->at(iel)<<"). mu p = ("<<mus_pt()->at(imu)<<","<<mus_eta()->at(imu)
      // 	    <<","<<mus_phi()->at(imu)<<")"<<endl;
      // }
    } // Loop over all jets
    // Adding all clean jets
    for(unsigned ijet(0); ijet<good_jets.size(); ijet++) {
      mj_jets.push_back(good_jets[ijet]);
      mj_jets_islep.push_back(false);      
    } // Loop over good jets

    WriteFatJets(tree.nfjets(), tree.mj(),
                 tree.fjets_pt(), tree.fjets_eta(),
                 tree.fjets_phi(), tree.fjets_m(),
                 tree.fjets_nconst(),
                 tree.fjets_sumcsv(), tree.fjets_poscsv(),
                 tree.fjets_btags(), tree.jets_fjet_index(),
                 1.2, mj_jets);
    WriteFatJets(tree.nfjets08(), tree.mj08(),
                 tree.fjets08_pt(), tree.fjets08_eta(),
                 tree.fjets08_phi(), tree.fjets08_m(),
                 tree.fjets08_nconst(),
                 tree.fjets08_sumcsv(), tree.fjets08_poscsv(),
                 tree.fjets08_btags(), tree.jets_fjet08_index(),
                 0.8, mj_jets);

    /////////////////////////////////  MC  ///////////////////////////////
    std::vector<int> mc_mus, mc_els, mc_taush, mc_tausl;
    GetTrueLeptons(mc_els, mc_mus, mc_taush, mc_tausl);
    tree.ntrumus()   = mc_mus.size();
    tree.ntruels()   = mc_els.size();
    tree.ntrutaush() = mc_taush.size();
    tree.ntrutausl() = mc_tausl.size();
    tree.ntruleps()  = tree.ntrumus()+tree.ntruels()+tree.ntrutaush()+tree.ntrutausl();

    //for systematics:
    float toppt1(0),toppt2(0),topphi1(0),topphi2(0);
    int nisr(0);
    for(unsigned i = 0; i < mc_doc_id()->size(); ++i){
      const int id = static_cast<int>(floor(fabs(mc_doc_id()->at(i))+0.5));
      const int mom = static_cast<int>(floor(fabs(mc_doc_mother_id()->at(i))+0.5));
      const int gmom = static_cast<int>(floor(fabs(mc_doc_grandmother_id()->at(i))+0.5));
      const int ggmom = static_cast<int>(floor(fabs(mc_doc_ggrandmother_id()->at(i))+0.5));
      if(mc_doc_id()->at(i)==6) {toppt1 = mc_doc_pt()->at(i); topphi1 = mc_doc_phi()->at(i);}
      if(mc_doc_id()->at(i)==(-6)){ toppt2 = mc_doc_pt()->at(i); topphi2 = mc_doc_phi()->at(i);}
      if(mc_doc_status()->at(i)==23 && id!=6 && mom!=6 && mom!=24 && gmom!=6 && ggmom!=6) nisr++;
    }
    tree.trutop1_pt() = toppt1;
    tree.trutop2_pt() = toppt2;
    tree.trutop1_phi() = topphi1;
    tree.trutop2_phi() = topphi2;
    tree.ntrumeisr() = nisr;
    
    tree.genht() = genHT();
 
    // int nobj = tree.njets() + tree.nmus() + tree.nels();
    // int nobj_mj(0);
    // //int nobj_list(mj_jets.size());
    // for(unsigned ijet(0); ijet < tree.fjets_nconst().size(); ijet++)
    //   nobj_mj += tree.fjets_nconst().at(ijet);
    // if(nobj != nobj_mj) cout<<entry<<": nobj "<<nobj<<", nobj_mj "<<nobj_mj<<endl;

    // if(tree.njets() != static_cast<int>(good_jets.size())) 
    //   cout<<entry<<": njets "<<tree.njets()<<", gj_size "<<good_jets.size()<<endl;

    tree.Fill();
 }

  tree.Write();

  TString model = model_params()->c_str();
  TString commit = RemoveTrailingNewlines(execute("git rev-parse HEAD"));
  TString type = tree.Type();
  TString root_version = gROOT->GetVersion();
  TString root_tutorial_dir = gROOT->GetTutorialsDir();
  TTree treeglobal("treeglobal", "treeglobal");
  treeglobal.Branch("nev_file", &num_entries);
  treeglobal.Branch("nev_sample", &num_total_entries);
  treeglobal.Branch("commit", &commit);
  treeglobal.Branch("model", &model);
  treeglobal.Branch("type", &type);
  treeglobal.Branch("root_version", &root_version);
  treeglobal.Branch("root_tutorial_dir", &root_tutorial_dir);
  treeglobal.Branch("trig_name", &trig_name);
  treeglobal.Fill();
  treeglobal.Write();
  out_file.Close();
  puweights->Close();
}



event_handler_quick::~event_handler_quick(){
}

void event_handler_quick::WriteFatJets(int &nfjets,
                                       float &mj,
                                       vector<float> &fjets_pt,
                                       vector<float> &fjets_eta,
                                       vector<float> &fjets_phi,
                                       vector<float> &fjets_m,
                                       vector<int> &fjets_nconst,
                                       vector<float> &fjets_sumcsv,
                                       vector<float> &fjets_poscsv,
                                       vector<int> &fjets_btags,
                                       vector<int> &jets_fjet_index,
                                       double radius,
                                       const vector<int> &jets,
                                       bool gen,
                                       bool clean,
                                       const vector<bool> &to_clean){
  vector<PseudoJet> sjets(0);
  vector<int> ijets(0);
  vector<float> csvs(0);
  jets_fjet_index = vector<int>(jets.size(), -1);

  if(gen){
    for(size_t idirty = 0; idirty<jets.size(); ++idirty){
      int jet = jets.at(idirty);
      TLorentzVector v;
      v.SetPtEtaPhiE(mc_jets_pt()->at(jet), mc_jets_eta()->at(jet),
                     mc_jets_phi()->at(jet), mc_jets_energy()->at(jet));
      const PseudoJet this_pj(v.Px(), v.Py(), v.Pz(), v.E());
      sjets.push_back(this_pj);
      ijets.push_back(idirty);
      csvs.push_back(0.);
    }
  }else{
    for(size_t idirty = 0; idirty<jets.size(); ++idirty){

      int jet = jets.at(idirty);
      if(is_nan(jets_corr_p4().at(jet).Px()) || is_nan(jets_corr_p4().at(jet).Py())
         || is_nan(jets_corr_p4().at(jet).Pz()) || is_nan(jets_corr_p4().at(jet).E())
         || (clean && to_clean.at(idirty))) continue;
      const TLorentzVector &this_lv = jets_corr_p4().at(jet);
      const PseudoJet this_pj(this_lv.Px(), this_lv.Py(), this_lv.Pz(), this_lv.E());

      sjets.push_back(this_pj);
      ijets.push_back(idirty);
      csvs.push_back(jets_btag_inc_secVertexCombined()->at(jet));
    }
  }

  JetDefinition jet_def(antikt_algorithm, radius);
  ClusterSequence cs(sjets, jet_def);
  vector<PseudoJet> fjets = sorted_by_m(cs.inclusive_jets());
  nfjets = 0;
  mj = 0.;
  fjets_pt.resize(fjets.size());
  fjets_eta.resize(fjets.size());
  fjets_phi.resize(fjets.size());
  fjets_m.resize(fjets.size());
  fjets_nconst.resize(fjets.size());
  fjets_sumcsv.resize(fjets.size());
  fjets_poscsv.resize(fjets.size());
  fjets_btags.resize(fjets.size());

  for(size_t ipj = 0; ipj < fjets.size(); ++ipj){
    const PseudoJet &pj = fjets.at(ipj);
    fjets_pt.at(ipj) = pj.pt();
    fjets_eta.at(ipj) = pj.eta();
    fjets_phi.at(ipj) = pj.phi_std();
    fjets_m.at(ipj) = pj.m();
    const vector<PseudoJet> &cjets = pj.constituents();
    fjets_nconst.at(ipj) = cjets.size();
    mj += pj.m();
    ++nfjets;
    fjets_btags.at(ipj) = 0;
    fjets_sumcsv.at(ipj) = 0.;
    fjets_poscsv.at(ipj) = 0.;
    for(size_t ijet = 0; ijet < ijets.size(); ++ijet){
      size_t i = ijets.at(ijet);
      for(size_t cjet = 0; cjet < cjets.size(); ++ cjet){
        if((cjets.at(cjet) - sjets.at(ijet)).pt() < 0.0001){
          jets_fjet_index.at(i) = ipj;
          fjets_sumcsv.at(ipj) += csvs.at(ijet);
          if(csvs.at(ijet) > 0.){
            fjets_poscsv.at(ipj) += csvs.at(ijet);
          }
          if(csvs.at(ijet) > CSVCuts[1]){
            ++(fjets_btags.at(ipj));
          }
        }
      }
    }
  }
}

void event_handler_quick::SetMiniIso(small_tree_quick &tree, int ilep, int ParticleType){
  // double bignum = std::numeric_limits<double>::max();
  switch(ParticleType){
  case 11:
    tree.els_miniso().push_back(els_miniso()->at(ilep));
    break;
  case 13:
    tree.mus_miniso().push_back(mus_miniso()->at(ilep));
    break;
  default:
    break;
  }
}

float event_handler_quick::GetMinMTWb(const vector<int> &good_jets,
                                      const double pt_cut,
                                      const double bTag_req,
                                      const bool use_W_mass) const{
  float min_mT(fltmax);
  for (uint ijet(0); ijet<good_jets.size(); ijet++) {
    uint jet = good_jets[ijet];
    if (jets_corr_p4().at(jet).Pt()<pt_cut) continue;
    if (jets_btag_inc_secVertexCombined()->at(jet)<bTag_req) continue;
    float mT = GetMT(use_W_mass ? 80.385 : 0., met_corr(), met_phi_corr(),
                     0., jets_corr_p4().at(jet).Pt(), jets_corr_p4().at(jet).Phi());
    if (mT<min_mT) min_mT=mT;
  }
  if (min_mT==fltmax) return bad_val;
  else return min_mT;
}

unsigned event_handler_quick::TypeCode(const vector<mc_particle> &parts,
                                       const vector<size_t> &moms){
  const string sample_name = SampleName();
  unsigned sample_code = 0xF;
  if(Contains(sample_name, "SMS")){
    sample_code = 0x0;
  }else if(Contains(sample_name, "TTJets")
           || Contains(sample_name, "TT_")){
    sample_code = 0x1;
  }else if(Contains(sample_name, "WJets")){
    sample_code = 0x2;
  }else if(Contains(sample_name, "T_s-channel")
           || Contains(sample_name, "T_tW-channel")
           || Contains(sample_name, "T_t-channel")
           || Contains(sample_name, "Tbar_s-channel")
           || Contains(sample_name, "Tbar_tW-channel")
           || Contains(sample_name, "Tbar_t-channel")){
    sample_code = 0x3;
  }else if(Contains(sample_name, "QCD")){
    sample_code = 0x4;
  }else if(Contains(sample_name, "DY")){
    sample_code = 0x5;
  }else{
    sample_code = 0xF;
  }
  unsigned nlep, ntau, ntaul;
  CountLeptons(parts, moms, nlep, ntau, ntaul);
  if(nlep > 0xF) nlep = 0xF;
  if(ntau > 0xF) ntau = 0xF;
  if(ntaul > 0xF) ntaul = 0xF;
  return (sample_code << 12) | (nlep << 8) | (ntaul << 4) | ntau;
}

// MC lepton counting without building the whole tree
void event_handler_quick::GetTrueLeptons(vector<int> &true_electrons, vector<int> &true_muons,
				   vector<int> &true_had_taus, vector<int> &true_lep_taus) {
  true_electrons.clear();
  true_muons.clear();
  true_had_taus.clear();
  true_lep_taus.clear();
  bool tau_to_3tau(false);
  vector<int> lep_from_tau;
  for(unsigned i = 0; i < mc_doc_id()->size(); ++i){
    const int id = static_cast<int>(floor(fabs(mc_doc_id()->at(i))+0.5));
    const int mom = static_cast<int>(floor(fabs(mc_doc_mother_id()->at(i))+0.5));
    const int gmom = static_cast<int>(floor(fabs(mc_doc_grandmother_id()->at(i))+0.5));
    const int ggmom = static_cast<int>(floor(fabs(mc_doc_ggrandmother_id()->at(i))+0.5));
  
    
    if((id == 11 || id == 13) && (mom == 24 || (mom == 15 && (gmom == 24 || (gmom == 15 && ggmom == 24))))){
      if (mom == 24) { // Lep from W
	if (id==11) true_electrons.push_back(i);
	else if (id==13) true_muons.push_back(i);
      } else if(!tau_to_3tau) { // Lep from tau, check for Brem
	uint nlep(1);
	for(uint j=i+1; j<mc_doc_id()->size(); ++j) {
	  const int idb = static_cast<int>(floor(fabs(mc_doc_id()->at(j))+0.5));
	  const int momb = static_cast<int>(floor(fabs(mc_doc_mother_id()->at(j))+0.5));
	  if(momb==15 && (idb==11 || idb==13)) nlep++;
	  if(momb!=15 || (momb==15&&idb==16) || j==mc_doc_id()->size()-1){
	    if(nlep==1){
	      // if (id==11) true_electrons.push_back(i); // If we want to count isolated leptons
	      // else if (id==13) true_muons.push_back(i);
	      lep_from_tau.push_back(i);
	    }
	    i = j-1; // Moving index to first particle after tau daughters
	    break;
	  }
	} // Loop over tau daughters
      } // if lepton comes from tau
   } 
    if(id == 15 && mom == 24){
      true_had_taus.push_back(i);
    }
    // Counting number of tau->tautautau
    if((id == 15) && (mom == 15 && (gmom == 24 || (gmom == 15 && ggmom == 24)))){
      uint nlep(1);
      for(uint j=i+1; j<mc_doc_id()->size(); ++j) {
	const int idb = static_cast<int>(floor(fabs(mc_doc_id()->at(j))+0.5));
	const int momb = static_cast<int>(floor(fabs(mc_doc_mother_id()->at(j))+0.5));
	if(momb==15 && idb==15) nlep++;
	if(momb!=15 || (momb==15&&idb==16) || j==mc_doc_id()->size()-1){
	  if(nlep>1) tau_to_3tau = true;
	  i = j-1; // Moving index to first particle after tau daughters
	  break;
	}
      } // Loop over tau daughters
    } // if tau comes from prompt tau
  } // Loop over mc_doc
  // Removing leptonic taus from tau list by finding smallest DeltaR(lep,tau)
  for(unsigned ind = 0; ind < lep_from_tau.size(); ++ind){
    float minDr(9999.), lepEta(mc_doc_eta()->at(lep_from_tau[ind])), lepPhi(mc_doc_phi()->at(lep_from_tau[ind]));
    int imintau(-1);
    for(unsigned itau=0; itau < true_had_taus.size(); itau++){
      if(mc_doc_mother_id()->at(lep_from_tau[ind]) != mc_doc_id()->at(true_had_taus[itau])) continue;
      float tauEta(mc_doc_eta()->at(true_had_taus[itau])), tauPhi(mc_doc_phi()->at(true_had_taus[itau]));
      float tauDr(dR(tauEta,lepEta, tauPhi,lepPhi));
      if(tauDr < minDr) {
	minDr = tauDr;
	imintau = itau;
      }
    }
    if(imintau>=0) {
      true_lep_taus.push_back(imintau);
      true_had_taus.erase(true_had_taus.begin()+imintau);
    } else cout<<"Not found a tau match for lepton "<<ind<<endl; // Should not happen
  } // Loop over leptons from taus
  return;
}

bool event_handler_quick::greater_m(const fastjet::PseudoJet &a, const fastjet::PseudoJet &b){
  return a.m() > b.m();
}

vector<fastjet::PseudoJet> event_handler_quick::sorted_by_m(vector<fastjet::PseudoJet> pjs){
  sort(pjs.begin(), pjs.end(), greater_m);
  return pjs;
}

void event_handler_quick::Setllmass(small_tree_quick &tree, size_t id1, size_t id2, int pdgid, bool isSig){
  typedef float& (small_tree::*sm_float)() ;
  sm_float ll_m, ll_pt1, ll_pt2, ll_zpt;
  if(pdgid==11){
    if(els_charge()->at(id1)*els_charge()->at(id2) > 0) return; // Only using opposite sign leptons
    if(els_pt()->at(id2) <= MinVetoLeptonPt) return; // Momentum of id1 already checked
    if(els_miniso()->at(id1)>=0.1 || els_miniso()->at(id2)>=0.1) return; // Use only isolated leptons
    if(isSig){
      ll_m   = &small_tree::elel_m;
      ll_pt1 = &small_tree::elel_pt1;
      ll_pt2 = &small_tree::elel_pt2;
      ll_zpt = &small_tree::elel_zpt;
    } else {
      ll_m   = &small_tree::elelv_m;
      ll_pt1 = &small_tree::elelv_pt1;
      ll_pt2 = &small_tree::elelv_pt2;
      ll_zpt = &small_tree::elelv_zpt;
   }
  } else if(pdgid==13){
    if(mus_charge()->at(id1)*mus_charge()->at(id2) > 0) return; // Only using opposite sign leptons
    if(mus_pt()->at(id2) <= MinVetoLeptonPt) return;
    if(mus_miniso()->at(id1)>=0.2 || mus_miniso()->at(id2)>=0.2) return; // Use only isolated leptons
    if(isSig){
      ll_m   = &small_tree::mumu_m;
      ll_pt1 = &small_tree::mumu_pt1;
      ll_pt2 = &small_tree::mumu_pt2;
      ll_zpt = &small_tree::mumu_zpt;
    } else {
      ll_m   = &small_tree::mumuv_m;
      ll_pt1 = &small_tree::mumuv_pt1;
      ll_pt2 = &small_tree::mumuv_pt2;
      ll_zpt = &small_tree::mumuv_zpt;
   }
  } else {
    cout<<"PDG ID "<<pdgid<<" not supported in Setllmass, you silly."<<endl;
    return;
  }
  if((tree.*ll_m)() > 0) return; // We only set it with the first good ll combination

  float px1(-1.), py1(-1.), pz1(-1.), energy1(-1.), px2(-1.), py2(-1.), pz2(-1.), energy2(-1.);
  if(pdgid==11){
    px1 = els_px()->at(id1); py1 = els_py()->at(id1); pz1 = els_pz()->at(id1); energy1 = els_energy()->at(id1);
    px2 = els_px()->at(id2); py2 = els_py()->at(id2); pz2 = els_pz()->at(id2); energy2 = els_energy()->at(id2);
  }
  if(pdgid==13){
    px1 = mus_px()->at(id1); py1 = mus_py()->at(id1); pz1 = mus_pz()->at(id1); energy1 = mus_energy()->at(id1);
    px2 = mus_px()->at(id2); py2 = mus_py()->at(id2); pz2 = mus_pz()->at(id2); energy2 = mus_energy()->at(id2);
  }

  TLorentzVector lep1(px1, py1, pz1, energy1), lep2(px2, py2, pz2, energy2);
  lep2 += lep1;
  (tree.*ll_m)() = lep2.M();
  (tree.*ll_zpt)() = lep2.Pt();
  (tree.*ll_pt1)() = max(sqrt(px1*px1+py1*py1), sqrt(px2*px2+py2*py2));
  (tree.*ll_pt2)() = min(sqrt(px1*px1+py1*py1), sqrt(px2*px2+py2*py2));
  
}
