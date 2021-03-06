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
#include "RegressionTreeRegisters.h"

namespace PLearn {
using namespace std;

PLEARN_IMPLEMENT_OBJECT(RegressionTreeLeave,
                        "Object to represent the leaves of a regression tree.",
                        "It maintains the necessary statistics to compute the output and the train error\n"
                        "of the samples in the leave.\n"
    );

int RegressionTreeLeave::verbosity = 0;
Vec RegressionTreeLeave::dummy_vec;
bool RegressionTreeLeave::output_confidence_target = false;

RegressionTreeLeave::RegressionTreeLeave():
    missing_leave(false),
    loss_function_weight(0),
    id(0),
    length_(0),
    weights_sum(0),
    targets_sum(0),
    weighted_targets_sum(0),
    weighted_squared_targets_sum(0),
    loss_function_factor(1)
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
    declareStaticOption(ol, "verbosity", &RegressionTreeLeave::verbosity, OptionBase::buildoption,
                  "The desired level of verbosity\n");
    declareOption(ol, "train_set", &RegressionTreeLeave::train_set, 
                  OptionBase::buildoption | OptionBase::nosave,
                  "The train set with the sorted row index matrix and the leave id vector\n");
    declareOption(ol, "length", &RegressionTreeLeave::length_, OptionBase::learntoption,
                  "The number of rows in this leave\n");
    declareOption(ol, "weights_sum", &RegressionTreeLeave::weights_sum, OptionBase::learntoption,
                  "The sum of weights for the samples in this leave\n");
    declareOption(ol, "targets_sum", &RegressionTreeLeave::targets_sum, OptionBase::learntoption,
                  "The sum of targets for the samples in this leave\n");
    declareOption(ol, "weighted_targets_sum", &RegressionTreeLeave::weighted_targets_sum, OptionBase::learntoption,
                  "The sum of weighted targets for the samples in this leave\n");
    declareOption(ol, "weighted_squared_targets_sum", &RegressionTreeLeave::weighted_squared_targets_sum, OptionBase::learntoption,
                  "The sum of squared weighted target values for the samples in this leave\n");
    declareOption(ol, "loss_function_factor", &RegressionTreeLeave::loss_function_factor, OptionBase::learntoption,
                  "2 / pow(loss_function_weight, 2.0).\n");

    declareStaticOption(ol, "output_confidence_target",
                  &RegressionTreeLeave::output_confidence_target,
                  OptionBase::buildoption,
                  "If false the output size is 1 and contain only the predicted"
                  " target. Else output size is 2 and contain also the"
                  " confidence\n");

    declareStaticOption(ol, "output", &RegressionTreeLeave::dummy_vec, OptionBase::nosave,
                  "DEPRECATED");
    declareStaticOption(ol, "error", &RegressionTreeLeave::dummy_vec, OptionBase::nosave,
                  "DEPRECATED");

    inherited::declareOptions(ol);
}

void RegressionTreeLeave::makeDeepCopyFromShallowCopy(CopiesMap& copies)
{
    inherited::makeDeepCopyFromShallowCopy(copies);
    //we don't deep copy it as we don't modify it 
    //and this is a link to the RegressionTree train_set
//    deepCopyField(train_set, copies);
}

void RegressionTreeLeave::build()
{
    inherited::build();
    build_();
}

void RegressionTreeLeave::build_()
{
}

void RegressionTreeLeave::initLeave(PP<RegressionTreeRegisters> the_train_set, RTR_type_id the_id, bool the_missing_leave)
{
    train_set = the_train_set;
    id = the_id;
    missing_leave = the_missing_leave;
}

void RegressionTreeLeave::initStats()
{
    length_ = 0;
    weights_sum= 0.0;
    targets_sum = 0.0;
    weighted_targets_sum = 0.0;
    weighted_squared_targets_sum = 0.0; 
    if (loss_function_weight != 0.0) 
        loss_function_factor = 2.0 / pow(loss_function_weight, 2);
    else loss_function_factor = 1.0;
}

void RegressionTreeLeave::addRow(int row, real target, real weight)
{
    length_ += 1;
    weights_sum += weight;
    targets_sum += target;
    real squared_target = pow(target, 2);
    weighted_targets_sum += weight * target;
    weighted_squared_targets_sum += weight * squared_target;  
}

void RegressionTreeLeave::addRow(int row)
{
    real weight = train_set->getWeight(row);
    real target = train_set->getTarget(row);
    addRow(row, target, weight);
}

void RegressionTreeLeave::addRow(int row, real target, real weight,
                                 Vec outputv, Vec errorv)
{
    addRow(row, target, weight);
    getOutputAndError(outputv,errorv);
}

