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
 * $Id$
 * This file is part of the PLearn library.
 ******************************************************* */

#include "MinusColumnVariable.h"
#include "RowSumVariable.h"
#include "Var_operators.h"

namespace PLearn {
using namespace std;


/** MinusColumnVariable **/

PLEARN_IMPLEMENT_OBJECT(MinusColumnVariable,
                        "ONE LINE DESCR",
                        "NO HELP");

MinusColumnVariable::MinusColumnVariable(Variable* input1, Variable* input2)
    : inherited(input1, input2, input1->length(), input1->width())
{
    build_();
}

void
MinusColumnVariable::build()
{
    inherited::build();
    build_();
}

void
MinusColumnVariable::build_()
{
    if (input1 && input2) {
        if(!input2->isColumnVec())
            PLERROR("IN MinusColumnVariable: input2 is not a column");
        if(input2->length() != input1->length())
            PLERROR("IN MinusColumnVariable: input1 and input2 have a different length()");
    }
}



void MinusColumnVariable::recomputeSize(int& l, int& w) const
{
    if (input1) {
        l = input1->length();
        w = input1->width();
    } else
        l = w = 0;
}


void MinusColumnVariable::fprop()
{
    int k=0;
    for(int i=0; i<length(); i++)
        for(int j=0; j<width(); j++, k++)
            valuedata[k] = input1->valuedata[k] - input2->valuedata[i];
}


void MinusColumnVariable::bprop()
{
    int k=0;
    for(int i=0; i<length(); i++)
        for(int j=0; j<width(); j++, k++)
        {
            input1->gradientdata[k] += gradientdata[k];
            input2->gradientdata[i] -= gradientdata[k];
        }
}


void MinusColumnVariable::bbprop()
{
    if (input1->diaghessian.length()==0)
        input1->resizeDiagHessian();
    if (input2->diaghessian.length()==0)
        input2->resizeDiagHessian();
    int k=0;
    for(int i=0; i<length(); i++)
        for(int j=0; j<width(); j++, k++)
        {
            input1->diaghessiandata[k] += diaghessiandata[k];
            input2->diaghessiandata[i] -= diaghessiandata[k];
        }
}


void MinusColumnVariable::symbolicBprop()
{
    input1->accg(g);
    input2->accg(-rowSum(g));
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
