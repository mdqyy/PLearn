// -*- C++ -*-

// RemoveDuplicateVMatrix.h
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
   * $Id: RemoveDuplicateVMatrix.h,v 1.2 2004/09/14 16:04:39 chrish42 Exp $ 
   ******************************************************* */

// Authors: Olivier Delalleau

/*! \file RemoveDuplicateVMatrix.h */


#ifndef RemoveDuplicateVMatrix_INC
#define RemoveDuplicateVMatrix_INC

#include "SelectRowsVMatrix.h"

namespace PLearn {
using namespace std;

class RemoveDuplicateVMatrix: public SelectRowsVMatrix
{

private:

  typedef SelectRowsVMatrix inherited;

protected:

  // *********************
  // * protected options *
  // *********************

public:

  // ************************
  // * public build options *
  // ************************

  real epsilon;
  int verbosity;

  // ****************
  // * Constructors *
  // ****************

  //! Default constructor.
  RemoveDuplicateVMatrix();

  // ******************
  // * Object methods *
  // ******************

private: 

  //! This does the actual building. 
  void build_();

protected: 

  //! Declares this class' options
  static void declareOptions(OptionList& ol);

public:

  // Simply calls inherited::build() then build_().
  virtual void build();

  //! Transforms a shallow copy into a deep copy.
  virtual void makeDeepCopyFromShallowCopy(CopiesMap& copies);

  //! Declares name and deepCopy methods
  PLEARN_DECLARE_OBJECT(RemoveDuplicateVMatrix);

};

DECLARE_OBJECT_PTR(RemoveDuplicateVMatrix);

} // end of namespace PLearn

#endif
