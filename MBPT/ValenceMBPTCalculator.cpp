#include "Include.h"
#include "ValenceMBPTCalculator.h"
#include "Universal/MathConstant.h"
#include "Universal/PhysicalConstant.h"

ValenceMBPTCalculator::ValenceMBPTCalculator(pOrbitalManagerConst orbitals, pHFIntegrals one_body, pSlaterIntegrals two_body, const std::string& fermi_orbitals):
    MBPTCalculator(orbitals, fermi_orbitals), one_body(one_body), two_body(two_body), excited(orbitals->excited), high(orbitals->high)
{}

ValenceMBPTCalculator::~ValenceMBPTCalculator()
{
    one_body->clear();
    two_body->clear();
}

unsigned int ValenceMBPTCalculator::GetStorageSize()
{
    unsigned int total = two_body->CalculateTwoElectronIntegrals(valence, valence, excited, high, true);
    total += one_body->CalculateOneElectronIntegrals(valence, high, true);

    return total;
}

void ValenceMBPTCalculator::UpdateIntegrals()
{
    SetValenceEnergies();

    one_body->CalculateOneElectronIntegrals(valence, high);
    two_body->CalculateTwoElectronIntegrals(valence, valence, excited, high);
}

double ValenceMBPTCalculator::GetOneElectronSubtraction(const OrbitalInfo& s1, const OrbitalInfo& s2)
{
    if(s1.Kappa() != s2.Kappa())
        return 0.;

    return CalculateOneElectronSub(s1, s2);
}

double ValenceMBPTCalculator::GetTwoElectronValence(unsigned int k, const OrbitalInfo& s1, const OrbitalInfo& s2, const OrbitalInfo& s3, const OrbitalInfo& s4)
{
    return CalculateTwoElectronValence(k, s1, s2, s3, s4);
}

double ValenceMBPTCalculator::GetTwoElectronSubtraction(unsigned int k, const OrbitalInfo& s1, const OrbitalInfo& s2, const OrbitalInfo& s3, const OrbitalInfo& s4)
{
    return CalculateTwoElectronSub(k, s1, s2, s3, s4);
}

double ValenceMBPTCalculator::GetTwoElectronBoxValence(unsigned int k, const OrbitalInfo& s1, const OrbitalInfo& s2, const OrbitalInfo& s3, const OrbitalInfo& s4)
{
    return CalculateTwoElectronValence(k, s1, s2, s3, s4);
}

double ValenceMBPTCalculator::CalculateOneElectronSub(const OrbitalInfo& sa, const OrbitalInfo& sb) const
{
    const bool debug = DebugOptions.LogMBPT();
    if(debug)
        *logstream << "Val 1.1:    ";

    double energy = 0.;
    const double ValenceEnergy = ValenceEnergies.find(sa.Kappa())->second;

    auto it_alpha = high->begin();
    while(it_alpha != high->end())
    {
        const OrbitalInfo& salpha = it_alpha->first;
        const double Ealpha = it_alpha->second->Energy();

        // InQSpace() usually will be true for states in "high" but test just in case this is overridden
        if(InQSpace(salpha) && (sa.Kappa() == salpha.Kappa()))
        {
            double term = one_body->GetMatrixElement(sa, salpha) * one_body->GetMatrixElement(salpha, sb);
            energy += term/(ValenceEnergy - Ealpha + delta);
        }

        it_alpha++;
    }

    if(debug)
        *logstream << "  " << std::setprecision(6) << energy * MathConstant::Instance()->HartreeEnergyInInvCm() << std::endl;
    return energy;
}

