// -*- C++ -*-

// PLearnerOutputVMatrix.cc
//
// Copyright (C) 2003 Yoshua Bengio 
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
   * $Id: PLearnerOutputVMatrix.cc,v 1.9 2004/03/30 20:09:05 tihocan Exp $
   ******************************************************* */

// Authors: Yoshua Bengio

/*! \file PLearnerOutputVMatrix.cc */


#include "PLearnerOutputVMatrix.h"

namespace PLearn {
using namespace std;


PLearnerOutputVMatrix::PLearnerOutputVMatrix()
 :inherited(),
  put_raw_input(false),
  train_learners(false)
  /* ### Initialise all fields to their default value */
{
}
PLearnerOutputVMatrix::PLearnerOutputVMatrix
(VMat data_,TVec<PP<PLearner> > learners_, bool put_raw_input_) 
  : data(data_),learners(learners_),put_raw_input(put_raw_input_)
{
  build();
}

PLEARN_IMPLEMENT_OBJECT(PLearnerOutputVMatrix, 
                        "Use a PLearner (or a set of them) to transform the input part of a data set into the learners outputs",
                        "The input part of this VMatrix is obtained from the input part an original data set on which\n"
                        "one or more PLearner's computeOutput method is applied. The other columns of the original data set\n"
                        "are copied as is. Optionally, the raw input can be copied as well\n"
                        "always in the input part of the new VMatrix. The order of the elements of a new row is as follows:\n"
                        "  - the outputs of the learners (concatenated) when applied on the input part of the original data,\n"
                        "  - optionally, the raw input part of the original data,\n"
                        "  - all the non-input columns of the original data.");

void PLearnerOutputVMatrix::getRow(int i, Vec v) const
{
  int c=0;
  if (learners_need_train) {
    // We need to train the learners first.
    for (int i = 0; i < learners.length(); i++) {
      learners[i]->train();
    }
    learners_need_train = false;
  }
  data->getRow(i,row);
  for (int j=0;j<learners.length();j++)
  {
    Vec out_j = learners_output(j);
    learners[j]->computeOutput(learner_input,out_j);
  }
  v.subVec(0,c=learners_output.size()) << learners_output.toVec();
  if (put_raw_input)
  {
    v.subVec(c,learner_input->length()) << learner_input;
    c+=learner_input->length();
  }
  v.subVec(c,non_input_part_of_data_row.length()) << non_input_part_of_data_row;
}

void PLearnerOutputVMatrix::declareOptions(OptionList& ol)
{
  // ### Declare all of this object's options here
  // ### For the "flags" of each option, you should typically specify  
  // ### one of OptionBase::buildoption, OptionBase::learntoption or 
  // ### OptionBase::tuningoption. Another possible flag to be combined with
  // ### is OptionBase::nosave

   declareOption(ol, "data", &PLearnerOutputVMatrix::data, OptionBase::buildoption,
                 "The original data set (a VMat)");

   declareOption(ol, "learners", &PLearnerOutputVMatrix::learners, OptionBase::buildoption,
                 "The vector of PLearners which will be applied to the data set");

   declareOption(ol, "put_raw_input", &PLearnerOutputVMatrix::put_raw_input, OptionBase::buildoption,
                 "Whether to include in the input part of this VMatrix the raw data input part");

   declareOption(ol, "train_learners", &PLearnerOutputVMatrix::train_learners, OptionBase::buildoption,
                "If set to 1, the learners will be train on 'data' before computing the output");

  // Now call the parent class' declareOptions
  inherited::declareOptions(ol);
}

void PLearnerOutputVMatrix::build_()
{
  if (data && learners.length()>0 && learners[0])
  {
    if (train_learners) {
      // Set the learners' training set.
      for (int i = 0; i < learners.length(); i++) {
        learners[i]->setTrainingSet(data);
      }
      // Note that the learners will be train only if we actually call getRow().
    }
    learners_need_train = train_learners;
    row.resize(data->width());
    learner_input = row.subVec(0,data->inputsize());
    learner_target = row.subVec(data->inputsize(),data->targetsize());
    non_input_part_of_data_row = row.subVec(data->inputsize(),data->width()-data->inputsize());
    learners_output.resize(learners->length(),learners[0]->outputsize());
    inputsize_ = 0;
    for (int i=0;i<learners->length();i++)
      inputsize_ += learners[i]->outputsize();
    if (put_raw_input) 
      inputsize_ += data->inputsize();
    targetsize_ = data->targetsize();
    weightsize_ = data->weightsize();
    length_ = data->length();
    width_ = data->width() - data->inputsize() + inputsize_;

    // Set field info.
    fieldinfos.resize(width_);
    if (data->getFieldInfos().size() >= data->inputsize() + data->targetsize()) {
      // We can retrieve the information for the target columns.
      for (int i = 0; i < data->targetsize(); i++) {
        fieldinfos[i + this->inputsize()] = data->getFieldInfos()[i + data->inputsize()];
      }
    }
  }
}

// ### Nothing to add here, simply calls build_
void PLearnerOutputVMatrix::build()
{
  inherited::build();
  build_();
}

void PLearnerOutputVMatrix::makeDeepCopyFromShallowCopy(map<const void*, void*>& copies)
{
  inherited::makeDeepCopyFromShallowCopy(copies);

  deepCopyField(row, copies);
  deepCopyField(learner_input, copies);
  deepCopyField(learners_output, copies);
  deepCopyField(learner_target, copies);
  deepCopyField(non_input_part_of_data_row, copies);
}

} // end of namespace PLearn

