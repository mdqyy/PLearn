// -*- C++ -*-

// DictionaryVMatrix.h
//
// Copyright (C) 2004 Christopher Kermorvant 
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
   * $Id: DictionaryVMatrix.h,v 1.7 2004/09/14 16:04:39 chrish42 Exp $ 
   ******************************************************* */

// Authors: Christopher Kermorvant

/*! \file DictionaryVMatrix.h */


#ifndef DictionaryVMatrix_INC
#define DictionaryVMatrix_INC

#include <plearn/vmat/RowBufferedVMatrix.h>
#include <plearn/base/stringutils.h>
#include <plearn_learners/language/Dictionary/Dictionary.h>

// number of attributes of dictionary specifications
#define DIC_SPECIFICATION_SIZE 3

namespace PLearn {
using namespace std;

class DictionaryVMatrix: public RowBufferedVMatrix
{

private:
  
  //! Number of attributes in the input text file (\t separated)
  int attributes_number;
  typedef RowBufferedVMatrix inherited;

  Mat data;
protected:

  // *********************
  // * protected options *
  // *********************

public:

  // ************************
  // * public build options *
  // ************************

  //! The text input file which is processed with dictionaries 
  string input_file;

  //! The dictionaries, one for each attributes
  TVec< PP<Dictionary> > dictionaries;
  
  //! The options fields of every dictionaries
  TVec< TVec <int> > option_fields;

  //! String delimiters for input file fields
  string delimiters;

  // ****************
  // * Constructors *
  // ****************

  // Default constructor, make sure the implementation in the .cc
  // initializes all fields to reasonable default values.
  DictionaryVMatrix();

  // ******************
  // * Object methods *
  // ******************
  DictionaryVMatrix(const string filename);
private: 

  //! This does the actual building. 
  // (Please implement in .cc)
  void build_();
  void extractDicType();
  void buildDics();
protected: 

  //! Declares this class' options
  // (Please implement in .cc)
  static void declareOptions(OptionList& ol);

  //! Fill the vector 'v' with the content of the i-th row.
  //! v is assumed to be the right size.
   virtual void getNewRow(int i, const Vec& v) const;

  //! returns value associated with a string (or MISSING_VALUE if there's no association for this string)
  virtual real getStringVal(int col, const string & str) const;
  
  virtual string getValString(int col, real val) const;
  
  virtual int getDimension(int row, int col) const;

  virtual Vec getValues(int row, int col) const;

public:

  // Simply call inherited::build() then build_().
  virtual void build();

  //! Transform a shallow copy into a deep copy.
   virtual void makeDeepCopyFromShallowCopy(CopiesMap& copies);
  
  //  virtual void save(const string& filename) ;  
  //! Declare name and deepCopy methods.
  PLEARN_DECLARE_OBJECT(DictionaryVMatrix);

};

DECLARE_OBJECT_PTR(DictionaryVMatrix);

} // end of namespace PLearn
#endif
