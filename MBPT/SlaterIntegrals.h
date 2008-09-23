#ifndef SLATER_INTEGRALS_H
#define SLATER_INTEGRALS_H

#include "Basis/ExcitedStates.h"

#define LongKey unsigned long long int

class SlaterIntegrals
{
    /** Class to hold Slater integrals. */
public:
    SlaterIntegrals(const ExcitedStates* excited_states):
        core(*excited_states->GetCore()), excited(*excited_states), include_valence_sms(false)
    {}
    virtual ~SlaterIntegrals() {}

    /** Calculate number of one-electron and two-electron integrals that will be stored.
        Return total.
     */
    virtual unsigned int GetStorageSize(const ExcitedStates& valence) = 0;

    /** Update all integrals (on the assumption that the excited states have changed). */
    virtual void Update(const ExcitedStates& valence);

    /** Clear all integrals. */
    virtual void Clear();

    /** Include the scaled specific mass shift in the two electron integrals. */
    inline void IncludeValenceSMS(bool include)
    {   include_valence_sms = include;
    }

    /** GetOneElectronIntegral(i, j) = <i|H|j> */
    double GetOneElectronIntegral(const StateInfo& s1, const StateInfo& s2) const;

    /** GetSMSIntegral(i, j) = <i|p|j> */
    double GetSMSIntegral(const StateInfo& s1, const StateInfo& s2) const;
    
    /** GetTwoElectronIntegral(k, i, j, l, m) = R_k(ij, lm): i->l, j->m */
    virtual double GetTwoElectronIntegral(unsigned int k, const StateInfo& s1, const StateInfo& s2, const StateInfo& s3, const StateInfo& s4) const;

    inline double GetNuclearInverseMass() const
    {   return core.GetNuclearInverseMass();
    }

protected:
    /** Change ordering of states so that it corresponds to a stored integral.
        Returns false if SMS sign needs to be changed.
     */
    virtual bool TwoElectronIntegralOrdering(unsigned int& i1, unsigned int& i2, unsigned int& i3, unsigned int& i4) const;

    virtual void UpdateStateIndexes(const ExcitedStates& valence_states);

    virtual void UpdateOneElectronIntegrals() = 0;
    virtual void UpdateTwoElectronIntegrals() = 0;

    void swap(unsigned int& i1, unsigned int& i2) const;

protected:
    const Core& core;
    const ExcitedStates& excited;

    LongKey NumStates;

    // The ordering of states is not arbitrary:
    // - core states should have lower indices
    // - excited states should be ordered by pqn first.
    std::map<StateInfo, unsigned int> state_index;
    std::map<unsigned int, StateInfo> reverse_state_index;

    // Sets of state indices indicate what type of orbital.
    //   core = closed shell core states
    //   valence is a subset of excited states that are external lines in MBPT diagrams
    //      (and then are generally included in CI calculations)
    std::set<unsigned int> core_states;
    std::set<unsigned int> valence_states;
    std::set<unsigned int> excited_states;

    // Storage for one and two electron integrals.

    // OneElectronIntegrals(i, j) = <i|H|j>
    std::map<unsigned int, double> OneElectronIntegrals;

    // TwoElectronIntegrals(k, i, j, l, m) = R_k(ij, lm): i->l, j->m
    std::map<LongKey, double> TwoElectronIntegrals;

    // SMSIntegrals(i, j) = <i|p|j>
    std::map<unsigned int, double> SMSIntegrals;

    // Include SMS in two-body integrals.
    bool include_valence_sms;
};

inline void SlaterIntegrals::swap(unsigned int& i1, unsigned int& i2) const
{   unsigned int temp = i1;
    i1 = i2;
    i2 = temp;
}

#endif