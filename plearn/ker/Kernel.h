// -*- C++ -*-

// PLearn (A C++ Machine Learning Library)
// Copyright (C) 1998 Pascal Vincent
// Copyright (C) 1999-2002 Pascal Vincent, Yoshua Bengio, Rejean Ducharme and University of Montreal
// Copyright (C) 2001-2002 Nicolas Chapados, Ichiro Takeuchi, Jean-Sebastien Senecal
// Copyright (C) 2002 Xiangdong Wang, Christian Dorion

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
   * $Id: Kernel.h,v 1.23 2004/04/07 23:15:17 morinf Exp $
   * This file is part of the PLearn library.
   ******************************************************* */

#ifndef Kernel_INC
#define Kernel_INC

#include "Object.h"
#include "VMat.h"
//#include "PLMPI.h"

namespace PLearn {
using namespace std;

class Kernel: public Object
{

private:

  // norman: moved to public (Check this!)
  typedef Object inherited;
		
protected:

  VMat data; //!<  data for kernel matrix, which will be used for calls to evaluate_i_j and the like
  int data_inputsize;   //< The inputsize of data (if -1, is set to data.width()).

  static void declareOptions(OptionList& ol);

public:

  //! Build options.
  bool is_symmetric;
  int report_progress;
  VMat specify_dataset;

  //! Constructor.
  Kernel(bool is__symmetric = true);

  PLEARN_DECLARE_ABSTRACT_OBJECT(Kernel);

  //!  ** Subclasses must overload this method **
  virtual real evaluate(const Vec& x1, const Vec& x2) const = 0; //!<  returns K(x1,x2) 

  //!  ** Subclasses may overload these methods to provide efficient kernel matrix access **

  /*!     
    This method sets the data VMat that will be used to define the kernel
    matrix. It may precompute values from this that may later accelerate
    the evaluation of a kernel matrix element
  */
  virtual void setDataForKernelMatrix(VMat the_data);

  /*!
    This method is meant to be used any time the data matrix
    is appended a new row by an outer instance (e.g. SequentialKernel).
    Through this method, the kernel must update any data dependant internal 
    structure. The internal structures should have consistant length with 
    the data matrix, assuming a sequential growing of the vmat.
  */
  virtual void addDataForKernelMatrix(const Vec& newRow);

  virtual int nExamples()
    { return data.length(); }

  //!  returns evaluate(data(i),data(j))
  virtual real evaluate_i_j(int i, int j) const; 

/*!     returns evaluate(data(i),x)
    [squared_norm_of_x is just a hint that may
    allow to speed up computation if it is already known, 
    but it's optional]
*/
  virtual real evaluate_i_x(int i, const Vec& x, real squared_norm_of_x=-1) const; 

  //!  returns evaluate(x,data(i)) [default version calls evaluate_i_x if
  //!  kernel is_symmetric]
  virtual real evaluate_x_i(const Vec& x, int i, real squared_norm_of_x=-1) const; 

  //! Call evaluate_i_j to fill each of the entries (i,j) of symmetric matrix K.
  virtual void computeGramMatrix(Mat K) const;

  //!  ** Subclasses may overload these methods ** 
  //!  They provide a generic way to set and retrieve kernel parameters
  virtual void setParameters(Vec paramvec); //!<  default version produces an error
  virtual Vec getParameters() const; //!<  default version returns an empty Vec

  //!  ** Subclasses should NOT overload the following methods. The default versions are fine. **

  void apply(VMat m1, VMat m2, Mat& result) const; //!<  result(i,j) = K(m1(i),m2(j))
  Mat apply(VMat m1, VMat m2) const; //!<  same as above, but returns the result mat instead
  void apply(VMat m, const Vec& x, Vec& result) const; //!<  result[i]=K(m[i],x)
  void apply(Vec x, VMat m, Vec& result) const; //!<  result[i]=K(x,m[i])

  inline real operator()(const Vec& x1, const Vec& x2) const
    { return evaluate(x1,x2); }

  //! Return true iif there is a data matrix set for this kernel.
  bool hasData();

  //! Return the data matrix set for this kernel.
  inline VMat getData() {return this->data;}

  //!  Returns a Mat m such that m(i,j) is the index of jth closest neighbour of input i, 
  //!  according to the "distance" measures given by D(i,j)
  static Mat computeNeighbourMatrixFromDistanceMatrix(const Mat& D, bool insure_self_first_neighbour=true, bool report_progress = false);

  Mat estimateHistograms(VMat d, real sameness_threshold, real minval, real maxval, int nbins) const;
  Mat estimateHistograms(Mat input_and_class, real minval, real maxval, int nbins) const;
  real test(VMat d, real threshold, real sameness_below_threshold, real sameness_above_threshold) const;
  virtual void build();
    //virtual void oldwrite(ostream& out) const;
    //virtual void oldread(istream& in);
  virtual ~Kernel();

  virtual void makeDeepCopyFromShallowCopy(CopiesMap& copies);
private:

  virtual void build_();

};
DECLARE_OBJECT_PTR(Kernel);

/*! *******
  * Ker *
  *******
*/
class Ker: public PP<Kernel>
{
public:
  Ker() {}
  Ker(Kernel* v) :PP<Kernel>(v) {}
  Ker(const Ker& other) :PP<Kernel>(other) {}

  real operator()(const Vec& x1, const Vec& x2) const
    { return ptr->evaluate(x1,x2); }
};

DECLARE_OBJECT_PP(Ker, Kernel);

template <>
inline
void deepCopyField(Ker& field, CopiesMap& copies)
{
  if (field)
    field = static_cast<Kernel*>(field->deepCopy(copies));
}

// last column of data is supposed to be a classnum
// returns a matrix of (index1, index2, distance)
Mat findClosestPairsOfDifferentClass(int k, VMat data, Ker dist);

//!  ********************
//!  inline Ker operators

inline Array<Ker> operator&(const Ker& k1, const Ker& k2)
{ return Array<Ker>(k1,k2); }

/*!************
 * CostFunc *
 ************
*/

//!  a cost function maps (output,target) to a loss
typedef Ker CostFunc;

/*!**********************************************************************
  FINANCIAL STUFF
  **********************************************************************
*/

//!  a profit function maps (output,target) to a profit
typedef CostFunc ProfitFunc;


} // end of namespace PLearn

#endif

