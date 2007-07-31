// -*- C++ -*-

// RegressionTreeLeave.cc
// Copyright (c) 1998-2002 Pascal Vincent
// Copyright (C) 1999-2002 Yoshua Bengio and University of Montreal
// Copyright (c) 2002 Jean-Sebastien Senecal, Xavier Saint-Mleux, Rejean Ducharme
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


/* ********************************************************************************    
 * $Id: RegressionTreeLeave.cc, v 1.0 2004/07/19 10:00:00 Bengio/Kegl/Godbout    *
 * This file is part of the PLearn library.                                     *
 ******************************************************************************** */

#include "RegressionTreeLeave.h"

namespace PLearn {
using namespace std;

PLEARN_IMPLEMENT_OBJECT(RegressionTreeLeave,
                        "Object to represent the leaves of a regression tree.",
                        "It maintains the necessary statistics to compute the output and the train error\n"
                        "of the samples in the leave.\n"
    );

RegressionTreeLeave::RegressionTreeLeave()
{
    build();
}

RegressionTreeLeave::~RegressionTreeLeave()
{
}

void RegressionTreeLeave::declareOptions(OptionList& ol)
{ 
    declareOption(ol, "id", &RegressionTreeLeave::id, OptionBase::buildoption,
                  "The id of this leave to register the rows of the RegressionTreeRegisters\n");
    declareOption(ol, "missing_leave", &RegressionTreeLeave::missing_leave, OptionBase::buildoption,
                  "The indicator that it is a leave with missing values for the split feature\n");
    declareOption(ol, "loss_function_weight", &RegressionTreeLeave::loss_function_weight, OptionBase::buildoption,
                  "The hyper parameter to balance the error and the confidence factor\n");
    declareOption(ol, "verbosity", &RegressionTreeLeave::verbosity, OptionBase::buildoption,
                  "The desired level of verbosity\n");
    declareOption(ol, "train_set", &RegressionTreeLeave::train_set, OptionBase::buildoption,
                  "The train set with the sorted row index matrix and the leave id vector\n");
      
    declareOption(ol, "length", &RegressionTreeLeave::length, OptionBase::learntoption,
                  "The number of rows in this leave\n");
    declareOption(ol, "output", &RegressionTreeLeave::output, OptionBase::learntoption,
                  "The regression output for this leave\n");
    declareOption(ol, "error", &RegressionTreeLeave::error, OptionBase::learntoption,
                  "The error on this leave\n");
    declareOption(ol, "weights_sum", &RegressionTreeLeave::weights_sum, OptionBase::learntoption,
                  "The sum of weights for the samples in this leave\n");
    declareOption(ol, "targets_sum", &RegressionTreeLeave::targets_sum, OptionBase::learntoption,
                  "The sum of targets for the samples in this leave\n");
    declareOption(ol, "squared_targets_sum", &RegressionTreeLeave::squared_targets_sum, OptionBase::learntoption,
                  "The sum of squared target values for the sample in this leave\n");
    declareOption(ol, "weighted_targets_sum", &RegressionTreeLeave::weighted_targets_sum, OptionBase::learntoption,
                  "The sum of weighted targets for the samples in this leave\n");
    declareOption(ol, "weighted_squared_targets_sum", &RegressionTreeLeave::weighted_squared_targets_sum, OptionBase::learntoption,
                  "The sum of squared weighted target values for the samples in this leave\n");
    declareOption(ol, "loss_function_factor", &RegressionTreeLeave::loss_function_factor, OptionBase::learntoption,
                  "2 / pow(loss_function_weight, 2.0).\n");
    inherited::declareOptions(ol);
}

void RegressionTreeLeave::makeDeepCopyFromShallowCopy(CopiesMap& copies)
{
    inherited::makeDeepCopyFromShallowCopy(copies);
    deepCopyField(id, copies);
    deepCopyField(missing_leave, copies);
    deepCopyField(loss_function_weight, copies);
    deepCopyField(verbosity, copies);
    deepCopyField(train_set, copies);
    deepCopyField(length, copies);
    deepCopyField(output, copies);
    deepCopyField(error, copies);
    deepCopyField(weights_sum, copies);
    deepCopyField(targets_sum, copies);
    deepCopyField(squared_targets_sum, copies);
    deepCopyField(weighted_targets_sum, copies);
    deepCopyField(weighted_squared_targets_sum, copies);
    deepCopyField(loss_function_factor, copies);
}

void RegressionTreeLeave::build()
{
    inherited::build();
    build_();
}

void RegressionTreeLeave::build_()
{
}

void RegressionTreeLeave::initLeave(PP<RegressionTreeRegisters> the_train_set)
{
    train_set = the_train_set;
}

void RegressionTreeLeave::initStats()
{
    length = 0;
    output.resize(2);
    output[0] = 0.0;
    output[1] = 0.0;
    error.resize(3);
    error[0]  = 0.0;
    error[1]  = 0.0;
    error[2] = 0.0;
    weights_sum= 0.0;
    targets_sum = 0.0;
    squared_targets_sum = 0.0;
    weighted_targets_sum = 0.0;
    weighted_squared_targets_sum = 0.0; 
    if (loss_function_weight != 0.0) loss_function_factor = 2.0 / pow(loss_function_weight, 2);
    else loss_function_factor = 1.0;
}

void RegressionTreeLeave::addRow(int row, Vec outputv, Vec errorv)
{
    weight = train_set->getWeight(row);
    target = train_set->getTarget(row);
    length += 1;
    weights_sum += weight;
    targets_sum += target;
    squared_target = pow(target, 2);
    squared_targets_sum += squared_target;
    weighted_targets_sum += weight * target;
    weighted_squared_targets_sum += weight * squared_target;  
    computeOutputAndError();
    getOutput(outputv);
    getError(errorv);
    train_set->applyForRow(id, row);
}

void RegressionTreeLeave::removeRow(int row, Vec outputv, Vec errorv)
{
    weight = train_set->getWeight(row);
    target = train_set->getTarget(row);
    length -= 1;
    weights_sum -= weight;
    targets_sum -= target;
    squared_target = pow(target, 2);
    squared_targets_sum -= squared_target;
    weighted_targets_sum -= weight * target;
    weighted_squared_targets_sum -= weight * squared_target; 
    computeOutputAndError();
    getOutput(outputv);
    getError(errorv); 
}

void RegressionTreeLeave::computeOutputAndError()
{
    output[0] = weighted_targets_sum / weights_sum;
    if (missing_leave >= 1)
    {
        output[1] = 0.0;
        error[0] = 0.0;
        error[1] = weights_sum;
        error[2] = 0.0;
    }
    else
    {
        output[1] = 1.0;
        error[0] = ((weights_sum * output[0] * output[0]) - (2.0 * weighted_targets_sum * output[0]) + weighted_squared_targets_sum) * loss_function_factor;
        if (error[0] < 1E-10) error[0] = 0.0;
        error[1] = 0.0;
        if (error[0] > weights_sum * loss_function_factor) error[2] = weights_sum * loss_function_factor; else error[2] = error[0];
    }
}

void RegressionTreeLeave::registerRow(int row)
{
    train_set->registerLeave(id, row);
}

int RegressionTreeLeave::getId()
{
    return id;
}

int RegressionTreeLeave::getLength()
{
    return length;
}

void RegressionTreeLeave::getError(Vec errorv)
{
    errorv[0] = error[0];
    errorv[1] = error[1];  
    errorv[2] = error[2]; 
}

void RegressionTreeLeave::getOutput(Vec outputv)
{
    outputv[0] = output[0];
    outputv[1] = output[1];
}

void RegressionTreeLeave::printStats()
{
    cout << " l " << length;
    cout << " o0 " << output[0];
    cout << " o1 " << output[1];
    cout << " e0 " << error[0];
    cout << " e1 " << error[1];
    cout << " ws " << weights_sum;
    cout << " ts " << targets_sum;
    cout << " sts " << squared_targets_sum;
    cout << " wts " << weighted_targets_sum;
    cout << " wsts " << weighted_squared_targets_sum; 
    cout << endl;
}

void RegressionTreeLeave::verbose(string the_msg, int the_level)
{
    if (verbosity >= the_level)
        cout << the_msg << endl;
}

} // end of namespace PLearn


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
