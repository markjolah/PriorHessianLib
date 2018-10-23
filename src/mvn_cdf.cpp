/** @file NormalDist.cpp
 * @author Mark J. Olah (mjo\@cs.unm DOT edu)
 * @date 2017-2018
 * @brief NormalDist class defintion
 * 
 */
#include "PriorHessian/PriorHessianError.h"

#include <cmath>
#include <limits>

#include <armadillo>

#include <boost/math/special_functions/erf.hpp>
#include <boost/math/constants/constants.hpp>


namespace {
    const double sqrt2 = sqrt(2.);
    const double inv_sqrt2 = 1./sqrt2;
    const double inv_2pi = 1./(2.*arma::datum::pi);
}

namespace prior_hessian {


/** area of the lower tail of the unit normal curve below t. */
double unit_normal_cdf( double t )
{
    if(t==-INFINITY) return 0;
    if(t==INFINITY) return 1;
    return .5*(1+std::erf(t*::inv_sqrt2));
}

double unit_normal_icdf( double u )
{
    if(u<=0) return -INFINITY;
    if(u>=1) return INFINITY;
    return ::sqrt2*boost::math::erf_inv(2*u-1);
}

double bounded(double x)
{
    return std::min(std::max(x,0.),1.);
}

/** compute the bivariate normal cdf integral
 * computes the probability for two normal variates X and Y
 *    whose correlation is R, that AH <= X and AK <= Y.
 * 
 * Adapted to modern C++ with efficiency improvements by:
 * Mark Olah (mjo@cs.unm DOT edu)
 * 10/2018
 * 
 * Reference:
 *    Thomas Donnelly,
 *    Algorithm 462: Bivariate Normal Distribution,
 *    Communications of the ACM,
 *    October 1973, Volume 16, Number 10, page 638.
 */    
double bvn_integral( double ah, double ak, double r )
{
    static int eps = 1.0E-15;
    if (r<-1 || r>1) throw ParameterValueError("must have -1<=rho<=1");

    double gh = unit_normal_cdf(-ah)/2.0;
    double gk = unit_normal_cdf(-ak)/2.0;

    if(r == 0.0) return bounded(4*gh*gk);

    double b = 0.0; //return value
    double rr = (1+r)*(1-r);
    if(rr==0) { //Degenerate cases r=-1 r=0 r=1
        if(r<0) {
            if(ah+ak<0) b = 2*(gh+gk)-1;
        } else {
            if(ah-ak<0) b = 2*gk;
            else b = 2*gh;
        }
        return bounded(b);
    }

    double isqr = 1./sqrt(rr);
    double con = arma::datum::pi * eps;
    double wh;
    double wk;
    double gw;
    int is;
    if(ah == 0) {
        if(ak == 0) return bounded(0.25+asin(r)*::inv_2pi); // (0,0)
        //(0, !0)
        b = gk;
        wh = -ak;
        wk = (ah/ak - r)*isqr;
        gw = 2*gk;
        is = 1;
    } else {
        //  (!0, 0)
        b = gh;             
        wh = -ah;
        wk = (ak/ah - r)*isqr;
        gw = 2*gh;
        is = -1;
        if(ak > 0) { // ( !0, !0)
            b = gh+gk;
            if (ah*ak < 0) b-= 0.5;
        }
    }

    for ( ; ; ) {
        double sgn = -1.0;
        double t = 0.0;

        if(wk != 0) {
            if(fabs(wk) == 1) {
                t = .5*wk*gw*(1-gw);
                b += sgn*t;
            } else {
                if (fabs(wk) > 1) {
                    sgn = -sgn;
                    wh = wh*wk;
                    double g2 = unit_normal_cdf(wh);
                    wk = 1./wk;
                    if(wk < 0) b += 0.5;
                    b -= .5*(gw+g2) + gw*g2;
                }
                double h2 = wh*wh;
                double a2 = wk*wk;
                double h4 = .5*h2;
                double ex = exp(-h4);
                double w2 = h4*ex;
                double ap = 1.0;
                double s2 = ap - ex;
                double sp = ap;
                double s1 = 0.0;
                double sn = s1;
                double conex = fabs(con/wk);

                for ( ; ; ) {
                    double cn = ap*s2/(sn+sp);
                    s1 += cn;
                    if(fabs(cn) <= conex) break;

                    sn = sp;
                    sp += 1.0;
                    s2 = s2-w2;
                    w2 = w2*h4/sp;
                    ap = -ap*a2;
                }
                t = (atan(wk) - wk*s1) * ::inv_2pi;
                b += sgn*t;
            }
        }
        //Check for convergence
        if(0 <= is || ak == 0) return bounded(b);
        wh = -ak;
        wk = (ah/ak - r)*isqr;
        gw = 2*gk;
        is = 1;
    }
    //never get here
}



} /* namespace prior_hessian */
