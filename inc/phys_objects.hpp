// phys_objects: Class with the standard physics objects that inherits from the cfa class. 
//              Reduced tree makers should inherit from this class

#ifndef H_PHYS_OBJECTS
#define H_PHYS_OBJECTS

#include <iostream>
#include <vector>
#include <string>

#include "cfa.hpp"
#include "pdtlund.hpp"


namespace particleId {
  enum leptonType {
    X=0,
    electron=2,
    muon=3,
    electronVeto=4,
    muonVeto=5
  };
}

class phys_objects : public cfa{
public:
  explicit phys_objects(const std::string &filename, bool is_8TeV=false);

  enum CutLevel{kVeto, kLoose, kMedium, kTight};

  // Muons
  std::vector<int> GetMuons(bool doSignal = true) const;

  bool IsSignalMuon(unsigned imu) const;
  bool IsVetoMuon(unsigned imu) const;
  bool IsSignalIdMuon(unsigned iel) const;
  bool IsVetoIdMuon(unsigned iel) const;

  bool IsIdMuon(unsigned imu, CutLevel threshold) const;

  float GetMuonIsolation(unsigned imu) const;

  // Electrons
  std::vector<int> GetElectrons(bool doSignal = true) const;

  bool IsSignalElectron(unsigned iel) const;
  bool IsVetoElectron(unsigned iel) const;
  bool IsSignalIdElectron(unsigned iel) const;
  bool IsVetoIdElectron(unsigned iel) const;

  bool IsIdElectron(unsigned iel, CutLevel threshold) const;

  float GetElectronIsolation(unsigned iel) const;
  float GetEffectiveArea(float SCEta, bool isMC) const;

  // Leptons
  static int GetMom(float id, float mom, float gmom, float ggmom,
		    bool &fromW);

  // Tracks
  bool IsGoodIsoTrack(unsigned itrk) const;

  // Jets
  std::vector<int> GetJets(const std::vector<int> &SigEl, const std::vector<int> &SigMu, 
                           const std::vector<int> &VetoEl, const std::vector<int> &VetoMu,
			   double pt_thresh, double eta_thresh) const;
  bool IsGoodJet(unsigned ijet, double ptThresh, double etaThresh) const;
  bool IsBasicJet(unsigned ijet) const;

  // Truth matching
  int GetTrueElectron(int index, int &momID, bool &fromW, double &closest_dR) const;
  int GetTrueMuon(int index, int &momID, bool &fromW, double &closest_dR) const;
  int GetTrueParticle(double RecEta, double RecPhi, double &closest_dR) const;

  // Event cleaning
  bool PassesMETCleaningCut() const;
  bool PassesPVCut() const;
  double getDZ(double vx, double vy, double vz, double px, double py, double pz, int firstGoodVertex) const;

  // Event variables
  double getDeltaPhiMETN(unsigned goodJetI, float otherpt, float othereta, bool useArcsin) const;
  double getDeltaPhiMETN_deltaT(unsigned goodJetI, float otherpt, float othereta) const;
  double getMinDeltaPhiMETN(unsigned maxjets, float mainpt, float maineta,
                            float otherpt, float othereta, bool useArcsin) const;

  // Utilities
  bool IsMC() const;
  bool hasPFMatch(int index, particleId::leptonType type, int &pfIdx) const;
};

#endif
