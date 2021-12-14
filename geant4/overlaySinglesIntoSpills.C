#include "TG4Event.h"

void overlaySinglesIntoSpills() {

  // get input file 
  std::string inFileName = "/pnfs/dune/tape_backed/dunepro/neardet/simulated/2021/mc/physics/dune_nd_miniproduction_2021_v1/00/00/00/00/neutrino.0_1635125340.edep.root";
  //TFile* inFile = new TFile(inFileName.c_str(),"READ");
  TChain* edep_evts = new TChain("EDepSimEvents");
  edep_evts->Add(inFileName.c_str());

  // make output file
  std::string outFileName = "spillFile.root"; 
  TFile* outFile = new TFile(outFileName.c_str(),"RECREATE");
  TTree* new_tree = edep_evts->CloneTree(0);

  // holds lbnf beam timing structure
  TFile* beam_t_file = new TFile("/dune/app/users/kwood/dune-nd/software/ana_edepsim_ndlar/output/lbnf_beam_microstructure.root","READ");
  TH1D* beam_t = (TH1D*) beam_t_file->Get("h_lbnf_spillT");

  unsigned int N_evts = edep_evts->GetEntries();
  double spill_pot, chain_pot;
  spill_pot = 7.5E13;
  chain_pot = 1.0E16;
  double evts_per_spill = ((double)N_evts)/(chain_pot/spill_pot);

  std::cout << "File: " << inFileName << std::endl;  
  std::cout << "    Number of spills: "<< chain_pot/spill_pot << std::endl;  
  std::cout << "    Events per spill: "<< evts_per_spill << std::endl;  

  TG4Event* edep_evt = NULL;
  edep_evts->SetBranchAddress("Event",&edep_evt);

  unsigned int N_left = N_evts;

  // Poisson fluctuation of average number of events per spill in this file
  int Nevts_this_spill = gRandom->Poisson(evts_per_spill);
  int spillN=0;
  int evt_it = 0;

  //for(unsigned int evt_it = 0; evt_it<N_evts; evt_it++) {
  while(N_left > Nevts_this_spill) {

    spillN++;
    std::cout << "working on spill # " << spillN << std::endl;

    // loop over the correct number of events for this spill
    int evt_spill_counter=0;
    while(evt_spill_counter < Nevts_this_spill) {
      edep_evts->GetEntry(evt_it);
      // change the eventID to correspond to the spill ID
      // this ID will not be unique for each entry in the tree
      edep_evt->EventId = spillN;

      // assign time offsets to...
      double event_time = beam_t->GetRandom();

      // ... interaction vertex
      for (std::vector<TG4PrimaryVertex>::iterator v = edep_evt->Primaries.begin(); v != edep_evt->Primaries.end(); ++v) {
        //v->Position.T() = event_time;
        v->Position.SetT(event_time);
      }

      // ... trajectories
      for (std::vector<TG4Trajectory>::iterator t = edep_evt->Trajectories.begin(); t != edep_evt->Trajectories.end(); ++t) {
        // loop over all points in the trajectory  
        for (std::vector<TG4TrajectoryPoint>::iterator p = t->Points.begin(); p != t->Points.end(); ++p) {
          p->Position.SetT(p->Position.T() + event_time);
        }
      }

      // ... and, finally, energy depositions
      for (auto d = edep_evt->SegmentDetectors.begin(); d != edep_evt->SegmentDetectors.end(); ++d) {
        for (std::vector<TG4HitSegment>::iterator h = d->second.begin(); h != d->second.end(); ++h) {
          h->Start.SetT(h->Start.T() + event_time);
          h->Stop.SetT(h->Stop.T() + event_time);
        }
      }

      new_tree->Fill();
      evt_spill_counter++;
      evt_it++;
      N_left--;
    }

    // move counters appropriately for next spill
    Nevts_this_spill = gRandom->Poisson(evts_per_spill); 

  } // spill loop

  outFile->cd();
  new_tree->Write();
  outFile->Close();

}
