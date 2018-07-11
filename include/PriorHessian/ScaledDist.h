/** @file ScaledDist.h
 * @author Mark J. Olah (mjo\@cs.unm DOT edu)
 * @date 2017
 * @brief SemiInfiniteDist class declaration and templated methods
 * 
 */
#ifndef _PRIOR_HESSIAN_SCALEDDIST_H
#define _PRIOR_HESSIAN_SCALEDDIST_H

#include <cmath>

#include "PriorHessian/PriorHessianError.h"

namespace prior_hessian {

template<class Dist>
class ScaledDist : public Dist
{
public:
    ScaledDist();
    ScaledDist(double lbound, double ubound);
    explicit ScaledDist(const Dist &dist);
    explicit ScaledDist(Dist &&dist);
    ScaledDist(const Dist &dist, double lbound, double ubound);
    ScaledDist(Dist &&dist, double lbound, double ubound);

    double lbound() const;
    double ubound() const;
    double unscaled_lbound() const;
    double unscaled_ubound() const;
    
    void set_lbound(double lbound);
    void set_ubound(double ubound);
    void set_bounds(double lbound, double ubound);

    double cdf(double x) const;
    double pdf(double x) const;
    double icdf(double u) const;
    double llh(double x) const;

    template<class RngT>
    double sample(RngT &rng) const;
protected:
    double _scaled_lbound;
    double _scaled_ubound;
    
    double scaling_ratio; // scaled_bounds_delta / unscaled_bounds_delta
    double llh_scaling_const; // -log(scaling_ratio)
    
    double convert_to_unitary_coords(double x) const;
    double convert_from_unitary_coords(double u) const;
};


template<class Dist>
ScaledDist<Dist>::ScaledDist() 
    : ScaledDist(Dist{})
{ }

template<class Dist>
ScaledDist<Dist>::ScaledDist(double lbound, double ubound) 
    : ScaledDist(Dist{}, lbound, ubound)
{ }

template<class Dist>
ScaledDist<Dist>::ScaledDist(const Dist &dist) 
    : ScaledDist(dist, dist.lbound(), dist.ubound())
{ }

template<class Dist>
ScaledDist<Dist>::ScaledDist(Dist &&dist)
    : ScaledDist(std::move(dist), dist.lbound(), dist.ubound())
{ }

template<class Dist>
ScaledDist<Dist>::ScaledDist(const Dist &dist, double lbound, double ubound)
    : Dist(dist)
{ set_bounds(lbound,ubound); }

template<class Dist>
ScaledDist<Dist>::ScaledDist(Dist &&dist, double lbound, double ubound)
    : Dist(std::move(dist))
{ set_bounds(lbound,ubound); }

template<class Dist>
double ScaledDist<Dist>::lbound() const
{ return _scaled_lbound; }

template<class Dist>
double ScaledDist<Dist>::ubound() const
{ return _scaled_ubound; }

template<class Dist>
double ScaledDist<Dist>::unscaled_lbound() const
{ return Dist::lbound(); }

template<class Dist>
double ScaledDist<Dist>::unscaled_ubound() const
{ return Dist::ubound(); }

template<class Dist>
void ScaledDist<Dist>::set_bounds(double lbound, double ubound)
{
    if( !(lbound < ubound) || !std::isfinite(lbound) || !std::isfinite(ubound) ){
        std::ostringstream msg;
        msg<<"set_bounds: Invalid bounds lbound:"<<lbound<<" should be < ubound:"<<ubound;
        throw ParameterValueError(msg.str());
    }
    double unscaled_delta = unscaled_ubound() - unscaled_lbound();
    if( !std::isfinite(unscaled_delta) ){
        std::ostringstream msg;
        msg<<"set_bounds: Invalid distribution with non-finite bounds cannot be scaled."
           <<"Unscaled: [ lbound:"<<unscaled_lbound()<<" ubound:"<<unscaled_ubound()<<"]";
        throw ParameterValueError(msg.str());
    }
        
    scaling_ratio = (ubound - lbound) / unscaled_delta;
    llh_scaling_const = -log(scaling_ratio);
    _scaled_lbound = lbound;
    _scaled_ubound = ubound;
}

template<class Dist>
void ScaledDist<Dist>::set_lbound(double lbound)
{ set_bounds(lbound,ubound()); }

template<class Dist>
void ScaledDist<Dist>::set_ubound(double ubound)
{ set_bounds(lbound(),ubound); }

template<class Dist>
double ScaledDist<Dist>::llh(double x) const
{
    return this->Dist::llh(x) + llh_scaling_const;
}

template<class Dist>
double ScaledDist<Dist>::cdf(double x) const
{
    return this->Dist::cdf( convert_to_unitary_coords(x) );
}

template<class Dist>
double ScaledDist<Dist>::icdf(double u) const
{
    return convert_from_unitary_coords(this->Dist::icdf(u));
}

template<class Dist>
double ScaledDist<Dist>::pdf(double x) const
{
    return this->Dist::pdf(convert_to_unitary_coords(x)) / scaling_ratio; //Correct for scaling by bounds_delta factor
}

template<class Dist>
template<class RngT>
double ScaledDist<Dist>::sample(RngT &rng) const
{
    return convert_from_unitary_coords(Dist::sample(rng)); 
}


template<class Dist>
double ScaledDist<Dist>::convert_to_unitary_coords(double x) const
{
    return (x - lbound()) / scaling_ratio + unscaled_lbound();
}

template<class Dist>
double ScaledDist<Dist>::convert_from_unitary_coords(double u) const
{
    return lbound() + (u-unscaled_lbound())*scaling_ratio;
}


} /* namespace prior_hessian */

#endif /* _PRIOR_HESSIAN_SCALEDDIST_H */