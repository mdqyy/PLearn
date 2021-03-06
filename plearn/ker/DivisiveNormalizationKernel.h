// -*- C++ -*-

// DivisiveNormalizationKernel.h
//
// Copyright (C) 2004 Olivier Delalleau 
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
 * $Id$ 
 ******************************************************* */

// Authors: Olivier Delalleau

/*! \file DivisiveNormalizationKernel.h */


#ifndef DivisiveNormalizationKernel_INC
#define DivisiveNormalizationKernel_INC

#include "SourceKernel.h"

namespace PLearn {
using namespace std;

class DivisiveNormalizationKernel: public SourceKernel
{

private:

    typedef SourceKernel inherited;
  
    //! Used to store the values of the source kernel.
    mutable Vec all_k_x;
  
protected:

    // *********************
    // * Protected options *
    // *********************

    Vec average_col;
    Vec average_row;

    // Fields below are not options.

    //! The last average computed in evaluate_i_x_again().
    mutable real avg_evaluate_i_x_again;

    //! The last average computed in evaluate_x_i_again().
    mutable real avg_evaluate_x_i_again;

public:

    // ************************
    // * Public build options *
    // ************************

    bool data_will_change;
    bool remove_bias;

    // ****************
    // * Constructors *
    // ****************

    //! Default constructor.
    DivisiveNormalizationKernel();

    //! Created from an existing kernel.
    DivisiveNormalizationKernel(Ker the_source, bool the_remove_bias = false);

    // ************************
    // * SourceKernel methods *
    // ************************

private: 

    //! This does the actual building. 
    void build_();

protected: 
  
    //! Declares this class' options.
    static void declareOptions(OptionList& ol);

    //! Return the average of K(x,x_i) or K(x_i,x), depending on the value of
    //! 'on_row' (true or false, respectively).
    inline real computeAverage(const Vec& x, bool on_row, real squared_norm_of_x = -1) const;

public:

    // ************************
    // **** Object methods ****
    // ************************

    //! Simply calls inherited::build() then build_().
    virtual void build();

    //! Transforms a shallow copy into a deep copy.
    virtual void makeDeepCopyFromShallowCopy(CopiesMap& copies);

    // Declares other standard object methods.
    PLEARN_DECLARE_OBJECT(DivisiveNormalizationKernel);

    // ******************************
    // **** SourceKernel methods ****
    // ******************************

    //! Overridden.
    virtual real evaluate(const Vec& x1, const Vec& x2) const;
    virtual real evaluate_i_j(int i, int j) const;
    virtual real evaluate_i_x(int i, const Vec& x, real squared_norm_of_x=-1) const;
    virtual real evaluate_x_i(const Vec& x, int i, real squared_norm_of_x=-1) const;
    virtual real evaluate_i_x_again(int i, const Vec& x, real squared_norm_of_x=-1, bool first_time = false) const;
    virtual real evaluate_x_i_again(const Vec& x, int i, real squared_norm_of_x=-1, bool first_time = false) const;
    virtual void computeGramMatrix(Mat K) const;
    virtual void setDataForKernelMatrix(VMat the_data);
  
    // You may also want to override these methods if you don't want them
    // to be directly forwarded to the underlying kernel.
    // virtual void addDataForKernelMatrix(const Vec& newRow);
    // virtual void setParameters(Vec paramvec);
    // virtual Vec getParameters() const;

};

// Declares a few other classes and functions related to this class.
DECLARE_OBJECT_PTR(DivisiveNormalizationKernel);
  
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
