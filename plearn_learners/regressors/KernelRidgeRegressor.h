// -*- C++ -*-

// KernelRidgeRegressor.h
//
// Copyright (C) 2005 Nicolas Chapados 
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

/* *******************************************************      
   * $Id: .pyskeleton_header 544 2003-09-01 00:05:31Z plearner $ 
   ******************************************************* */

// Authors: Nicolas Chapados

/*! \file KernelRidgeRegressor.h */


#ifndef KernelRidgeRegressor_INC
#define KernelRidgeRegressor_INC

// From PLearn
#include <plearn/ker/Kernel.h>
#include <plearn_learners/generic/PLearner.h>

namespace PLearn {

/**
 *  Implements a 'kernelized' version of linear ridge regression.
 *
 *  Given a kernel K(x,y) = phi(x)'phi(y), where phi(x) is the projection of a
 *  vector x into feature space, this class implements a version of the ridge
 *  estimator, giving the prediction at x as
 *
 *      f(x) = k(x)'(M + lambda I)^-1 y,
 *
 *  where x is the test vector where to estimate the response, k(x) is the
 *  vector of kernel evaluations between the test vector and the elements of
 *  the training set, namely
 *
 *      k(x) = (K(x,x1), K(x,x2), ..., K(x,xN))',
 *
 *  M is the Gram Matrix on the elements of the training set, i.e. the matrix
 *  where the element (i,j) is equal to K(xi, xj), lamdba is the weight decay
 *  coefficient, and y is the vector of training-set targets.
 *
 *  The disadvantage of this learner is that its training time is O(N^3) in the
 *  number of training examples (due to the matrix inversion).  When saving the
 *  learner, the training set must be saved, along with an additional vector of
 *  the length of the training set.
 */
class KernelRidgeRegressor : public PLearner
{
    typedef PLearner inherited;

public:
    //#####  Public Build Options  ############################################

    //! Whether to include a bias term in the regression (true by default)
    bool m_include_bias;
    
    //! Kernel to use for the computation.  This must be a similarity kernel
    //! (i.e. closer vectors give higher kernel evaluations).
    Ker m_kernel;

    //! Weight decay coefficient (default = 0)
    real m_weight_decay;
    

public:
    //#####  Public Member Functions  #########################################

    //! Default constructor
    KernelRidgeRegressor();


    //#####  PLearner Member Functions  #######################################

    //! Returns the size of this learner's output, (which typically
    //! may depend on its inputsize(), targetsize() and set options).
    virtual int outputsize() const;

    //! (Re-)initializes the PLearner in its fresh state (that state may depend
    //! on the 'seed' option) and sets 'stage' back to 0 (this is the stage of a
    //! fresh learner!).
    virtual void forget();
    
    //! The role of the train method is to bring the learner up to stage==nstages,
    //! updating the train_stats collector with training costs measured on-line
    //! in the process.
    virtual void train();

    //! Computes the output from the input.
    virtual void computeOutput(const Vec& input, Vec& output) const;

    //! Computes the costs from already computed output. 
    virtual void computeCostsFromOutputs(const Vec& input, const Vec& output, 
                                         const Vec& target, Vec& costs) const;
    
    //! Returns the names of the costs computed by computeCostsFromOutputs (and
    //! thus the test method). 
    virtual TVec<std::string> getTestCostNames() const;

    //! Returns the names of the objective costs that the train method computes and 
    //! for which it updates the VecStatsCollector train_stats.
    virtual TVec<std::string> getTrainCostNames() const;


    //#####  PLearn::Object Protocol  #########################################

    // Declares other standard object methods.
    PLEARN_DECLARE_OBJECT(KernelRidgeRegressor);

    // Simply calls inherited::build() then build_() 
    virtual void build();

    //! Transforms a shallow copy into a deep copy
    virtual void makeDeepCopyFromShallowCopy(CopiesMap& copies);

protected:
    //#####  Protected Options  ###############################################

    //! Vector of learned parameters, determined from the equation
    //!   (M + lambda I)^-1 y
    //! (don't forget that y can be a matrix for multivariate output problems)
    Mat m_params;

    //! Saved version of the training set inputs, which must be kept along for
    //! carrying out kernel evaluations with the test point
    VMat m_training_inputs;

    //! Buffer for kernel evaluations at test time
    mutable Vec m_kernel_evaluations;
    
protected:
    //! Declares the class options.
    static void declareOptions(OptionList& ol);

private: 
    //! This does the actual building. 
    void build_();
};

// Declares a few other classes and functions related to this class
DECLARE_OBJECT_PTR(KernelRidgeRegressor);
  
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
