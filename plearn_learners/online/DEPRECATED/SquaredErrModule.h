// -*- C++ -*-

// SquaredErrModule.h
//
// Copyright (C) 2005 Pascal Lamblin
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
   * $Id: SquaredErrModule.h,v 1.1 2005/11/30 04:36:17 lamblinp Exp $
   ******************************************************* */

// Authors: Pascal Lamblin

/*! \file PLearn/plearn_learners/online/DEPRECATED/SquaredErrModule.h */


#ifndef SquaredErrModule_INC
#define SquaredErrModule_INC

#include <plearn/base/Object.h>
#include <plearn/math/TMat_maths.h>
#include <plearn_learners/online/OnlineLearningModule.h>

namespace PLearn {

/**
 * Squared difference (and derivatives thereof) between the target and input.
 *
 * @deprecated: Use ../SquaredErrorCostModule instead
 */
class SquaredErrModule : public OnlineLearningModule
{
    typedef OnlineLearningModule inherited;

public:
    //#####  Public Build Options  ############################################


public:
    //#####  Public Member Functions  #########################################

    //! Default constructor
    SquaredErrModule();

    // Your other public member functions go here
/*
    virtual void setTarget(const Vec the_target);
    virtual void setTarget(real the_target);
    virtual Vec getTarget() const;
*/
    virtual void fprop(const Vec& input, Vec& output) const;

    virtual void bpropUpdate(const Vec& input, const Vec& output,
                             const Vec& output_gradient);

    virtual void bpropUpdate(const Vec& input, const Vec& output,
                             Vec& input_gradient, const Vec& output_gradient);

    virtual void bbpropUpdate(const Vec& input, const Vec& output,
                              const Vec& output_gradient,
                              const Vec& output_diag_hessian);

    virtual void bbpropUpdate(const Vec& input, const Vec& output,
                              Vec& input_gradient,
                              const Vec& output_gradient,
                              Vec& input_diag_hessian,
                              const Vec& output_diag_hessian);


    virtual void forget();

    //#####  PLearn::Object Protocol  #########################################

    // Declares other standard object methods.
    PLEARN_DECLARE_OBJECT(SquaredErrModule);

    // Simply calls inherited::build() then build_()
    virtual void build();

    //! Transforms a shallow copy into a deep copy
    // (PLEASE IMPLEMENT IN .cc)
    virtual void makeDeepCopyFromShallowCopy(CopiesMap& copies);

protected:
    //#####  Protected Options  ###############################################
//    Vec target;
    int target_size;
    //#####  Protected Member Functions  ######################################

    //! Declares the class options.
    static void declareOptions(OptionList& ol);

private:
    //#####  Private Member Functions  ########################################

    //! This does the actual building.
    void build_();

private:
    //#####  Private Data Members  ############################################

    // The rest of the private stuff goes here
};

// Declares a few other classes and functions related to this class
DECLARE_OBJECT_PTR(SquaredErrModule);

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
