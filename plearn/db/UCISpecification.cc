// -*- C++ -*-

// UCISpecification.cc
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
   * $Id: UCISpecification.cc,v 1.1 2004/07/09 13:03:42 tihocan Exp $ 
   ******************************************************* */

// Authors: Olivier Delalleau

/*! \file UCISpecification.cc */


#include "UCISpecification.h"

namespace PLearn {
using namespace std;

UCISpecification::UCISpecification() 
: file_all(""),
  file_test(""),
  file_train(""),
  inputsize(-1),
  targetsize(-1),
  weightsize(-1),
  target_is_first(0)
{
}

PLEARN_IMPLEMENT_OBJECT(UCISpecification, "ONE LINE DESCRIPTION", "MULTI LINE\nHELP");

void UCISpecification::declareOptions(OptionList& ol)
{
  // ### Declare all of this object's options here
  // ### For the "flags" of each option, you should typically specify  
  // ### one of OptionBase::buildoption, OptionBase::learntoption or 
  // ### OptionBase::tuningoption. Another possible flag to be combined with
  // ### is OptionBase::nosave

  declareOption(ol, "file_all", &UCISpecification::file_all, OptionBase::buildoption,
      "The filename for the whole dataset");

  declareOption(ol, "file_train", &UCISpecification::file_train, OptionBase::buildoption,
      "The filename for the train data.");

  declareOption(ol, "file_test", &UCISpecification::file_test, OptionBase::buildoption,
      "The filename for the test data.");

  declareOption(ol, "inputsize", &UCISpecification::inputsize, OptionBase::buildoption,
      "Input size of the data");

  declareOption(ol, "targetsize", &UCISpecification::targetsize, OptionBase::buildoption,
      "Target size of the data");

  declareOption(ol, "weightsize", &UCISpecification::weightsize, OptionBase::buildoption,
      "Weight size of the data");

  declareOption(ol, "target_is_first", &UCISpecification::target_is_first, OptionBase::buildoption,
        "Whether the target is the first column (otherwise it is assumed to be the last one..");

  // Now call the parent class' declareOptions
  inherited::declareOptions(ol);
}

void UCISpecification::build_()
{
  // ### This method should do the real building of the object,
  // ### according to set 'options', in *any* situation. 
  // ### Typical situations include:
  // ###  - Initial building of an object from a few user-specified options
  // ###  - Building of a "reloaded" object: i.e. from the complete set of all serialised options.
  // ###  - Updating or "re-building" of an object after a few "tuning" options have been modified.
  // ### You should assume that the parent class' build_() has already been called.
}

// ### Nothing to add here, simply calls build_
void UCISpecification::build()
{
  inherited::build();
  build_();
}

void UCISpecification::makeDeepCopyFromShallowCopy(map<const void*, void*>& copies)
{
  inherited::makeDeepCopyFromShallowCopy(copies);

  // ### Call deepCopyField on all "pointer-like" fields 
  // ### that you wish to be deepCopied rather than 
  // ### shallow-copied.
  // ### ex:
  // deepCopyField(trainvec, copies);

  // ### Remove this line when you have fully implemented this method.
  PLERROR("UCISpecification::makeDeepCopyFromShallowCopy not fully (correctly) implemented yet!");
}

} // end of namespace PLearn
