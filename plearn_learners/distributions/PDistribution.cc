// -*- C++ -*-

// PDistribution.cc
//
// Copyright (C) 2003  Pascal Vincent 
// Copyright (C) 2004  Universit� de Montr�al
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
   * $Id: PDistribution.cc,v 1.16 2004/05/26 18:39:42 tihocan Exp $ 
   ******************************************************* */

/*! \file PDistribution.cc */
#include "PDistribution.h"

namespace PLearn {
using namespace std;

///////////////////
// PDistribution //
///////////////////
PDistribution::PDistribution() 
: n_input(0),
  n_margin(0),
  n_target(0),
  full_joint_distribution(true),
  need_set_input(true),
  n_curve_points(-1),
  outputs_def("l")
{}

PLEARN_IMPLEMENT_OBJECT(PDistribution, 
    "PDistribution is the base class for distributions.\n",
    "PDistributions derive from PLearner (as some of them may be fitted to data with train()),\n"
    "but they have additional methods allowing for ex. to compute density or generate data points.\n"
    "The default implementations of the learner-type methods for computing outputs and costs work as follows:\n"
    "  - the outputs_def option allows to choose what outputs are produced\n"
    "  - cost is a vector of size 1 containing only the negative log-likelihood (NLL) i.e. -log_density).\n"
    "A PDistribution may be conditional P(Y|X), if the option 'conditional_flags' is set. If it is the case,\n"
    "the input should always be made of both the 'input' part (X) and the 'target' part (Y), even if the\n"
    "output may not need to use the Y part. The exception is when computeOutput() needs to be called\n"
    "successively with the same value of X: in this case, after a first call with both X and Y, one may\n"
    "only provide Y as input, and X will be assumed to be unchanged.\n"
    );

////////////////////
// declareOptions //
////////////////////
void PDistribution::declareOptions(OptionList& ol)
{

  // Build options.

  declareOption(ol, "outputs_def", &PDistribution::outputs_def, OptionBase::buildoption,
      "A string where the characters have the following meaning: \n"
      "'l'-> log_density, 'd' -> density, 'c' -> cdf, 's' -> survival_fn,\n"
      "'e' -> expectation, 'v' -> variance.\n"
      "In lower case they give the value associated with a given observation.\n"
      "In upper case, a curve is evaluated at regular intervals and produced in\n"
      "output (as a histogram). For 'L', 'D', 'C', 'S', it is the target part that\n"
      "varies, while for 'E' and 'V' it is the input part.\n"
      "The number of curve points is determined by the 'n_curve_points' option.\n"
      "Note that these options upper case letters) only work for SCALAR variables."
      );

  declareOption(ol, "conditional_flags", &PDistribution::conditional_flags, OptionBase::buildoption,
      "Indicates what each input variable corresponds to:\n"
      " - 0 = it is marginalized (it does not appear in the distribution Y|X)\n"
      " - 1 = it is an input (the X in Y|X)\n"
      " - 2 = it is a target (the Y in Y|X)\n"
      "If this vector is empty, then all variables are considered targets (thus\n"
      "it is an unconditional distribution)."
      );

  declareOption(ol, "input_part", &PDistribution::input_part, OptionBase::buildoption,
      "The x in p(y|x), if the density is conditional.");

  declareOption(ol, "n_curve_points", &PDistribution::n_curve_points, OptionBase::buildoption,
      "The number of points for which the distribution is evaluated when outputs_defs\n"
      "is upper case (produce a histogram).\n"
      "The lower_bound and upper_bound options specify where the curve begins and ends.\n"
      "Note that these options (upper case letters) only work for SCALAR variables."
      );

  declareOption(ol, "lower_bound",  &PDistribution::lower_bound, OptionBase::buildoption,
      "The lower bound of scalar Y values to compute a histogram of the distribution\n"
      "when upper case outputs_def are specified.\n");

  declareOption(ol, "upper_bound",  &PDistribution::upper_bound, OptionBase::buildoption,
      "The upper bound of scalar Y values to compute a histogram of the distribution\n"
      "when upper case outputs_def are specified.\n");

  // Learnt options.

  declareOption(ol, "cond_sort",  &PDistribution::cond_sort, OptionBase::learntoption,
      "A vector containing the indices of the variables, so that they are ordered like\n"
      "this: input, target, margin.");

  declareOption(ol, "n_margin",  &PDistribution::n_margin, OptionBase::learntoption,
      "The size of the variables that are marginalized in p(y|x). E.g., if the whole\n"
      "input contains (x,y,z), and we want to compute p(y|x), then n_margin = z.length().");
      
  declareOption(ol, "n_input",  &PDistribution::n_input, OptionBase::learntoption,
      "The size of the input x in p(y|x).");

  declareOption(ol, "n_target",  &PDistribution::n_target, OptionBase::learntoption,
      "The size of the target y in p(y|x).");
      
  // Now call the parent class' declareOptions
  inherited::declareOptions(ol);
}

///////////
// build //
///////////
void PDistribution::build()
{
  inherited::build();
  build_();
}

////////////
// build_ //
////////////
void PDistribution::build_()
{
  if (n_curve_points > 0) {
    delta_curve = (upper_bound - lower_bound) / real(n_curve_points);
  }
  resizeParts();
  // Preccompute the stuff associated to the conditional flags.
  setConditionalFlags(conditional_flags);
  // Set the input part for a conditional distribution.
  setInput(input_part);
}

///////////////////
// computeOutput //
///////////////////
void PDistribution::computeOutput(const Vec& input, Vec& output) const
{
  static Vec expect;
  static Mat cov;
  static int k,l;
  static Vec y;
  need_set_input = splitCond(input, input_part, target_part);
  if (need_set_input) {
    // There is an input part, and it is not the same as in the previous call.
    setInput(input_part);
  }
  l = (int) outputs_def.length();
  k = 0;
  for(int i=0; i<l; i++)
  {
    switch(outputs_def[i])
    {
      case 'l':
        output[k++] = log_density(target_part);
        break;
      case 'd':
        output[k++] = density(target_part);
        break;
      case 'c':
        output[k++] = cdf(target_part);
        break;
      case 's':
        output[k++] = survival_fn(target_part);
        break;
      case 'e':
        expect = output.subVec(k, n_target);
        expectation(expect);
        k += n_target;
        break;
      case 'v':
        cov = output.subVec(k, square(n_target)).toMat(n_target, n_target);
        variance(cov);
        k += square(n_target);
        break;
      case 'E':
      case 'V':
        if (n_target > 1)
          PLERROR("In PDistribution::computeOutput - Can only plot histogram of expectation or variance for one-dimensional target");
      case 'L':
      case 'D':
      case 'C':
      case 'S':
        real t;
        y.resize(1);
        y[0] = lower_bound;
        for (int j = 0; j < n_curve_points; j++) {
          switch(outputs_def[i]) {
            case 'L':
              t = log_density(y);
              break;
            case 'D':
              t = density(y);
              break;
            case 'C':
              t = cdf(y);
              break;
            case 'S':
              t = survival_fn(y);
              break;
            case 'E':
              setInput(y);
              expectation(expect);
              t = expect[0];
              break;
            case 'V':
              setInput(y);
              cov = expect.toMat(1,1);
              variance(cov);
              t = expect[0];
              break;
            default:
              PLERROR("In PDistribution::computeOutput - This should never happen");
          }
          output[j + k] = t;
          y[0] += delta_curve;
        }
        k += n_curve_points;
        break;
      default:
        PLERROR("In PDistribution::computeOutput - Unrecognized outputs_def character");
    }
  }
}

/////////////////////////////
// computeCostsFromOutputs //
/////////////////////////////
void PDistribution::computeCostsFromOutputs(const Vec& input, const Vec& output, 
    const Vec& target, Vec& costs) const
{
  if(outputs_def[0] != 'l')
    PLERROR("In PDistribution::computeCostsFromOutputs currently can only 'compute' \n"
        "negative log likelihood from log likelihood returned as first output \n");
  costs.resize(1);
  costs[0] = -output[0];
}                                

/////////////////////////////////
// ensureFullJointDistribution //
/////////////////////////////////
bool PDistribution::ensureFullJointDistribution(TVec<int>& old_flags) {
  bool restore_flags = false;
  if (!full_joint_distribution) {
    // Backup flags.
    restore_flags = true;
    old_flags.resize(conditional_flags.length());
    old_flags << conditional_flags;
    // Set flags to compute the full joint distribution.
    setConditionalFlags(TVec<int>());
  } else {
    old_flags.resize(0);
  }
  return restore_flags;
}

//////////////////////
// getTestCostNames //
//////////////////////
TVec<string> PDistribution::getTestCostNames() const
{
  return TVec<string>(1,"NLL");
}

///////////////////////
// getTrainCostNames //
///////////////////////
TVec<string> PDistribution::getTrainCostNames() const
{
  // Default = no train cost computed. This may be overridden in subclasses.
  TVec<string> c;
  return c;
}

///////////////
// generateN //
///////////////
void PDistribution::generateN(const Mat& X) const
{
  if(X.width()!=inputsize())
    PLERROR("In PDistribution::generateN  matrix width (%d) differs from inputsize() (%d)", X.width(), inputsize());
  int N = X.length();  
  for(int i=0; i<N; i++)
  {
    Vec v = X(i);
    generate(v);
  }
}

/////////////////////////////////
// makeDeepCopyFromShallowCopy //
/////////////////////////////////
void PDistribution::makeDeepCopyFromShallowCopy(map<const void*, void*>& copies)
{
  inherited::makeDeepCopyFromShallowCopy(copies);
  PLERROR("In PDistribution::makeDeepCopyFromShallowCopy - Make sure the implementation is correct");
}

////////////////
// outputsize //
////////////////
int PDistribution::outputsize() const
{
  int l = 0;
  for (unsigned int i=0; i<outputs_def.length(); i++) {
    if (outputs_def[i]=='L' || outputs_def[i]=='D' || outputs_def[i]=='C' || outputs_def[i]=='S'
        || outputs_def[i]=='E' || outputs_def[i]=='V')
      l+=n_curve_points;
    else if (outputs_def[i]=='e')
      l += n_target;
    else if (outputs_def[i]=='v') // by default assume variance is full nxn matrix 
      l += n_target * n_target;
    else l++;
  }
  return l;
}

/////////////////
// resizeParts //
/////////////////
void PDistribution::resizeParts() {
  input_part.resize(n_input);
  target_part.resize(n_target);
}

/////////////////////////
// setConditionalFlags //
/////////////////////////
void PDistribution::setConditionalFlags(TVec<int> flags) {
  static TVec<int> input;
  static TVec<int> target;
  static TVec<int> margin;
  if (inputsize_ <= 0) {
    // No dataset has been specified yet.
    return;
  }
  int is = inputsize();
  input.resize(0);
  target.resize(0);
  margin.resize(0);
  if (flags.isEmpty()) {
    // No flags: everything is target.
    for (int i = 0; i < is; i++) {
      target.append(i);
    }
  } else {
    for (int i = 0; i < flags.length(); i++) {
      switch (flags[i]) {
        case 0:
          margin.append(i);
          break;
        case 1:
          input.append(i);
          break;
        case 2:
          target.append(i);
          break;
        default:
          PLERROR("In PDistribution::setConditionalFlags - Unknown flag value");
      }
    }
  }
  // Update the sizes.
  n_input = input.length();
  n_target = target.length();
  n_margin = margin.length();
  resizeParts();
  if (n_input == 0 && n_margin == 0) {
    // Only full joint distribution.
    full_joint_distribution = true;
  } else {
    full_joint_distribution = false;
  }
  // Fill the new vector of sorted indices.
  TVec<int> new_cond_sort(is);
  new_cond_sort.subVec(0, n_input) << input;
  new_cond_sort.subVec(n_input, n_target) << target;
  new_cond_sort.subVec(n_input + n_target, n_margin) << margin;
  // Check whether we are in the 'easy' case where input, target and margin
  // are correctly sorted.
  if ((n_input == 0 || max(input) == n_input - 1) &&
      (n_target == 0 || max(target) == n_target + n_input - 1)) {
    already_sorted = true;
  } else {
    already_sorted = false;
  }
  // Deduce the indices to be swapped compared to the previous sorting.
  bool found;
  int j;
  int index;
  cond_swap.resize(0);
  if (cond_sort.length() != is) {
    // The previous cond_sort is not valid anymore, we probably
    // have a new training set.
    cond_sort = TVec<int>(0, is - 1, 1);
  }
  for (int i = 0; i < is; i++) {
    found = false;
    j = 0;
    index = new_cond_sort[i];
    while (!found) {
      if (cond_sort[j] == index) {
        found = true;
        if (i != j) {
          // There is really a need to swap the indices.
          cond_swap.append(i);
          cond_swap.append(j);
        }
      } else {
        j++;
      }
    }
  }
  // Copy the new vector of sorted indices.
  cond_sort << new_cond_sort;
  // Copy the new flags.
  conditional_flags.resize(flags.length());
  conditional_flags << flags;
  // And call the method that updates the internal Vec and Mat given the new
  // sorting (this method should be written in subclasses).
  updateFromConditionalSorting();
}

//////////////
// setInput //
//////////////
void PDistribution::setInput(const Vec& input) const {
  // Default behavior = nothing to do.
}

////////////////////
// setTrainingSet //
////////////////////
void PDistribution::setTrainingSet(VMat training_set, bool call_forget) {
  inherited::setTrainingSet(training_set, call_forget);
  // Update internal data according to conditional_flags.
  setConditionalFlags(conditional_flags);
}

///////////////////
// sortFromFlags //
///////////////////
void PDistribution::sortFromFlags(Vec& v) {
  static Vec tmp_copy;
  tmp_copy.resize(v.length());
  tmp_copy << v;
  for (int i = 0; i < cond_swap.length();) {
    v[cond_swap[i++]] = tmp_copy[cond_swap[i++]];
  }
}

void PDistribution::sortFromFlags(Mat& m, bool sort_columns, bool sort_rows) {
  static int j,k;
  static Mat tmp_copy;
  static Vec row;
  if (sort_columns) {
    for (int r = 0; r < m.length(); r++) {
      row = m(r);
      sortFromFlags(row);
    }
  }
  if (sort_rows && m.length() > 0 && m.width() > 0) {
    tmp_copy.resize(m.length(), m.width());
    tmp_copy << m;
    for (int i = 0; i < cond_swap.length();) {
      j = cond_swap[i++];
      k = cond_swap[i++];
      // The new j-th row is the old k-th row.
      m(j) << tmp_copy(k);
    }
  }
}

///////////////
// splitCond //
///////////////
bool PDistribution::splitCond(const Vec& input, Vec& input_part, Vec& target_part) const {
  if (n_input > 0 && input.length() == n_target + n_margin) {
    // No input part provided: this means this is the same as before.
    if (already_sorted) {
      target_part << input.subVec(0, n_target);
    } else {
      // A bit messy here. It probably won't happen, so it's not implemented
      // for now (but wouldn't be that hard to do it).
      PLERROR("In PDistribution::splitCond - You'll need to implement this case!");
    }
    return false;
  }
  if (already_sorted) {
    input_part << input.subVec(0, n_input);
    target_part << input.subVec(n_input, n_target);
  } else {
    for (int i = 0; i < n_input; i++) {
      input_part[i] = input[cond_sort[i]];
    }
    for (int i = 0; i < n_target; i++) {
      target_part[i] = input[cond_sort[i + n_input]];
    }
  }
  return true;
}

////////////////////
// subclass stuff //
////////////////////

void PDistribution::forget() {
  PLERROR("forget not implemented for this PDistribution");
}

real PDistribution::log_density(const Vec& x) const
{ PLERROR("density not implemented for this PDistribution"); return 0; }

real PDistribution::density(const Vec& x) const
{ return exp(log_density(x)); }

real PDistribution::survival_fn(const Vec& x) const
{ PLERROR("survival_fn not implemented for this PDistribution"); return 0; }

real PDistribution::cdf(const Vec& x) const
{ PLERROR("cdf not implemented for this PDistribution"); return 0; }

void PDistribution::expectation(Vec& mu) const
{ PLERROR("expectation not implemented for this PDistribution"); }

void PDistribution::variance(Mat& covar) const
{ PLERROR("variance not implemented for this PDistribution"); }

void PDistribution::resetGenerator(long g_seed) const
{ PLERROR("resetGenerator not implemented for this PDistribution"); }

void PDistribution::generate(Vec& x) const
{ PLERROR("generate not implemented for this PDistribution"); }

void PDistribution::train()
{ PLERROR("train not implemented for this PDistribution"); }

//////////////////////////////////
// updateFromConditionalSorting //
//////////////////////////////////
void PDistribution::updateFromConditionalSorting() {
  // Default does nothing.
  return;
}

} // end of namespace PLearn
