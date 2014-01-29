#include "Include.h"
#include "Orbital.h"
#include "Universal/PhysicalConstant.h"
#include "OrbitalInfo.h"
#include <math.h>

Orbital::Orbital(const OrbitalInfo& info):
    SingleParticleWavefunction(info)
{
    occupancy = 2.*abs(kappa);
}

Orbital::Orbital(int kappa, unsigned int pqn, double energy, unsigned int size):
    SingleParticleWavefunction(kappa, pqn, energy, size)
{
    occupancy = 2.*abs(kappa);
}

Orbital::Orbital(const Orbital& other):
    SingleParticleWavefunction(other), occupancy(other.occupancy)
{}

const Orbital& Orbital::operator=(const Orbital& other)
{
    SingleParticleWavefunction::operator=(other);
    occupancy = other.occupancy;
    return *this;
}

const Orbital& Orbital::operator*=(double scale_factor)
{
    SingleParticleWavefunction::operator*=(scale_factor);
    return *this;
}

Orbital Orbital::operator*(double scale_factor) const
{
    Orbital ret(*this);
    ret *= scale_factor;
    return ret;
}

const Orbital& Orbital::operator+=(const Orbital& other)
{
    SingleParticleWavefunction::operator+=(other);
    return *this;
}

const Orbital& Orbital::operator-=(const Orbital& other)
{
    SingleParticleWavefunction::operator-=(other);
    return *this;
}

Orbital Orbital::operator+(const Orbital& other) const
{
    Orbital ret(*this);
    ret += other;
    return ret;
}

Orbital Orbital::operator-(const Orbital& other) const
{
    Orbital ret(*this);
    ret -= other;
    return ret;
}

const Orbital& Orbital::operator*=(const RadialFunction& chi)
{
    SingleParticleWavefunction::operator*=(chi);
    return *this;
}

Orbital Orbital::operator*(const RadialFunction& chi) const
{
    Orbital ret(*this);
    ret *= chi;
    return ret;
}

double Orbital::Norm(pLatticeConst lattice) const
{
    double norm = 0.;
    unsigned int i;
    const double* dR = lattice->dR();

    norm = f[0]*f[0] * g[0]*g[0] * dR[0];
    for(i=0; i<Size()-2; i+=2)
    {   norm = norm + 4. * (f[i]*f[i] + g[i]*g[i]) * dR[i];
        norm = norm + 2. * (f[i+1]*f[i+1] + g[i+1]*g[i+1]) * dR[i+1];
    }

    norm = norm/3.;

    while(i<Size())
    {   norm = norm + (f[i]*f[i] + g[i]*g[i]) * dR[i];
        i++;
    }

    return norm;
}

std::string Orbital::Name() const
{
    return OrbitalInfo(this).Name();
}

bool Orbital::CheckSize(pLattice lattice, double tolerance)
{
    double maximum = 0.;
    unsigned int i = 0;
    unsigned int max_point = 0;
    while(i < f.size())
    {   if(fabs(f[i]) >= maximum)
        {   maximum = fabs(f[i]);
            max_point = i;
        }
        i++;
    }

    if(maximum < tolerance*100)
    {   *errstream << "Orbital::Checksize: Zero function. " << std::endl;
        PAUSE
        exit(1);
    }

    i = f.size() - 1;
    while(fabs(f[i])/maximum < tolerance)
        i--;

    if(i == f.size() - 1)
    {   // add points to wavefunction
        unsigned int max = f.size();
        double f_max, f_ratio, g_ratio;
        double log_f_ratio, log_g_ratio, dr_max;

        // Strip off any nearby node
        do
        {   max--;
            f_max = fabs(f[max]);
            f_ratio = f[max]/f[max-1];
            g_ratio = g[max]/g[max-1];
        }while(f_ratio < 0. || g_ratio < 0.);

        // Make sure we are tailing off
        if(f_ratio > 0.96)
            f_ratio = 0.96;
        if(g_ratio > 0.96)
            g_ratio = 0.96;

        log_f_ratio = log(f_ratio);
        log_g_ratio = log(g_ratio);
        dr_max = lattice->R(max)-lattice->R(max-1);

        // Resize the state (this is a slight overestimate assuming dr is constant).
        unsigned int old_size = max;
        while(f_max/maximum >= tolerance)
        {   max++;
            f_max = f_max * f_ratio;
        }
        ReSize(max+1);

        // Exponential decay (assumes dr changes slowly).
        unsigned int i = old_size;
        while((i < max) && (fabs(f[i])/maximum > tolerance))
        {
            double d2r = (lattice->R(i+1) - lattice->R(i))/dr_max -1.;

            f[i+1] = f[i] * f_ratio * (1. + log_f_ratio * d2r);
            g[i+1] = g[i] * g_ratio * (1. + log_g_ratio * d2r);

            i++;
        }
        ReSize(i+1);

        return false;
    }
    else if(i+2 < f.size())
    {   // Reduce size
        ReSize(i+2);
        return false;
    }
    else return true;
}

void Orbital::ReNormalise(pLatticeConst lattice, double norm)
{
    double scaling = sqrt(norm/Norm(lattice));
    if(scaling)
        (*this) *= scaling;
}

unsigned int Orbital::NumNodes() const
{
    // Count number of zeros
    unsigned int zeros = 0;

    // Get maximum point. This is generally past the last node.
    // We want to ignore small oscillations at the tail of the wavefunction,
    // these are due to the exchange interaction.
    double fmax = 0.;
    unsigned int i_fmax = 0;
    unsigned int i = 0;
    while(i < f.size())
    {   if(fabs(f[i]) > fmax)
        {   fmax = fabs(f[i]);
            i_fmax = i;
        }
        i++;
    }
    
    // Get effective end point
    unsigned int end = f.size() - 1;
    while(fabs(f[end]) < 1.e-2 * fmax)
        end--;

    // Get effective start point
    i = 0;
    double prev = f[i];
    while(fabs(prev) < 1.e-7 * fmax)
        prev = f[i++];

    while(i < end)
    {   if(f[i] * prev < 0.)
            zeros++;
        prev = f[i];
        i++;
    }
    return zeros;
}

void Orbital::Write(FILE* fp) const
{
    // As well as the SpinorFunction vectors, we need to output some other things
    fwrite(&kappa, sizeof(int), 1, fp);
    fwrite(&pqn, sizeof(unsigned int), 1, fp);
    fwrite(&occupancy, sizeof(double), 1, fp);

    SingleParticleWavefunction::Write(fp);
}

void Orbital::Read(FILE* fp)
{
    fread(&kappa, sizeof(int), 1, fp);
    fread(&pqn, sizeof(unsigned int), 1, fp);
    fread(&occupancy, sizeof(double), 1, fp);

    SingleParticleWavefunction::Read(fp);
}
