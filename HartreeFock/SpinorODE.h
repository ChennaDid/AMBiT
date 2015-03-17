#ifndef SPINOR_ODE_H
#define SPINOR_ODE_H

#include "Universal/SpinorFunction.h"
#include "Orbital.h"
#include "Core.h"

/** SpinorODE is an abstract class for numerical integration of coupled linear
    ordinary differential equations (ODEs) of the form
        df/dr = w[0] = w_f[0] f + w_g[0] g + w_const[0]
        dg/dr = w[1] = w_f[1] f + w_g[1] g + w_const[1]
    where w is a linear function of f and g (e.g. Hartree-Fock).
    w_const is the "nonlocal" part (the exchange part).
    It follows the Decorator (Wrapper) pattern, so it is recursively extensive.
    It is also a LatticeObserver, and is guaranteed to provide correct differential equations over
    the entire lattice.
 */
class SpinorODE : public LatticeObserver
{
public:
    SpinorODE(pLattice lattice);
    virtual ~SpinorODE() {}

    pLattice GetLattice() { return lattice; }

    /** Set exchange (nonlocal) potential and energy for ODE routines. */
    virtual void SetODEParameters(int kappa, double energy, const SpinorFunction* exchange = NULL) = 0;
    
    /** Set exchange (nonlocal) potential and energy for ODE routines. */
    virtual void SetODEParameters(const Orbital& approximation) = 0;

    /** Get exchange (nonlocal) potential. */
    virtual SpinorFunction GetExchange(pOrbitalConst approximation = pOrbitalConst()) const = 0;

    /** Tell SpinorODE whether to include the nonlocal (w_const) terms in GetODEFunction, GetODECoefficients, and GetODEJacobian. */
    virtual void IncludeExchange(bool include_exchange);

    /** Interrogate whether to include nonlocal (w_const) terms in GetODEFunction, GetODECoefficients, and GetODEJacobian. */
    virtual bool IncludeExchange() const;

    /** Get df/dr = w[0] and dg/dr = w[1] given point r, (f, g).
        PRE: w should be an allocated 2 dimensional array;
             latticepoint < size().
     */
    virtual void GetODEFunction(unsigned int latticepoint, const SpinorFunction& fg, double* w) const = 0;

    /** Get numerical coefficients of the ODE at the point r, (f,g).
        w_f and w_g are coefficients of f and g in w; w_const is the constant term of w (not proportional to f or g).
        PRE: w_f, w_g, and w_const should be allocated 2 dimensional arrays;
             latticepoint < size().
     */
    virtual void GetODECoefficients(unsigned int latticepoint, const SpinorFunction& fg, double* w_f, double* w_g, double* w_const) const = 0;
    
    /** Get Jacobian (dw[i]/df and dw[i]/dg), dw[i]/dr at a point r, (f, g).
        PRE: jacobian should be an allocated 2x2 matrix;
             dwdr and w_const should be allocated 2 dimensional arrays;
             latticepoint < size().
     */
    virtual void GetODEJacobian(unsigned int latticepoint, const SpinorFunction& fg, double** jacobian, double* dwdr) const = 0;
    
    /** Get approximation to solution for first numpoints near the origin. */
    virtual void EstimateOrbitalNearOrigin(unsigned int numpoints, SpinorFunction& s) const = 0;
    
    /** Get approximation to solution for last numpoints far from the origin.
        This routine can change the size of the orbital.
     */
    virtual void EstimateOrbitalNearInfinity(unsigned int numpoints, Orbital& s) const = 0;

    /** Get df/dr and dg/dr given (f, g).
        POST: This function can call SetODEParameters(), changing exchange and include_exchange.
     */
    virtual void GetDerivative(Orbital& fg);
    
protected:
    bool include_nonlocal;
};

typedef std::shared_ptr<SpinorODE> pSpinorODE;
typedef std::shared_ptr<const SpinorODE> pSpinorODEConst;

/** SpinorODEDecorator is for adding extra terms to an existing SpinorODE.
    The Decorator pattern allows nesting of additional terms in any order.
    When using, remember that the Decorator wraps objects, not classes.
 */
class SpinorODEDecorator : public SpinorODE
{
public:
    SpinorODEDecorator(pSpinorODE decorated_object): SpinorODE(decorated_object->GetLattice()), wrapped(decorated_object) {}
    virtual ~SpinorODEDecorator() {}

    /** Lattice has changed size, but maybe I don't care. */
    virtual void Alert() override {}

    /** Set exchange (nonlocal) potential and energy for ODE routines. */
    virtual void SetODEParameters(int kappa, double energy, const SpinorFunction* exchange = NULL)
    {   return wrapped->SetODEParameters(kappa, energy, exchange);
    }

    /** Set exchange (nonlocal) potential and energy for ODE routines. */
    virtual void SetODEParameters(const Orbital& approximation)
    {   return wrapped->SetODEParameters(approximation);
    }

    /** Get exchange (nonlocal) potential. */
    virtual SpinorFunction GetExchange(pOrbitalConst approximation = pOrbitalConst()) const
    {   return wrapped->GetExchange(approximation);
    }

    /** Tell SpinorODE whether to include the nonlocal (w_const) terms in GetODEFunction, GetODECoefficients, and GetODEJacobian. */
    virtual void IncludeExchange(bool include_exchange) override
    {   include_nonlocal = include_exchange;
        wrapped->IncludeExchange(include_exchange);
    }

    /** Get df/dr = w[0] and dg/dr = w[1] given point r, (f, g).
        PRE: w should be an allocated 2 dimensional array.
     */
    virtual void GetODEFunction(unsigned int latticepoint, const SpinorFunction& fg, double* w) const
    {   return wrapped->GetODEFunction(latticepoint, fg, w);
    }

    /** Get numerical coefficients of the ODE at the point r, (f,g).
        PRE: w_f, w_g, and w_const should be allocated 2 dimensional arrays.
     */
    virtual void GetODECoefficients(unsigned int latticepoint, const SpinorFunction& fg, double* w_f, double* w_g, double* w_const) const
    {   return wrapped->GetODECoefficients(latticepoint, fg, w_f, w_g, w_const);
    }

    /** Get Jacobian (dw[i]/df and dw[i]/dg), and dw[i]/dr at a point r, (f, g).
         PRE: jacobian should be an allocated 2x2 matrix,
              dwdr should be an allocated 2 dimensional array.
     */
    virtual void GetODEJacobian(unsigned int latticepoint, const SpinorFunction& fg, double** jacobian, double* dwdr) const
    {   return wrapped->GetODEJacobian(latticepoint, fg, jacobian, dwdr);
    }
    
    /** Get approximation to eigenfunction for first numpoints near the origin. */
    virtual void EstimateOrbitalNearOrigin(unsigned int numpoints, SpinorFunction& s) const
    {   return wrapped->EstimateOrbitalNearOrigin(numpoints, s);
    }
    
    /** Get approximation to eigenfunction for last numpoints far from the origin.
        This routine can change the size of the orbital.
     */
    virtual void EstimateOrbitalNearInfinity(unsigned int numpoints, Orbital& s) const
    {   return wrapped->EstimateOrbitalNearInfinity(numpoints, s);
    }

protected:
    pSpinorODE wrapped;
};

#endif
