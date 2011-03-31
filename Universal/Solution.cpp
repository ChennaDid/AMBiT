#include "Include.h"
#include "Universal/Enums.h"
#include "Universal/Solution.h"

SolutionID::SolutionID(double aJ, ParityType::Enum aParity, unsigned int aID)
{
    mJ = aJ;
    mParity = aParity;
    mID = aID;
}

bool SolutionID::operator<(const SolutionID& other) const
{
    if(mJ < other.mJ)
    {
        return true;
    }
    else if(mJ == other.mJ)
    {
        if(mParity < other.mParity)
        {
            return true;
        }
        else if(mParity == other.mParity)
        {
            if(mID < other.mID)
            {
                return true;
            }
        }
    }

    return false;
}

Solution::Solution(double aEnergy, double agFactor)
{
    mEnergy = aEnergy;
    mgFactor = agFactor;
}

Solution::Solution(double aEnergy, std::map<Configuration, double> aPercentagesMap, double agFactor)
{
    mEnergy = aEnergy;
    mgFactor = agFactor;
    
    std::map<Configuration, double>::iterator cd_it;
    for(cd_it = aPercentagesMap.begin(); cd_it != aPercentagesMap.end(); cd_it++)
    {
        if(cd_it->second > 1)
        {
            mConfigurationSet.insert(ConfigurationPair(cd_it->first, cd_it->second));
        }
    }
}

void SolutionMap::Print(DisplayOutputType::Enum aDisplayOutputType)
{
    if(aDisplayOutputType == DisplayOutputType::Standard)
    {
        SolutionMap::iterator sm_it;
        SolutionID LastSolutionID(-1, ParityType::Even, 0);
        for(sm_it = begin(); sm_it != end(); sm_it++)
        {
            if(sm_it->first.SameJP(LastSolutionID))
            {
                *outstream << "Solutions for J = " << sm_it->first.GetJ() << ", P = " << ParityType::LowerName(sm_it->first.GetParity()) << ":" << std::endl;
            }
            *outstream << sm_it->first.GetID() << ": " << std::setprecision(8) << sm_it->second.GetEnergyAtomicUnits() << "    " << std::setprecision(12) << sm_it->second.GetEnergyInversecm() << " /cm" << std::endl;
            sm_it->second.GetConfigurationSet()->Print();
            if(sm_it->first.GetJ() != 0)
            {
                *outstream << "    g-factor = " << std::setprecision(5) << sm_it->second.GetgFactor() << std::endl;
            }
            *outstream << std::endl;
            LastSolutionID = sm_it->first;
        }
    }
    else if(aDisplayOutputType == DisplayOutputType::SpaceSeparated)
    {
        *outstream << "J P ID E g" << std::endl;
        SolutionMap::iterator sm_it;
        for(sm_it = begin(); sm_it != end(); sm_it++)
        {
            *outstream << sm_it->first.GetJ() << " " << ParityType::ShortName(sm_it->first.GetParity()) << " " << sm_it->first.GetID() << " " << std::setprecision(12) << sm_it->second.GetEnergyInversecm();
            *outstream << " " << std::setprecision(5) << sm_it->second.GetgFactor();
            // Add error checking for GetLargestConfiguration!
            *outstream << " " << sm_it->second.GetConfigurationSet()->GetLargestConfiguration()->first.ShortName() << " " << std::setprecision(2) << sm_it->second.GetConfigurationSet()->GetLargestConfiguration()->second << "%" << std::endl;
        }
        *outstream << std::endl;
    }
    else if(aDisplayOutputType == DisplayOutputType::CommaSeparated)
    {
        *outstream << "J,P,ID,E,g" << std::endl;
        SolutionMap::iterator sm_it;
        for(sm_it = begin(); sm_it != end(); sm_it++)
        {
            *outstream << sm_it->first.GetJ() << "," << ParityType::ShortName(sm_it->first.GetParity()) << "," << sm_it->first.GetID() << "," << std::setprecision(12) << sm_it->second.GetEnergyInversecm();
            *outstream << "," << std::setprecision(5) << sm_it->second.GetgFactor();
            // Add error checking for GetLargestConfiguration!
            *outstream << "," << sm_it->second.GetConfigurationSet()->GetLargestConfiguration()->first.Name(false) << "," << std::setprecision(2) << sm_it->second.GetConfigurationSet()->GetLargestConfiguration()->second << "%" << std::endl;
        }
        *outstream << std::endl;
    }
    else if(aDisplayOutputType == DisplayOutputType::TabSeparated)
    {
        *outstream << "J\tP\tID\tE\tg" << std::endl;
        SolutionMap::iterator sm_it;
        for(sm_it = begin(); sm_it != end(); sm_it++)
        {
            *outstream << sm_it->first.GetJ() << "\t" << ParityType::ShortName(sm_it->first.GetParity()) << "\t" << sm_it->first.GetID() << "\t" << std::setprecision(12) << sm_it->second.GetEnergyInversecm();
            *outstream << "\t" << std::setprecision(5) << sm_it->second.GetgFactor();
            // Add error checking for GetLargestConfiguration!
            *outstream << "\t" << sm_it->second.GetConfigurationSet()->GetLargestConfiguration()->first.Name(false) << "\t" << std::setprecision(2) << sm_it->second.GetConfigurationSet()->GetLargestConfiguration()->second << "%" << std::endl;
        }
        *outstream << std::endl;
    }
}