void RegressionTreeLeave::addRow(int row, Vec outputv, Vec errorv)
{
    addRow(row);
    getOutputAndError(outputv,errorv);
}
void RegressionTreeLeave::removeRow(int row, Vec output, Vec error)
{
    real weight = train_set->getWeight(row);
    real target = train_set->getTarget(row);
    removeRow(row, target, weight, output, error);
}
void RegressionTreeLeave::removeRow(int row, real target, real weight)
{
    length_ -= 1;
    weights_sum -= weight;
    targets_sum -= target;
    real squared_target = pow(target, 2);
    weighted_targets_sum -= weight * target;
    weighted_squared_targets_sum -= weight * squared_target; 
}
void RegressionTreeLeave::removeRow(int row, real target, real weight,
                                    Vec outputv, Vec errorv)
{
    removeRow(row,target,weight);
    getOutputAndError(outputv, errorv);
}

void RegressionTreeLeave::getOutputAndError(Vec& output, Vec& error)const
{
    real conf = 0;
    if(length_>0){
        output[0] = weighted_targets_sum / weights_sum;
        if (!RTR_HAVE_MISSING || missing_leave != true)
        {
            //we put the most frequent case first as an optimisation
            conf = 1.0;
            error[0] = ((weights_sum * output[0] * output[0]) - 
                        (2.0 * weighted_targets_sum * output[0]) + weighted_squared_targets_sum)
                * loss_function_factor;
            if (error[0] < 1E-10) {error[0] = 0.0;} //PLWARNING("E[0] <1e-10: %f",error[0]);}
            error[1] = 0.0;
            real weights_sum_factor  = weights_sum * loss_function_factor;
            if (error[0] > weights_sum_factor) error[2] = weights_sum_factor;
            else error[2] = error[0];
        }
        else
        {
            error[0] = 0.0;
            error[1] = weights_sum;
            error[2] = 0.0;
        }
    }else{
        output[0] = MISSING_VALUE;
        error.clear();
    }
    if(output_confidence_target) output[1] = conf;
}

TVec<string> RegressionTreeLeave::getOutputNames() const
{
    TVec<string> ret;
    ret.append("val_pred");
    if(output_confidence_target)
        ret.append("confidence");
    return ret;
}

void RegressionTreeLeave::printStats()
{
    cout << " l " << length_;
    Vec output(2);
    Vec error(3);
    getOutputAndError(output,error);
    cout << " o0 " << output[0];
    if(output_confidence_target)
        cout << " o1 " << output[1];
    cout << " e0 " << error[0];
    cout << " e1 " << error[1];
    cout << " ws " << weights_sum;
    cout << " ts " << targets_sum;
    cout << " wts " << weighted_targets_sum;
    cout << " wsts " << weighted_squared_targets_sum;
    cout << " wts/ws " <<weighted_targets_sum/weights_sum;
    cout << " wsts/ws "<<weighted_squared_targets_sum/weights_sum;
    cout << " sqrt(wsts/ws) "<<sqrt(weighted_squared_targets_sum/weights_sum);
    cout << endl;
}
bool RegressionTreeLeave::uniqTarget(){
    if(classname()=="RegressionTreeLeave"){
        real wts_w = weighted_targets_sum/weights_sum;
        real wsts_w= sqrt(weighted_squared_targets_sum/weights_sum);
        return fast_is_equal(wts_w,wsts_w);
    }else
        PLERROR("In RegressionTreeLeave::uniqTarget subclass must reimplement it.");
}

void RegressionTreeLeave::addLeave(PP<RegressionTreeLeave> leave){
    if(leave->classname()=="RegressionTreeLeave" && classname()=="RegressionTreeLeave"){
        length_ += leave->length_;
        weights_sum += leave->weights_sum;
        targets_sum += leave->targets_sum;
        weighted_targets_sum += leave->weighted_targets_sum;
        weighted_squared_targets_sum += leave->weighted_squared_targets_sum;        
    }else
        PLERROR("In RegressionTreeLeave::addLeave subclass %s or %s must reimplement it.",
                classname().c_str(), leave->classname().c_str());
}

void RegressionTreeLeave::removeLeave(PP<RegressionTreeLeave> leave){
    if(leave->classname()=="RegressionTreeLeave" && classname()=="RegressionTreeLeave"){
        length_ -= leave->length_;
        weights_sum -= leave->weights_sum;
        targets_sum -= leave->targets_sum;
        weighted_targets_sum -= leave->weighted_targets_sum;
        weighted_squared_targets_sum -= leave->weighted_squared_targets_sum;
    }else
        PLERROR("In RegressionTreeLeave::removeLeave subclass %s or %s must reimplement it.",
                classname().c_str(), leave->classname().c_str());
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
