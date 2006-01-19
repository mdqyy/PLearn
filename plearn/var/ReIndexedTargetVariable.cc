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
 * $Id: ReIndexedTargetVariable.cc 3994 2005-08-25 13:35:03Z chapados $
 * This file is part of the PLearn library.
 ******************************************************* */

#include "ReIndexedTargetVariable.h"
#include "ColumnIndexVariable.h"

namespace PLearn {
using namespace std;


/** ReIndexedTargetVariable **/

PLEARN_IMPLEMENT_OBJECT(ReIndexedTargetVariable,
                        "ONE LINE DESCR",
                        "NO HELP");


ReIndexedTargetVariable::ReIndexedTargetVariable()
{ }
  
ReIndexedTargetVariable::ReIndexedTargetVariable(Variable* input1, Variable* input2, VMat the_source, TVec<int> the_target_cols)
    : inherited(input1, input2, input1->length(), input1->width()), source(the_source), target_cols(the_target_cols)
{
    build_();
}

ReIndexedTargetVariable::ReIndexedTargetVariable(Variable* input1, Variable* input2, PP<Dictionary> the_dict, TVec<int> the_target_cols)
    : inherited(input1, input2, input1->length(), input1->width()), dict(the_dict), target_cols(the_target_cols)
{
    build_();
}

void
ReIndexedTargetVariable::build()
{
    inherited::build();
    build_();
}

void
ReIndexedTargetVariable::build_()
{
    if (input1 && input2 && (source || dict)) {
        if(!input1->isVec() || !input2->isVec())
            PLERROR("In ReIndexedTargetVariable: input1 or input2 must be a vector var");
        if(input1->size() != target_cols.length())
            PLERROR("In ReIndexedTargetVariable: input1 should be of size target_cols.length()");        
        if(source)
        {
            if(input2->size() != source->inputsize())
            PLERROR("In ReIndexedTargetVariable: input2 should be of size source->inputsize()");

            row.resize(source->width());
            row.subVec(source->inputsize(),source->width()-source->inputsize()).fill(MISSING_VALUE);
        }
    }
}

void
ReIndexedTargetVariable::declareOptions(OptionList &ol)
{
    declareOption(ol, "source", &ReIndexedTargetVariable::source, OptionBase::buildoption, "");
    declareOption(ol, "dict", &ReIndexedTargetVariable::dict, OptionBase::buildoption, "");
    declareOption(ol, "target_cols", &ReIndexedTargetVariable::target_cols, OptionBase::buildoption, "");
    inherited::declareOptions(ol);
}

void ReIndexedTargetVariable::recomputeSize(int& l, int& w) const
{ l=input1->length(); w=input1->width(); }


void ReIndexedTargetVariable::fprop()
{
    if(source) row.subVec(0,source->inputsize()) << input2->value;
    
    for (int j=0; j<input1->size(); j++)
    {          
        if(source)
            valuedata[j] = source->getValues(row,target_cols[j]).find((int)input1->valuedata[j]);
        else
            valuedata[j] = dict->getValues().find((int)input1->valuedata[j]);
        
        if(valuedata[j] < 0) PLERROR("In ReIndexedTargetVariable::fprop(): target %d is not among possible values",input1->valuedata[j]);
    }
}


void ReIndexedTargetVariable::bprop()
{}


void ReIndexedTargetVariable::symbolicBprop()
{
    PLERROR("In ReIndexedTargetVariable::symbolicBprop(): not implemented");
}


void ReIndexedTargetVariable::rfprop()
{
    PLERROR("In ReIndexedTargetVariable::rfprop(): not implemented.");
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