double ValenceMBPTCalculator::CalculateTwoElectronValence(unsigned int k, const OrbitalInfo& sa, const OrbitalInfo& sb, const OrbitalInfo& sc, const OrbitalInfo& sd) const
{
    const bool debug = DebugOptions.LogMBPT();
    if(debug)
        *logstream << "Val 2.1:   ";

    MathConstant* constants = MathConstant::Instance();

    double energy = 0.;
    const double coeff_ac = constants->Electron3j(sa.TwoJ(), sc.TwoJ(), k);
    const double coeff_bd = constants->Electron3j(sb.TwoJ(), sd.TwoJ(), k);
    if(!coeff_ac || !coeff_bd)
        return energy;

    const double ValenceEnergy = ValenceEnergies.find(sa.Kappa())->second + ValenceEnergies.find(sb.Kappa())->second;
    int k1, k1max;
    int k2, k2max;

    auto it_alpha = excited->begin();
    while(it_alpha != excited->end())
    {
        const OrbitalInfo& salpha = it_alpha->first;
        const double Ealpha = it_alpha->second->Energy();

        auto it_beta = excited->begin();
        while(it_beta != excited->end())
        {
            const OrbitalInfo& sbeta = it_beta->first;
            const double Ebeta = it_beta->second->Energy();

            if(InQSpace(salpha, sbeta) &&
               (sa.L() + salpha.L())%2 == (sb.L() + sbeta.L())%2 &&
               (salpha.L() + sc.L())%2 == (sbeta.L() + sd.L())%2)
            {
                double coeff_alphabeta = salpha.MaxNumElectrons() * sbeta.MaxNumElectrons() * (2 * k + 1);

                coeff_alphabeta = coeff_alphabeta/(coeff_ac * coeff_bd);
                coeff_alphabeta = coeff_alphabeta/(ValenceEnergy - Ealpha - Ebeta + delta);

                int exponent = (sa.TwoJ() + sb.TwoJ() + sc.TwoJ() + sd.TwoJ() + salpha.TwoJ() + sbeta.TwoJ())/2;
                coeff_alphabeta *= constants->minus_one_to_the_power(exponent + k + 1);

                k1 = kmin(sa, salpha);
                k1max = kmax(sa, salpha);

                while(k1 <= k1max)
                {
                    double coeff_ab = constants->Electron3j(sa.TwoJ(), salpha.TwoJ(), k1) *
                                      constants->Electron3j(sb.TwoJ(), sbeta.TwoJ(), k1);

                    if(coeff_ab)
                    {
                        double R1 = two_body->GetTwoElectronIntegral(k1, sa, sb, salpha, sbeta);

                        k2 = kmin(salpha, sc);
                        k2max = kmax(salpha, sc);

                        while(k2 <= k2max)
                        {
                            double coeff = constants->Electron3j(salpha.TwoJ(), sc.TwoJ(), k2) *
                                           constants->Electron3j(sbeta.TwoJ(), sd.TwoJ(), k2);

                            if(coeff)
                                coeff = coeff * constants->Wigner6j(sc.J(), sa.J(), k, k1, k2, salpha.J())
                                              * constants->Wigner6j(sd.J(), sb.J(), k, k1, k2, sbeta.J());
                            if(coeff)
                            {
                                coeff = coeff * coeff_ab * coeff_alphabeta;
                                if((k1 + k2)%2)
                                    coeff = -coeff;

                                double R2 = two_body->GetTwoElectronIntegral(k2, salpha, sbeta, sc, sd);

                                energy += R1 * R2 * coeff;
                            }

                            k2 += 2;
                        }
                    }

                    k1 += 2;
                }
            }
            it_beta++;
        }
        it_alpha++;
    }

    if(debug)
        *logstream << "  " << std::setprecision(6) << energy * constants->HartreeEnergyInInvCm() << std::endl;
    return energy;
}

double ValenceMBPTCalculator::CalculateTwoElectronSub(unsigned int k, const OrbitalInfo& sa, const OrbitalInfo& sb, const OrbitalInfo& sc, const OrbitalInfo& sd) const
{
    const bool debug = DebugOptions.LogMBPT();
    if(debug)
        *logstream << "Val 2.2:   ";

    double energy = 0.;

    const double Ea = ValenceEnergies.find(sa.Kappa())->second;
    const double Eb = ValenceEnergies.find(sb.Kappa())->second;
    const double Ec = ValenceEnergies.find(sc.Kappa())->second;
    const double Ed = ValenceEnergies.find(sd.Kappa())->second;


    auto it_alpha = high->begin();
    while(it_alpha != high->end())
    {
        const OrbitalInfo& salpha = it_alpha->first;
        const double Ealpha = it_alpha->second->Energy();

        // InQSpace() usually will be true for states in "high" but test just in case this is overridden
        if(InQSpace(salpha))
        {
            if(sa.Kappa() == salpha.Kappa())
            {
                double R1 = two_body->GetTwoElectronIntegral(k, salpha, sb, sc, sd);
                energy += R1 * one_body->GetMatrixElement(sa, salpha)/(Ea - Ealpha + delta);
            }

            if(sc.Kappa() == salpha.Kappa())
            {
                double R1 = two_body->GetTwoElectronIntegral(k, sa, sb, salpha, sd);
                energy += R1 * one_body->GetMatrixElement(salpha, sc)/(Ec - Ealpha + delta);
            }

            if(sb.Kappa() == salpha.Kappa())
            {
                double R1 = two_body->GetTwoElectronIntegral(k, sa, salpha, sc, sd);
                energy += R1 * one_body->GetMatrixElement(sb, salpha)/(Eb - Ealpha + delta);
            }

            if(sd.Kappa() == salpha.Kappa())
            {
                double R1 = two_body->GetTwoElectronIntegral(k, sa, sb, sc, salpha);
                energy += R1 * one_body->GetMatrixElement(salpha, sd)/(Ed - Ealpha + delta);
            }
        }
        it_alpha++;
    }

    if(debug)
        *logstream << "  " << std::setprecision(6) << energy * MathConstant::Instance()->HartreeEnergyInInvCm() << std::endl;
    return energy;
}