// -*- C++ -*-

// LLE.h
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
   * $Id: LLE.h,v 1.5 2004/09/14 16:04:59 chrish42 Exp $ 
   ******************************************************* */

// Authors: Olivier Delalleau

/*! \file LLE.h */


#ifndef LLE_INC
#define LLE_INC

#include "KernelProjection.h"
#include <plearn/ker/LLEKernel.h>

namespace PLearn {
using namespace std;

class LLE: public KernelProjection
{

private:

  typedef KernelProjection inherited;
  
protected:

  // *********************
  // * protected options *
  // *********************

  PP<LLEKernel> lle_kernel;

public:

  // ************************
  // * public build options *
  // ************************

  bool classical_induction;
  int knn;
  real reconstruct_coeff;
  real regularizer;

  // ****************
  // * Constructors *
  // ****************

  //! Default constructor.
  LLE();


  // ****************************
  // * KernelProjection methods *
  // ****************************

private: 

  //! This does the actual building. 
  void build_();

protected: 
  
  //! Declares this class' options.
  static void declareOptions(OptionList& ol);

public:

  // ************************
  // **** Object methods ****
  // ************************

  //! Simply calls inherited::build() then build_().
  virtual void build();

  //! Transforms a shallow copy into a deep copy.
  virtual void makeDeepCopyFromShallowCopy(CopiesMap& copies);

  // Declares other standard object methods.
  PLEARN_DECLARE_OBJECT(LLE);


  // **********************************
  // **** KernelProjection methods ****
  // **********************************

  //! (Re-)initializes the PLearner in its fresh state.
  virtual void forget();

};

// Declares a few other classes and functions related to this class.
DECLARE_OBJECT_PTR(LLE);
  
} // end of namespace PLearn

#endif
