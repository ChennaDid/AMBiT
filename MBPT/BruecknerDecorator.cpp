#include "BruecknerDecorator.h"
#include "Universal/Interpolator.h"

BruecknerDecorator::BruecknerDecorator(pHFOperator wrapped_hf, pOPIntegrator integration_strategy):
    HFOperatorDecorator(wrapped_hf, integration_strategy), lambda(1.)
{}

void BruecknerDecorator::CalculateSigma(int kappa, pBruecknerSigmaCalculator brueckner_calculator)
{
    if(sigmas.find(kappa) == sigmas.end())
    {
        pSigmaPotential sigma(new SigmaPotential(lattice->size(), 100));
        sigma->IncludeLower(use_fg, use_gg);
        brueckner_calculator->GetSecondOrderSigma(kappa, *sigma);

        sigmas[kappa] = sigma;
    }
}

void BruecknerDecorator::CalculateSigma(int kappa, pOrbitalManagerConst orbitals, pHartreeY hartreeY, pSpinorOperatorConst bare_hf)
{
    if(sigmas.find(kappa) == sigmas.end())
    {
        pBruecknerSigmaCalculator calculator;
        if(bare_hf)
            calculator.reset(new BruecknerSigmaCalculator(orbitals, bare_hf, hartreeY));
        else
            calculator.reset(new BruecknerSigmaCalculator(orbitals, wrapped, hartreeY));

        CalculateSigma(kappa, calculator);
    }
}

/** Attempt to read sigma with given kappa, filename is "identifier.[kappa].sigma". */
void BruecknerDecorator::Read(const std::string& identifier, int kappa)
{
    std::string filename = identifier + "." + itoa(kappa) + ".sigma";

    pSigmaPotential sigma(new SigmaPotential());
    if(sigma->Read(filename))
    {
        sigmas[kappa] = sigma;
    }
}

/** Write all sigmas; filenames "identifier.[kappa].sigma". */
void BruecknerDecorator::Write(const std::string& identifier, int kappa) const
{
    auto it = sigmas.find(kappa);
    if(it != sigmas.end())
    {
        std::string filename = identifier + "." + itoa(kappa) + ".sigma";
        it->second->Write(filename);
    }
}

/** Write all sigmas; filenames "identifier.[kappa].sigma". */
void BruecknerDecorator::Write(const std::string& identifier) const
{
    for(auto& pair: sigmas)
    {
        std::string filename = identifier + "." + itoa(pair.first) + ".sigma";
        pair.second->Write(filename);
    }
}

void BruecknerDecorator::Alert()
{
    // Don't even try to change the size of the Sigma matrices.
    if(currentExchangePotential.size() > lattice->size())
        currentExchangePotential.resize(lattice->size());
}

/** Set exchange (nonlocal) potential and energy for ODE routines. */
void BruecknerDecorator::SetODEParameters(const SingleParticleWavefunction& approximation)
{
    HFOperatorDecorator::SetODEParameters(approximation);
    currentExchangePotential = CalculateExtraNonlocal(approximation, true);
}

/** Get exchange (nonlocal) potential. */
SpinorFunction BruecknerDecorator::GetExchange(pSingleParticleWavefunctionConst approximation) const
{
    SpinorFunction ret = wrapped->GetExchange(approximation);

    if(approximation == NULL)
        ret += currentExchangePotential;
    else
        ret += CalculateExtraNonlocal(*approximation, true);

    return ret;
}

void BruecknerDecorator::GetODEFunction(unsigned int latticepoint, const SpinorFunction& fg, double* w) const
{
    wrapped->GetODEFunction(latticepoint, fg, w);

    if(include_nonlocal && latticepoint < currentExchangePotential.size())
    {   double alpha = physicalConstant->GetAlpha();
        w[0] += alpha * currentExchangePotential.g[latticepoint];
        w[1] -= alpha * currentExchangePotential.f[latticepoint];
    }
}

void BruecknerDecorator::GetODECoefficients(unsigned int latticepoint, const SpinorFunction& fg, double* w_f, double* w_g, double* w_const) const
{
    wrapped->GetODECoefficients(latticepoint, fg, w_f, w_g, w_const);

    if(include_nonlocal && latticepoint < currentExchangePotential.size())
    {   double alpha = physicalConstant->GetAlpha();
        w_const[0] += alpha * currentExchangePotential.g[latticepoint];
        w_const[1] -= alpha * currentExchangePotential.f[latticepoint];
    }
}

void BruecknerDecorator::GetODEJacobian(unsigned int latticepoint, const SpinorFunction& fg, double** jacobian, double* dwdr) const
{
    wrapped->GetODEJacobian(latticepoint, fg, jacobian, dwdr);

    if(include_nonlocal && latticepoint < currentExchangePotential.size())
    {   double alpha = physicalConstant->GetAlpha();
        dwdr[0] += alpha * currentExchangePotential.dgdr[latticepoint];
        dwdr[1] -= alpha * currentExchangePotential.dfdr[latticepoint];
    }
}

SpinorFunction BruecknerDecorator::ApplyTo(const SpinorFunction& a) const
{
    SpinorFunction ta = wrapped->ApplyTo(a);
    ta -= CalculateExtraNonlocal(a, false);

    return ta;
}

SpinorFunction BruecknerDecorator::CalculateExtraNonlocal(const SpinorFunction& s, bool include_derivative) const
{
    SpinorFunction ret(s.Kappa());

    auto it = sigmas.find(s.Kappa());
    if(it != sigmas.end())
    {
        pSigmaPotential sigma = it->second;
        if(s.size() < sigma->size())
        {   SpinorFunction bigger_s(s);
            bigger_s.resize(sigma->size());
            ret = sigma->ApplyTo(bigger_s, lattice);
        }
        else
            ret = sigma->ApplyTo(s, lattice);

        ret *= (-lambda);   // Remember we are storing HF potential -V (that is, V > 0)

        if(include_derivative)
        {
            Interpolator interp(lattice);
            interp.GetDerivative(ret.f, ret.dfdr, 6);
            interp.GetDerivative(ret.g, ret.dgdr, 6);
        }
    }

    return ret;
}