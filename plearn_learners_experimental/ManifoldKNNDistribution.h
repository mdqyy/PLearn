// -*- C++ -*-

// ManifoldKNNDistribution.h
//
// Copyright (C) 2007 Hugo Larochelle
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//  1. Redistributions of source code must retain the above copyright
//     notice, this list of conditions and the following disclaimer.
//
//  2. Redistributions in binary form must reproduce the above copyright
//     notice, this list of conditions and the following disclaimer in the
//     documentation and/or other materials provided with the distribution.
//
//  3. The name of the authors may not be used to endorse or promote
//     products derived from this software without specific prior written
//     permission.
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS OR
// IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
// OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN
// NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
// TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// This file is part of the PLearn library. For more information on the PLearn
// library, go to the PLearn Web site at www.plearn.org

// Authors: Hugo Larochelle

/*! \file ManifoldKNNDistribution.h */


#ifndef ManifoldKNNDistribution_INC
#define ManifoldKNNDistribution_INC

#include <plearn_learners/distributions/UnconditionalDistribution.h>
#include <plearn_learners/nearest_neighbors/GenericNearestNeighbors.h>

namespace PLearn {

// -*- C++ -*-


/**
 * K nearest neighbors density estimator that takes into accound the local manifold
 * structure.
 */
class ManifoldKNNDistribution : public UnconditionalDistribution
{
    typedef UnconditionalDistribution inherited;

public:
    //#####  Public Build Options  ############################################

    //! Nearest neighbors search algorithms for local manifold structure estimation.
    PP<GenericNearestNeighbors> knn_manifold;

    //! Nearest neighbors search algorithms for density estimation from ellipsoid's
    //! volume.
    PP<GenericNearestNeighbors> knn_density;

    //! Dimensionality of the manifold.
    int manifold_dimensionality;
    
    //! Minimum variance in all directions on the manifold. This value is added
    //! to the estimated covariance matrix.
    real min_sigma_square;

    //! Indication that the estimation of the manifold tangent vectors
    //! should be made around the knn_manifold neighbors' mean vector,
    //! not around the test point.
    bool center_around_manifold_neighbors;

    //! Indication that a Gaussian distribution should be used as the
    //! knn_manifold nearest neighbors distribution, instead of the
    //! uniform in the ellipsoid.
    bool use_gaussian_distribution;

    //! Generic density learner for knn_density nearest neighbors
    PP<PDistribution> density_learner;

public:
    //#####  Public Member Functions  #########################################

    //! Default constructor
    // ### Make sure the implementation in the .cc
    // ### initializes all fields to reasonable default values.
    ManifoldKNNDistribution();


    //#####  UnconditionalDistribution Member Functions  ######################

    //! Return log of probability density log(p(y)).
    virtual real log_density(const Vec& x) const;

    //! Return survival function: P(Y>y).
    virtual real survival_fn(const Vec& y) const;

    //! Return cdf: P(Y<y).
    virtual real cdf(const Vec& y) const;

    //! Return E[Y].
    virtual void expectation(Vec& mu) const;

    //! Return Var[Y].
    virtual void variance(Mat& cov) const;

    //! Return a pseudo-random sample generated from the distribution.
    virtual void generate(Vec& y) const;

    //! Reset the random number generator used by generate() using the
    //! given seed.
    virtual void resetGenerator(long g_seed) const;

    //#####  PLearner Member Functions  #######################################

    //! (Re-)initializes the PDistribution in its fresh state (that state may
    //! depend on the 'seed' option) and sets 'stage' back to 0 (this is the
    //! stage of a fresh learner!).
    virtual void forget();

    //! The role of the train method is to bring the learner up to
    //! stage == nstages, updating the train_stats collector with training
    //! costs measured on-line in the process.
    virtual void train();


    //#####  PLearn::Object Protocol  #########################################

    // Declares other standard object methods.
    PLEARN_DECLARE_OBJECT(ManifoldKNNDistribution);

    // Simply calls inherited::build() then build_()
    virtual void build();

    //! Transforms a shallow copy into a deep copy
    virtual void makeDeepCopyFromShallowCopy(CopiesMap& copies);

protected:
    //#####  Protected Options  ###############################################

    //! Nearest neighbors for manifold estimation
    mutable Mat nearest_neighbors_manifold;
    //! Nearest neighbors for manifold estimation as a vector
    mutable Vec nearest_neighbors_manifold_vec;
    //! Nearest neighbors for density estimation
    mutable Mat nearest_neighbors_density;
    //! Nearest neighbors for density estimation as a vector
    mutable Vec nearest_neighbors_density_vec;
    //! Manifold local tangent vectors
    mutable Mat eig_vectors;
    //! Manifold local standard deviations
    mutable Vec eig_values;
    //! SVD temporary variables
    mutable Mat Ut, V;
    //! SVD temporary variable
    mutable Vec S;
    //! Projection on the local tangent vectors
    mutable Vec eig_vectors_projection;
    //! Mean vector of neighbors
    mutable Vec neighbors_mean;
    //! Difference between test point and neighbors_mean;
    mutable Vec test_minus_mean;

    //! The density learner training set
    mutable VMat density_learner_train_set;

protected:
    //#####  Protected Member Functions  ######################################

    //! Declares the class options.
    static void declareOptions(OptionList& ol);

    void computeLocalPrincipalComponents(const Vec& x, 
                                         Vec& eig_values, 
                                         Mat& eig_vectors) const;

private:
    //#####  Private Member Functions  ########################################

    //! This does the actual building.
    void build_();

private:
    //#####  Private Data Members  ############################################

    // The rest of the private stuff goes here
};

// Declares a few other classes and functions related to this class
DECLARE_OBJECT_PTR(ManifoldKNNDistribution);

} // end of namespace PLearn

#endif


/*
  Local Variables:
  mode:c++
  c-basic-offset:4
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0))
  indent-tabs-mode:nil
  fill-column:79
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=79 :
