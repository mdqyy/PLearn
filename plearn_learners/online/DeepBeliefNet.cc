// -*- C++ -*-

// DeepBeliefNet.cc
//
// Copyright (C) 2006 Pascal Lamblin
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

// Authors: Pascal Lamblin

/*! \file DeepBeliefNet.cc */


#define PL_LOG_MODULE_NAME "DeepBeliefNet"
#include <plearn/io/pl_log.h>

#include "DeepBeliefNet.h"

#define minibatch_hack 0 // Do we force the minibatch setting? (debug hack)

namespace PLearn {
using namespace std;

PLEARN_IMPLEMENT_OBJECT(
    DeepBeliefNet,
    "Neural network, learned layer-wise in a greedy fashion.",
    "This version supports different unit types, different connection types,\n"
    "and different cost functions, including the NLL in classification.\n");

///////////////////
// DeepBeliefNet //
///////////////////
DeepBeliefNet::DeepBeliefNet() :
    cd_learning_rate( 0. ),
    grad_learning_rate( 0. ),
    batch_size( 1 ),
    grad_decrease_ct( 0. ),
    // grad_weight_decay( 0. ),
    n_classes( -1 ),
    use_classification_cost( true ),
    reconstruct_layerwise( false ),
    n_layers( 0 ),
    online ( false ),
    background_gibbs_update_ratio(0),
    gibbs_chain_statistics_forgetting_factor(0.999),
    gibbs_chain_reinit_freq( INT_MAX ),
    minibatch_size( 0 ),
    initialize_gibbs_chain( false ),
    final_module_has_learning_rate( false ),
    final_cost_has_learning_rate( false ),
    nll_cost_index( -1 ),
    class_cost_index( -1 ),
    final_cost_index( -1 ),
    reconstruction_cost_index( -1 )
{
    random_gen = new PRandom( seed_ );
}

////////////////////
// declareOptions //
////////////////////
void DeepBeliefNet::declareOptions(OptionList& ol)
{
    declareOption(ol, "cd_learning_rate", &DeepBeliefNet::cd_learning_rate,
                  OptionBase::buildoption,
                  "The learning rate used during contrastive divergence"
                  " learning");

    declareOption(ol, "grad_learning_rate", &DeepBeliefNet::grad_learning_rate,
                  OptionBase::buildoption,
                  "The decrease constant of the learning rate used during"
                  " gradient descent");

    declareOption(ol, "batch_size", &DeepBeliefNet::batch_size,
                  OptionBase::buildoption,
        "Training batch size (1=stochastic learning, 0=full batch learning).");

    /* NOT IMPLEMENTED YET
    declareOption(ol, "grad_weight_decay", &DeepBeliefNet::grad_weight_decay,
                  OptionBase::buildoption,
                  "The weight decay used during the gradient descent");
    */

    declareOption(ol, "n_classes", &DeepBeliefNet::n_classes,
                  OptionBase::buildoption,
                  "Number of classes in the training set:\n"
                  "  - 0 means we are doing regression,\n"
                  "  - 1 means we have two classes, but only one output,\n"
                  "  - 2 means we also have two classes, but two outputs"
                  " summing to 1,\n"
                  "  - >2 is the usual multiclass case.\n"
                  );

    declareOption(ol, "training_schedule", &DeepBeliefNet::training_schedule,
                  OptionBase::buildoption,
                  "Number of examples before starting each phase except the first\n"
                  "(first the greedy phases, and then the fine-tuning phase).\n"
                  "For example for 2 hidden layers, with 1000 examples in each\n"
                  "greedy phase, this option should be [1000 2000] and the last\n"
                  "nstages - 2000 examples will be used for fine-tuning.\n"
                  "When online = true, this vector is ignored and should be empty.\n");

    declareOption(ol, "use_classification_cost",
                  &DeepBeliefNet::use_classification_cost,
                  OptionBase::buildoption,
                  "Put the class target as an extra input of the top-level RBM\n"
                  "and compute and maximize conditional class probability in that\n"
                  "top layer (probability of the correct class given the other input\n"
                  "of the top-level RBM, which is the output of the rest of the network.\n"
                  "This only makes sense if a classification_module is provided.\n");

    declareOption(ol, "reconstruct_layerwise",
                  &DeepBeliefNet::reconstruct_layerwise,
                  OptionBase::buildoption,
                  "Compute reconstruction error of each layer as an auto-encoder.\n"
                  "This is done using cross-entropy between actual and reconstructed.\n"
                  "This option automatically adds the following cost names:\n"
                  "   layerwise_reconstruction_error (sum over all layers)\n"
                  "   layer1_reconstruction_error (only layer 1)\n"
                  "   layer2_reconstruction_error (only layer 2)\n"
                  "   etc.\n");

    declareOption(ol, "layers", &DeepBeliefNet::layers,
                  OptionBase::buildoption,
                  "The layers of units in the network (including the input layer).");

    declareOption(ol, "connections", &DeepBeliefNet::connections,
                  OptionBase::buildoption,
                  "The weights of the connections between the layers");

    declareOption(ol, "classification_module",
                  &DeepBeliefNet::classification_module,
                  OptionBase::learntoption,
                  "The module computing the class probabilities (if"
                  " use_classification_cost)\n"
                  );

    declareOption(ol, "classification_cost",
                  &DeepBeliefNet::classification_cost,
                  OptionBase::nosave,
                  "The module computing the classification cost function (NLL)"
                  " on top\n"
                  "of classification_module.\n"
                  );

    declareOption(ol, "joint_layer", &DeepBeliefNet::joint_layer,
                  OptionBase::nosave,
                  "Concatenation of layers[n_layers-2] and the target layer\n"
                  "(that is inside classification_module), if"
                  " use_classification_cost.\n"
                 );

    declareOption(ol, "final_module", &DeepBeliefNet::final_module,
                  OptionBase::buildoption,
                  "Optional module that takes as input the output of the last"
                  " layer\n"
                  "layers[n_layers-1), and its output is fed to final_cost,"
                  " and\n"
                  "concatenated with the one of classification_cost (if"
                  " present)\n"
                  "as output of the learner.\n"
                  "If it is not provided, then the last layer will directly be"
                  " put as\n"
                  "input of final_cost.\n"
                 );

    declareOption(ol, "final_cost", &DeepBeliefNet::final_cost,
                  OptionBase::buildoption,
                  "The cost function to be applied on top of the DBN (or of\n"
                  "final_module if provided). Its gradients will be"
                  " backpropagated\n"
                  "to final_module, then combined with the one of"
                  " classification_cost and\n"
                  "backpropagated to the layers.\n"
                  );

    declareOption(ol, "partial_costs", &DeepBeliefNet::partial_costs,
                  OptionBase::buildoption,
                  "The different cost functions to be applied on top of each"
                  " layer\n"
                  "(except the first one) of the RBM. These costs are not\n"
                  "back-propagated to previous layers.\n");

    declareOption(ol, "online", &DeepBeliefNet::online,
                  OptionBase::buildoption,
                  "If true then all unsupervised training stages (as well as\n"
                  "the fine-tuning stage) are done simultaneously.\n");

    declareOption(ol, "background_gibbs_update_ratio", &DeepBeliefNet::background_gibbs_update_ratio,
                  OptionBase::buildoption,
                  "Coefficient between 0 and 1. If non-zero, run a background Gibbs chain and use\n"
                  "the visible-hidden statistics to contribute in the negative phase update\n"
                  "(in proportion background_gibbs_update_ratio wrt the contrastive divergence\n"
                  "negative phase statistics). If = 1, then do not perform any contrastive\n"
                  "divergence negative phase (use only the Gibbs chain statistics).\n");

    declareOption(ol, "gibbs_chain_statistics_forgetting_factor", 
                  &DeepBeliefNet::gibbs_chain_statistics_forgetting_factor,
                  OptionBase::buildoption,
                  "Negative chain statistics are forgotten at this rate (a value of 0\n"
                  "would only use the current sample, a value of .99 would use 1% of\n"
                  "the current sample and 99% of the old statistics).\n");

    declareOption(ol, "gibbs_chain_reinit_freq",
                  &DeepBeliefNet::gibbs_chain_reinit_freq,
                  OptionBase::buildoption,
                  "After how many training examples to re-initialize the Gibbs chains.\n"
                  "If == INT_MAX, the default value of this option, then NEVER\n"
                  "re-initialize except at the beginning, when stage==0.\n");

    declareOption(ol, "top_layer_joint_cd", &DeepBeliefNet::top_layer_joint_cd,
                  OptionBase::buildoption,
                  "Wether we do a step of joint contrastive divergence on"
                  " top-layer.\n"
                  "Only used if online for the moment.\n");

    declareOption(ol, "n_layers", &DeepBeliefNet::n_layers,
                  OptionBase::learntoption,
                  "Number of layers");

    declareOption(ol, "minibatch_size", &DeepBeliefNet::minibatch_size,
                  OptionBase::learntoption,
                  "Size of a mini-batch.");

    declareOption(ol, "gibbs_down_state", &DeepBeliefNet::gibbs_down_state,
                  OptionBase::learntoption,
                  "State of visible units of RBMs at each layer in background Gibbs chain.");

    /*
    declareOption(ol, "n_final_costs", &DeepBeliefNet::n_final_costs,
                  OptionBase::learntoption,
                  "Number of final costs");
     */

    /*
    declareOption(ol, "", &DeepBeliefNet::,
                  OptionBase::learntoption,
                  "");
     */

    // Now call the parent class' declareOptions
    inherited::declareOptions(ol);
}

////////////
// build_ //
////////////
void DeepBeliefNet::build_()
{
    PLASSERT( batch_size >= 0 );

    MODULE_LOG << "build_() called" << endl;

    // Initialize some learnt variables
    if (layers.isEmpty())
        PLERROR("In DeepBeliefNet::build_ - You must provide at least one RBM "
                "layer through the 'layers' option");
    else
        n_layers = layers.length();

    if( training_schedule.length() != n_layers-1  && !online )
    {
        MODULE_LOG << "training_schedule.length() != n_layers-1, resizing and"
            " zeroing" << endl;
        training_schedule.resize( n_layers-1 );
        training_schedule.fill( 0 );
    }

    build_layers_and_connections();

    build_costs();
}

/////////////////
// build_costs //
/////////////////
void DeepBeliefNet::build_costs()
{
    cost_names.resize(0);
    int current_index = 0;

    // build the classification module, its cost and the joint layer
    if( use_classification_cost )
    {
        PLASSERT( n_classes >= 2 );
        build_classification_cost();

        cost_names.append("NLL");
        nll_cost_index = current_index;
        current_index++;

        cost_names.append("class_error");
        class_cost_index = current_index;
        current_index++;
    }

    if( final_cost )
    {
        build_final_cost();

        TVec<string> final_names = final_cost->name();
        int n_final_costs = final_names.length();

        for( int i=0; i<n_final_costs; i++ )
            cost_names.append("final." + final_names[i]);

        final_cost_index = current_index;
        current_index += n_final_costs;
    }

    if( partial_costs )
    {
        int n_partial_costs = partial_costs.length();
        partial_costs_indices.resize(n_partial_costs);

        for( int i=0; i<n_partial_costs; i++ )
            if( partial_costs[i] )
            {
                TVec<string> names = partial_costs[i]->name();
                int n_partial_costs_i = names.length();
                for( int j=0; j<n_partial_costs_i; j++ )
                    cost_names.append("partial"+tostring(i)+"."+names[j]);
                partial_costs_indices[i] = current_index;
                current_index += n_partial_costs_i;
            }
            else
                partial_costs_indices[i] = -1;
    }
    else
        partial_costs_indices.resize(0);

    if( reconstruct_layerwise )
    {
        reconstruction_costs.resize(n_layers-1);

        cost_names.append("layerwise_reconstruction_error");
        reconstruction_cost_index = current_index;
        current_index++;

        for( int i=0; i<n_layers-1; i++ )
            cost_names.append("layer"+tostring(i)+".reconstruction_error");
        current_index += n_layers-1;
    }
    else
        reconstruction_costs.resize(0);

    PLASSERT( current_index == cost_names.length() );
}

//////////////////////////////////
// build_layers_and_connections //
//////////////////////////////////
void DeepBeliefNet::build_layers_and_connections()
{
    MODULE_LOG << "build_layers_and_connections() called" << endl;

    if( connections.length() != n_layers-1 )
        PLERROR("DeepBeliefNet::build_layers_and_connections() - \n"
                "connections.length() (%d) != n_layers-1 (%d).\n",
                connections.length(), n_layers-1);

    if( inputsize_ >= 0 )
        PLASSERT( layers[0]->size == inputsize() );

    activation_gradients.resize( n_layers );
    activations_gradients.resize( n_layers );
    expectation_gradients.resize( n_layers );
    expectations_gradients.resize( n_layers );
    gibbs_down_state.resize( n_layers-1 );

    for( int i=0 ; i<n_layers-1 ; i++ )
    {
        if( layers[i]->size != connections[i]->down_size )
            PLERROR("DeepBeliefNet::build_layers_and_connections() - \n"
                    "layers[%i]->size (%d) != connections[%i]->down_size (%d)."
                    "\n", i, layers[i]->size, i, connections[i]->down_size);

        if( connections[i]->up_size != layers[i+1]->size )
            PLERROR("DeepBeliefNet::build_layers_and_connections() - \n"
                    "connections[%i]->up_size (%d) != layers[%i]->size (%d)."
                    "\n", i, connections[i]->up_size, i+1, layers[i+1]->size);

        layers[i]->random_gen = random_gen;
        layers[i]->build();

        connections[i]->random_gen = random_gen;
        connections[i]->build();

        activation_gradients[i].resize( layers[i]->size );
        expectation_gradients[i].resize( layers[i]->size );
    }
    layers[n_layers-1]->random_gen = random_gen;
    int last_layer_size = layers[n_layers-1]->size;
    PLASSERT_MSG(last_layer_size >= 0,
                 "Size of last layer must be non-negative");
    activation_gradients[n_layers-1].resize(last_layer_size);
    expectation_gradients[n_layers-1].resize(last_layer_size);
}

///////////////////////////////
// build_classification_cost //
///////////////////////////////
void DeepBeliefNet::build_classification_cost()
{
    MODULE_LOG << "build_classification_cost() called" << endl;

    PLASSERT_MSG(batch_size == 1, "DeepBeliefNet::build_classification_cost - "
            "This method has not been verified yet for minibatch "
            "compatibility");

    PP<RBMMatrixConnection> last_to_target = new RBMMatrixConnection();
    last_to_target->up_size = layers[n_layers-1]->size;
    last_to_target->down_size = n_classes;
    last_to_target->random_gen = random_gen;
    last_to_target->build();

    PP<RBMMultinomialLayer> target_layer = new RBMMultinomialLayer();
    target_layer->size = n_classes;
    target_layer->random_gen = random_gen;
    target_layer->build();

    PLASSERT_MSG(n_layers >= 2, "You must specify at least two layers (the "
            "input layer and one hidden layer)");

    classification_module = new RBMClassificationModule();
    classification_module->previous_to_last = connections[n_layers-2];
    classification_module->last_layer =
        (RBMBinomialLayer*) (RBMLayer*) layers[n_layers-1];
    classification_module->last_to_target = last_to_target;
    classification_module->target_layer = target_layer;
    classification_module->random_gen = random_gen;
    classification_module->build();

    classification_cost = new NLLCostModule();
    classification_cost->input_size = n_classes;
    classification_cost->target_size = 1;
    classification_cost->build();

    joint_layer = new RBMMixedLayer();
    joint_layer->sub_layers.resize( 2 );
    joint_layer->sub_layers[0] = layers[ n_layers-2 ];
    joint_layer->sub_layers[1] = target_layer;
    joint_layer->random_gen = random_gen;
    joint_layer->build();
}

//////////////////////
// build_final_cost //
//////////////////////
void DeepBeliefNet::build_final_cost()
{
    MODULE_LOG << "build_final_cost() called" << endl;

    PLASSERT_MSG(final_cost->input_size >= 0, "The input size of the final "
            "cost must be non-negative");

    final_cost_gradient.resize( final_cost->input_size );
    final_cost->setLearningRate( grad_learning_rate );

    if( final_module )
    {
        if( layers[n_layers-1]->size != final_module->input_size )
            PLERROR("DeepBeliefNet::build_final_cost() - "
                    "layers[%i]->size (%d) != final_module->input_size (%d)."
                    "\n", n_layers-1, layers[n_layers-1]->size,
                    final_module->input_size);

        if( final_module->output_size != final_cost->input_size )
            PLERROR("DeepBeliefNet::build_final_cost() - "
                    "final_module->output_size (%d) != final_cost->input_size."
                    "\n", n_layers-1, layers[n_layers-1]->size,
                    final_module->input_size);

        final_module->setLearningRate( grad_learning_rate );
    }
    else
    {
        if( layers[n_layers-1]->size != final_cost->input_size )
            PLERROR("DeepBeliefNet::build_final_cost() - "
                    "layers[%i]->size (%d) != final_cost->input_size (%d)."
                    "\n", n_layers-1, layers[n_layers-1]->size,
                    final_cost->input_size);
    }

    // check target size and final_cost->input_size
    if( n_classes == 0 ) // regression
    {
        if( final_cost->input_size != targetsize() )
            PLERROR("DeepBeliefNet::build_final_cost() - "
                    "final_cost->input_size (%d) != targetsize() (%d), "
                    "although we are doing regression (n_classes == 0).\n",
                    final_cost->input_size, targetsize());
    }
    else
    {
        if( final_cost->input_size != n_classes )
            PLERROR("DeepBeliefNet::build_final_cost() - "
                    "final_cost->input_size (%d) != n_classes (%d), "
                    "although we are doing classification (n_classes != 0).\n",
                    final_cost->input_size, n_classes);

        if( targetsize_ >= 0 && targetsize() != 1 )
            PLERROR("DeepBeliefNet::build_final_cost() - "
                    "targetsize() (%d) != 1, "
                    "although we are doing classification (n_classes != 0).\n",
                    targetsize());
    }

}

///////////
// build //
///////////
void DeepBeliefNet::build()
{
    inherited::build();
    build_();
}

/////////////////////////////////
// makeDeepCopyFromShallowCopy //
/////////////////////////////////
void DeepBeliefNet::makeDeepCopyFromShallowCopy(CopiesMap& copies)
{
    inherited::makeDeepCopyFromShallowCopy(copies);

    deepCopyField(training_schedule,        copies);
    deepCopyField(layers,                   copies);
    deepCopyField(connections,              copies);
    deepCopyField(final_module,             copies);
    deepCopyField(final_cost,               copies);
    deepCopyField(partial_costs,            copies);
    deepCopyField(classification_module,    copies);
    deepCopyField(timer,                    copies);
    deepCopyField(classification_cost,      copies);
    deepCopyField(joint_layer,              copies);
    deepCopyField(activation_gradients,     copies);
    deepCopyField(activations_gradients,    copies);
    deepCopyField(expectation_gradients,    copies);
    deepCopyField(expectations_gradients,   copies);
    deepCopyField(final_cost_input,         copies);
    deepCopyField(final_cost_inputs,        copies);
    deepCopyField(final_cost_value,         copies);
    deepCopyField(final_cost_values,        copies);
    deepCopyField(final_cost_output,        copies);
    deepCopyField(class_output,             copies);
    deepCopyField(class_gradient,           copies);
    deepCopyField(class_input_gradient,     copies);
    deepCopyField(final_cost_gradient,      copies);
    deepCopyField(final_cost_gradients,     copies);
    deepCopyField(save_layer_activation,    copies);
    deepCopyField(save_layer_expectation,   copies);
    deepCopyField(save_layer_activations,   copies);
    deepCopyField(save_layer_expectations,  copies);
    deepCopyField(pos_down_val,             copies);
    deepCopyField(pos_up_val,               copies);
    deepCopyField(cd_neg_up_vals,           copies);
    deepCopyField(cd_neg_down_vals,         copies);
    deepCopyField(gibbs_down_state,         copies);
    deepCopyField(optimized_costs,          copies);
    deepCopyField(partial_costs_indices,    copies);
}


////////////////
// outputsize //
////////////////
int DeepBeliefNet::outputsize() const
{
    int out_size = 0;
    if( use_classification_cost )
        out_size += n_classes;

    if( final_module )
        out_size += final_module->output_size;
    else
        out_size += layers[n_layers-1]->size;

    return out_size;
}

////////////
// forget //
////////////
void DeepBeliefNet::forget()
{
    inherited::forget();

    for( int i=0 ; i<n_layers-1 ; i++ )
        connections[i]->forget();

    if( use_classification_cost )
    {
        classification_cost->forget();
        classification_module->forget();
    }

    if( final_module )
        final_module->forget();

    if( final_cost )
        final_cost->forget();

    if( partial_costs )
        for( int i=0 ; i<n_layers-1 ; i++ )
            if( partial_costs[i] )
                partial_costs[i]->forget();
}

///////////
// train //
///////////
void DeepBeliefNet::train()
{
    MODULE_LOG << "train() called " << endl;
    MODULE_LOG << "  training_schedule = " << training_schedule << endl;
    MODULE_LOG << "stage = " << stage << ", target nstages = " << nstages << endl;

    PLASSERT( train_set );
    if (stage == 0) {
        // Training set-dependent initialization.
        minibatch_size = batch_size > 0 ? batch_size : train_set->length();
        for (int i = 0 ; i < n_layers; i++) {
            activations_gradients[i].resize(minibatch_size, layers[i]->size);
            expectations_gradients[i].resize(minibatch_size, layers[i]->size);

            if (background_gibbs_update_ratio>0)
                gibbs_down_state[i].resize(minibatch_size, layers[i]->size);
        }
        if (final_cost)
            final_cost_gradients.resize(minibatch_size, final_cost->input_size);
        optimized_costs.resize(minibatch_size);
    }

    layers[n_layers-1]->random_gen = random_gen;
    int last_layer_size = layers[n_layers-1]->size;
    PLASSERT_MSG(last_layer_size >= 0,
                 "Size of last layer must be non-negative");
    activation_gradients[n_layers-1].resize(last_layer_size);
    expectation_gradients[n_layers-1].resize(last_layer_size);

    Vec input( inputsize() );
    Vec target( targetsize() );
    real weight; // unused
    Mat inputs(minibatch_size, inputsize());
    Mat targets(minibatch_size, targetsize());
    Vec weights;

    TVec<string> train_cost_names = getTrainCostNames() ;
    Vec train_costs( train_cost_names.length() );
    Mat train_costs_m(minibatch_size, train_cost_names.length());
    train_costs.fill(MISSING_VALUE) ;
    train_costs_m.fill(MISSING_VALUE);

    int nsamples = train_set->length();

    if( !initTrain() )
    {
        MODULE_LOG << "train() aborted" << endl;
        return;
    }

    PP<ProgressBar> pb;

    // clear stats of previous epoch
    train_stats->forget();

    if (online)
    {
        // Train all layers simultaneously AND fine-tuning as well!
        if( report_progress && stage < nstages )
            pb = new ProgressBar( "Training "+classname(),
                                  nstages - stage );

        for( ; stage<nstages; stage++)
        {
            // Do a step every 'minibatch_size' examples.
            if (stage % minibatch_size == 0) {
                int sample_start = stage % nsamples;
                if (batch_size > 1 || minibatch_hack) {
                    train_set->getExamples(sample_start, minibatch_size,
                            inputs, targets, weights, NULL, true);
                    onlineStep( inputs, targets, train_costs_m );
                } else {
                    train_set->getExample(sample_start, input, target, weight);
                    onlineStep( input, target, train_costs );
                }
            }
            if( pb )
                pb->update( stage + 1 );
        }
    }
    else // Greedy learning, one layer at a time.
    {
        /***** initial greedy training *****/
        for( int i=0 ; i<n_layers-1 ; i++ )
        {
            if( use_classification_cost && i == n_layers-2 )
                break; // we will do a joint supervised learning instead

            MODULE_LOG << "Training connection weights between layers " << i
                       << " and " << i+1 << endl;

            int first_stage = stage;
            int end_stage = min( training_schedule[i], nstages );

            MODULE_LOG << "  stage = " << stage << endl;
            MODULE_LOG << "  end_stage = " << end_stage << endl;
            MODULE_LOG << "  cd_learning_rate = " << cd_learning_rate << endl;

            if( report_progress && stage < end_stage )
                pb = new ProgressBar( "Training layer "+tostring(i)
                                      +" of "+classname(),
                                      end_stage - first_stage);

            layers[i]->setLearningRate( cd_learning_rate );
            connections[i]->setLearningRate( cd_learning_rate );
            layers[i+1]->setLearningRate( cd_learning_rate );

            for( ; stage<end_stage ; stage++ )
            {
                // Do a step every 'minibatch_size' examples.
                if (stage % minibatch_size == 0) {
                    int sample_start = stage % nsamples;
                    if (batch_size > 1 || minibatch_hack) {
                        train_set->getExamples(sample_start, minibatch_size,
                                inputs, targets, weights, NULL, true);
                        train_costs_m.fill(MISSING_VALUE);
                        greedyStep( inputs, targets, i , train_costs_m);
                        for (int k = 0; k < minibatch_size; k++)
                            train_stats->update(train_costs_m(k));
                    } else {
                        train_set->getExample(sample_start, input, target, weight);
                        greedyStep( input, target, i );
                    }

                }
                if( pb )
                    pb->update( stage - first_stage + 1 );
/*                    if( i == 0 )
                    else
                    pb->update( stage - training_schedule[i-1] + 1 ); */
            }
        }

        // possible supervised part
        if( use_classification_cost )
        {
            PLASSERT_MSG(batch_size == 1, "'use_classification_cost' code not "
                    "verified with mini-batch learning yet");

            MODULE_LOG << "Training the classification module" << endl;

            int end_stage = min( training_schedule[n_layers-2], nstages );

            MODULE_LOG << "  stage = " << stage << endl;
            MODULE_LOG << "  end_stage = " << end_stage << endl;
            MODULE_LOG << "  cd_learning_rate = " << cd_learning_rate << endl;

            if( report_progress && stage < end_stage )
                pb = new ProgressBar( "Training the classification module",
                                      end_stage - stage );

            // set appropriate learning rate
            joint_layer->setLearningRate( cd_learning_rate );
            classification_module->joint_connection->setLearningRate(
                cd_learning_rate );
            layers[ n_layers-1 ]->setLearningRate( cd_learning_rate );

            int previous_stage = (n_layers < 3) ? 0
                : training_schedule[n_layers-3];
            for( ; stage<end_stage ; stage++ )
            {
                int sample = stage % nsamples;
                train_set->getExample( sample, input, target, weight );
                jointGreedyStep( input, target );

                if( pb )
                    pb->update( stage - previous_stage + 1 );
            }
        }


        /***** fine-tuning by gradient descent *****/
        if( stage >= nstages )
            return;

        MODULE_LOG << "Fine-tuning all parameters, by gradient descent" << endl;
        MODULE_LOG << "  stage = " << stage << endl;
        MODULE_LOG << "  nstages = " << nstages << endl;
        MODULE_LOG << "  grad_learning_rate = " << grad_learning_rate << endl;

        int init_stage = stage;
        if( report_progress && stage < nstages )
            pb = new ProgressBar( "Fine-tuning parameters of all layers of "
                                  + classname(),
                                  nstages - init_stage );

        setLearningRate( grad_learning_rate );

        train_stats->forget();
        bool update_stats = false;
        for( ; stage<nstages ; stage++ )
        {
            initialize_gibbs_chain=(stage%gibbs_chain_reinit_freq==0);
            // Update every 'minibatch_size' samples.
            if (stage % minibatch_size == 0) {
                int sample_start = stage % nsamples;
                // Only update train statistics for the last 'epoch', i.e. last
                // 'nsamples' seen.
                update_stats = update_stats || stage >= nstages - nsamples;

                if( !fast_exact_is_equal( grad_decrease_ct, 0. ) )
                    setLearningRate( grad_learning_rate
                            / (1. + grad_decrease_ct * (stage - init_stage) ) );

                if (minibatch_size > 1 || minibatch_hack) {
                    train_set->getExamples(sample_start, minibatch_size, inputs,
                            targets, weights, NULL, true);
                    train_costs_m.fill(MISSING_VALUE);
                    fineTuningStep(inputs, targets, train_costs_m);
                } else {
                    train_set->getExample( sample_start, input, target, weight );
                    fineTuningStep( input, target, train_costs );
                }
                if (update_stats)
                    if (minibatch_size > 1 || minibatch_hack)
                        for (int k = 0; k < minibatch_size; k++)
                            train_stats->update(train_costs_m(k));
                    else
                        train_stats->update( train_costs );

            }
            if( pb )
                pb->update( stage - init_stage + 1 );
        }
    }

    train_stats->finalize();

}

////////////////
// onlineStep //
////////////////
void DeepBeliefNet::onlineStep( const Vec& input, const Vec& target,
                                Vec& train_costs)
{
    PLASSERT(batch_size == 1);

    TVec<Vec> cost;
    if (partial_costs)
        cost.resize(n_layers-1);

    layers[0]->expectation << input;
    // FORWARD PHASE
    //Vec layer_input;
    for( int i=0 ; i<n_layers-1 ; i++ )
    {
        // mean-field fprop from layer i to layer i+1
        connections[i]->setAsDownInput( layers[i]->expectation );
        // this does the actual matrix-vector computation
        layers[i+1]->getAllActivations( connections[i] );
        layers[i+1]->computeExpectation();

        // propagate into local cost associated to output of layer i+1
        if( partial_costs && partial_costs[ i ] )
        {
            partial_costs[ i ]->fprop( layers[ i+1 ]->expectation,
                                       target, cost[i] );

            // Backward pass
            // first time we set these gradients: do not accumulate
            partial_costs[ i ]->bpropUpdate( layers[ i+1 ]->expectation,
                                             target, cost[i][0],
                                             expectation_gradients[ i+1 ] );

            train_costs.subVec(partial_costs_indices[i], cost[i].length())
                << cost[i];
        }
        else
            expectation_gradients[i+1].clear();
    }

    // top layer may be connected to a final_module followed by a
    // final_cost and / or may be used to predict class probabilities
    // through a joint classification_module

    if ( final_cost )
    {
        if( final_module )
        {
                final_module->fprop( layers[ n_layers-1 ]->expectation,
                        final_cost_input );
                final_cost->fprop( final_cost_input, target,
                        final_cost_value );
                final_cost->bpropUpdate( final_cost_input, target,
                        final_cost_value[0],
                        final_cost_gradient );

                final_module->bpropUpdate(
                        layers[ n_layers-1 ]->expectation,
                        final_cost_input,
                        expectation_gradients[ n_layers-1 ],
                        final_cost_gradient, true );
        }
        else
        {
                final_cost->fprop( layers[ n_layers-1 ]->expectation,
                        target,
                        final_cost_value );
                final_cost->bpropUpdate( layers[ n_layers-1 ]->expectation,
                        target, final_cost_value[0],
                        expectation_gradients[n_layers-1],
                        true);
        }

        train_costs.subVec(final_cost_index, final_cost_value.length())
            << final_cost_value;
    }

    if ( final_cost || (partial_costs && partial_costs[n_layers-2]) )
    {
        layers[n_layers-1]->setLearningRate( grad_learning_rate );
        connections[n_layers-2]->setLearningRate( grad_learning_rate );

        layers[ n_layers-1 ]->bpropUpdate( layers[ n_layers-1 ]->activation,
                                           layers[ n_layers-1 ]->expectation,
                                           activation_gradients[ n_layers-1 ],
                                           expectation_gradients[ n_layers-1 ],
                                           false);

        connections[ n_layers-2 ]->bpropUpdate(
            layers[ n_layers-2 ]->expectation,
            layers[ n_layers-1 ]->activation,
            expectation_gradients[ n_layers-2 ],
            activation_gradients[ n_layers-1 ],
            true);
        // accumulate into expectation_gradients[n_layers-2]
        // because a partial cost may have already put a gradient there
    }

    if( use_classification_cost )
    {
        classification_module->fprop( layers[ n_layers-2 ]->expectation,
                                      class_output );
        real nll_cost;

        // This doesn't work. gcc bug?
        // classification_cost->fprop( class_output, target, cost );
        classification_cost->CostModule::fprop( class_output, target,
                                                nll_cost );

        real class_error =
            ( argmax(class_output) == (int) round(target[0]) ) ? 0: 1;

        train_costs[nll_cost_index] = nll_cost;
        train_costs[class_cost_index] = class_error;

        classification_cost->bpropUpdate( class_output, target, nll_cost,
                                          class_gradient );

        classification_module->bpropUpdate( layers[ n_layers-2 ]->expectation,
                                            class_output,
                                            expectation_gradients[n_layers-2],
                                            class_gradient,
                                            true );
        if( top_layer_joint_cd )
        {
            // set the input of the joint layer
            Vec target_exp = classification_module->target_layer->expectation;
            fill_one_hot( target_exp, (int) round(target[0]), real(0.), real(1.) );

            joint_layer->setLearningRate( cd_learning_rate );
            layers[ n_layers-1 ]->setLearningRate( cd_learning_rate );
            classification_module->joint_connection->setLearningRate(
                cd_learning_rate );

            save_layer_activation.resize(layers[ n_layers-2 ]->size);
            save_layer_activation << layers[ n_layers-2 ]->activation;
            save_layer_expectation.resize(layers[ n_layers-2 ]->size);
            save_layer_expectation << layers[ n_layers-2 ]->expectation;

            contrastiveDivergenceStep(
                get_pointer(joint_layer),
                get_pointer(classification_module->joint_connection),
                layers[ n_layers-1 ], n_layers-2);

            layers[ n_layers-2 ]->activation << save_layer_activation;
            layers[ n_layers-2 ]->expectation << save_layer_expectation;
        }
    }


    // DOWNWARD PHASE (the downward phase for top layer is already done above)
    for( int i=n_layers-3 ; i>=0 ; i-- )
    {
        connections[ i ]->setLearningRate( grad_learning_rate );
        layers[ i+1 ]->setLearningRate( grad_learning_rate );

        layers[i+1]->bpropUpdate( layers[i+1]->activation,
                                  layers[i+1]->expectation,
                                  activation_gradients[i+1],
                                  expectation_gradients[i+1] );

        connections[i]->bpropUpdate( layers[i]->expectation,
                                     layers[i+1]->activation,
                                     expectation_gradients[i],
                                     activation_gradients[i+1],
                                     true);

        // N.B. the contrastiveDivergenceStep changes the activation and
        // expectation fields of top layer of the RBM, so it must be
        // done last
        layers[i]->setLearningRate( cd_learning_rate );
        layers[i+1]->setLearningRate( cd_learning_rate );
        connections[i]->setLearningRate( cd_learning_rate );

        if( i > 0 )
        {
            save_layer_activation.resize(layers[i]->size);
            save_layer_activation << layers[i]->activation;
            save_layer_expectation.resize(layers[i]->size);
            save_layer_expectation << layers[i]->expectation;
        }
        contrastiveDivergenceStep( layers[ i ],
                                   connections[ i ],
                                   layers[ i+1 ] ,
                                   i, true);
        if( i > 0 )
        {
            layers[i]->activation << save_layer_activation;
            layers[i]->expectation << save_layer_expectation;
        }
    }

}

void DeepBeliefNet::onlineStep(const Mat& inputs, const Mat& targets,
                               Mat& train_costs)
{
    // TODO Can we avoid this memory allocation?
    TVec<Mat> cost;
    Vec optimized_cost(inputs.length());
    if (partial_costs) {
        cost.resize(n_layers-1);
    }

    layers[0]->setExpectations(inputs);
    // FORWARD PHASE
    //Vec layer_input;
    for( int i=0 ; i<n_layers-1 ; i++ )
    {
        // mean-field fprop from layer i to layer i+1
        connections[i]->setAsDownInputs( layers[i]->getExpectations() );
        // this does the actual matrix-vector computation
        layers[i+1]->getAllActivations( connections[i], 0, true );
        layers[i+1]->computeExpectations();

        // propagate into local cost associated to output of layer i+1
        if( partial_costs && partial_costs[ i ] )
        {
            partial_costs[ i ]->fprop( layers[ i+1 ]->getExpectations(),
                                       targets, cost[i] );

            // Backward pass
            // first time we set these gradients: do not accumulate
            optimized_cost << cost[i].column(0); // TODO Can we optimize?
            partial_costs[ i ]->bpropUpdate( layers[ i+1 ]->getExpectations(),
                                             targets, optimized_cost,
                                             expectations_gradients[ i+1 ] );

            train_costs.subMatColumns(partial_costs_indices[i], cost[i].width())
                << cost[i];
        }
        else
            expectations_gradients[i+1].clear();
    }

    // top layer may be connected to a final_module followed by a
    // final_cost and / or may be used to predict class probabilities
    // through a joint classification_module

    if ( final_cost )
    {
        if( final_module )
        {
                final_module->fprop( layers[ n_layers-1 ]->getExpectations(),
                        final_cost_inputs );
                final_cost->fprop( final_cost_inputs, targets,
                        final_cost_values );
                optimized_cost << final_cost_values.column(0); // TODO optimize
                final_cost->bpropUpdate( final_cost_inputs, targets,
                        optimized_cost,
                        final_cost_gradients );

                final_module->bpropUpdate(
                        layers[ n_layers-1 ]->getExpectations(),
                        final_cost_inputs,
                        expectations_gradients[ n_layers-1 ],
                        final_cost_gradients, true );
        }
        else
        {
                final_cost->fprop( layers[ n_layers-1 ]->getExpectations(),
                        targets,
                        final_cost_values );
                optimized_cost << final_cost_values.column(0); // TODO optimize
                final_cost->bpropUpdate( layers[n_layers-1]->getExpectations(),
                        targets, optimized_cost,
                        expectations_gradients[n_layers-1],
                        true);
        }

        train_costs.subMatColumns(final_cost_index, final_cost_values.width())
            << final_cost_values;
    }

    if ( final_cost || (partial_costs && partial_costs[n_layers-2]) )
    {
        layers[n_layers-1]->setLearningRate( grad_learning_rate );
        connections[n_layers-2]->setLearningRate( grad_learning_rate );

        layers[ n_layers-1 ]->bpropUpdate(
                layers[ n_layers-1 ]->activations,
                layers[ n_layers-1 ]->getExpectations(),
                activations_gradients[ n_layers-1 ],
                expectations_gradients[ n_layers-1 ],
                false);

        connections[ n_layers-2 ]->bpropUpdate(
                layers[ n_layers-2 ]->getExpectations(),
                layers[ n_layers-1 ]->activations,
                expectations_gradients[ n_layers-2 ],
                activations_gradients[ n_layers-1 ],
                true);
        // accumulate into expectations_gradients[n_layers-2]
        // because a partial cost may have already put a gradient there
    }

    if( use_classification_cost )
    {
        PLERROR("In DeepBeliefNet::onlineStep - 'use_classification_cost' not "
                "implemented for mini-batches");

        /*
        classification_module->fprop( layers[ n_layers-2 ]->expectation,
                                      class_output );
        real nll_cost;

        // This doesn't work. gcc bug?
        // classification_cost->fprop( class_output, target, cost );
        classification_cost->CostModule::fprop( class_output, target,
                                                nll_cost );

        real class_error =
            ( argmax(class_output) == (int) round(target[0]) ) ? 0: 1;

        train_costs[nll_cost_index] = nll_cost;
        train_costs[class_cost_index] = class_error;

        classification_cost->bpropUpdate( class_output, target, nll_cost,
                                          class_gradient );

        classification_module->bpropUpdate( layers[ n_layers-2 ]->expectation,
                                            class_output,
                                            expectation_gradients[n_layers-2],
                                            class_gradient,
                                            true );
        if( top_layer_joint_cd )
        {
            // set the input of the joint layer
            Vec target_exp = classification_module->target_layer->expectation;
            fill_one_hot( target_exp, (int) round(target[0]), real(0.), real(1.) );

            joint_layer->setLearningRate( cd_learning_rate );
            layers[ n_layers-1 ]->setLearningRate( cd_learning_rate );
            classification_module->joint_connection->setLearningRate(
                cd_learning_rate );

            save_layer_activation.resize(layers[ n_layers-2 ]->size);
            save_layer_activation << layers[ n_layers-2 ]->activation;
            save_layer_expectation.resize(layers[ n_layers-2 ]->size);
            save_layer_expectation << layers[ n_layers-2 ]->expectation;

            contrastiveDivergenceStep(
                get_pointer(joint_layer),
                get_pointer(classification_module->joint_connection),
                layers[ n_layers-1 ], n_layers-2);

            layers[ n_layers-2 ]->activation << save_layer_activation;
            layers[ n_layers-2 ]->expectation << save_layer_expectation;
        }
        */
    }


    // DOWNWARD PHASE (the downward phase for top layer is already done above)
    for( int i=n_layers-3 ; i>=0 ; i-- )
    {
        connections[ i ]->setLearningRate( grad_learning_rate );
        layers[ i+1 ]->setLearningRate( grad_learning_rate );

        layers[i+1]->bpropUpdate( layers[i+1]->activations,
                                  layers[i+1]->getExpectations(),
                                  activations_gradients[i+1],
                                  expectations_gradients[i+1] );

        connections[i]->bpropUpdate( layers[i]->getExpectations(),
                                     layers[i+1]->activations,
                                     expectations_gradients[i],
                                     activations_gradients[i+1],
                                     true);

        // N.B. the contrastiveDivergenceStep changes the activation and
        // expectation fields of top layer of the RBM, so it must be
        // done last
        layers[i]->setLearningRate( cd_learning_rate );
        layers[i+1]->setLearningRate( cd_learning_rate );
        connections[i]->setLearningRate( cd_learning_rate );

        if( i > 0 )
        {
            const Mat& source_act = layers[i]->activations;
            save_layer_activations.resize(source_act.length(),
                                          source_act.width());
            save_layer_activation << source_act;
            const Mat& source_exp = layers[i]->getExpectations();
            save_layer_expectation.resize(source_exp.length(),
                                          source_exp.width());
            save_layer_expectation << source_exp;
        }
        contrastiveDivergenceStep( layers[ i ],
                                   connections[ i ],
                                   layers[ i+1 ] ,
                                   i, true);
        if( i > 0 )
        {
            layers[i]->activations << save_layer_activations;
            layers[i]->getExpectations() << save_layer_expectations;
        }
    }

}

////////////////
// greedyStep //
////////////////
void DeepBeliefNet::greedyStep( const Vec& input, const Vec& target, int index )
{
    PLASSERT( index < n_layers );

    layers[0]->expectation << input;
    for( int i=0 ; i<=index ; i++ )
    {
        connections[i]->setAsDownInput( layers[i]->expectation );
        layers[i+1]->getAllActivations( connections[i] );
        layers[i+1]->computeExpectation();
    }

    // TODO: add another learning rate?
    if( partial_costs && partial_costs[ index ] )
    {
        // put appropriate learning rate
        connections[ index ]->setLearningRate( grad_learning_rate );
        layers[ index+1 ]->setLearningRate( grad_learning_rate );

        // Backward pass
        real cost;
        partial_costs[ index ]->fprop( layers[ index+1 ]->expectation,
                                       target, cost );

        partial_costs[ index ]->bpropUpdate( layers[ index+1 ]->expectation,
                                             target, cost,
                                             expectation_gradients[ index+1 ]
                                             );

        layers[ index+1 ]->bpropUpdate( layers[ index+1 ]->activation,
                                        layers[ index+1 ]->expectation,
                                        activation_gradients[ index+1 ],
                                        expectation_gradients[ index+1 ] );

        connections[ index ]->bpropUpdate( layers[ index ]->expectation,
                                           layers[ index+1 ]->activation,
                                           expectation_gradients[ index ],
                                           activation_gradients[ index+1 ] );

        // put back old learning rate
        connections[ index ]->setLearningRate( cd_learning_rate );
        layers[ index+1 ]->setLearningRate( cd_learning_rate );
    }

    contrastiveDivergenceStep( layers[ index ],
                               connections[ index ],
                               layers[ index+1 ],
                               index, true);
}

/////////////////
// greedySteps //
/////////////////
void DeepBeliefNet::greedyStep( const Mat& inputs, const Mat& targets, int index, Mat& train_costs_m )
{
    PLASSERT( index < n_layers );

    layers[0]->setExpectations(inputs);
    for( int i=0 ; i<=index ; i++ )
    {
        connections[i]->setAsDownInputs( layers[i]->getExpectations() );
        layers[i+1]->getAllActivations( connections[i], 0, true );
        layers[i+1]->computeExpectations();
    }

    // TODO: add another learning rate?
    if( partial_costs && partial_costs[ index ] )
    {
        PLASSERT_MSG(batch_size == 1, "Not implemented for mini-batches");
        /*
        // put appropriate learning rate
        connections[ index ]->setLearningRate( grad_learning_rate );
        layers[ index+1 ]->setLearningRate( grad_learning_rate );

        // Backward pass
        real cost;
        partial_costs[ index ]->fprop( layers[ index+1 ]->expectation,
                                       target, cost );

        partial_costs[ index ]->bpropUpdate( layers[ index+1 ]->expectation,
                                             target, cost,
                                             expectation_gradients[ index+1 ]
                                             );

        layers[ index+1 ]->bpropUpdate( layers[ index+1 ]->activation,
                                        layers[ index+1 ]->expectation,
                                        activation_gradients[ index+1 ],
                                        expectation_gradients[ index+1 ] );

        connections[ index ]->bpropUpdate( layers[ index ]->expectation,
                                           layers[ index+1 ]->activation,
                                           expectation_gradients[ index ],
                                           activation_gradients[ index+1 ] );

        // put back old learning rate
        connections[ index ]->setLearningRate( cd_learning_rate );
        layers[ index+1 ]->setLearningRate( cd_learning_rate );
        */
    }

    contrastiveDivergenceStep( layers[ index ],
                               connections[ index ],
                               layers[ index+1 ],
                               index, true);

    if (reconstruct_layerwise)
    {
        connections[index]->setAsUpInputs(layers[index+1]->getExpectations());
        layers[index]->getAllActivations(connections[index], 0, true);
        layers[index]->fpropNLL(inputs, train_costs_m.column(reconstruction_cost_index+index+1));
        Mat rc = train_costs_m.column(reconstruction_cost_index);
        if (rc.hasMissing())
            rc << train_costs_m.column(reconstruction_cost_index+index+1);
        else
            rc += train_costs_m.column(reconstruction_cost_index+index+1);
    }
}

/////////////////////
// jointGreedyStep //
/////////////////////
void DeepBeliefNet::jointGreedyStep( const Vec& input, const Vec& target )
{
    PLASSERT( joint_layer );
    PLASSERT_MSG(batch_size == 1, "Not implemented for mini-batches");

    layers[0]->expectation << input;
    for( int i=0 ; i<n_layers-2 ; i++ )
    {
        connections[i]->setAsDownInput( layers[i]->expectation );
        layers[i+1]->getAllActivations( connections[i] );
        layers[i+1]->computeExpectation();
    }

    if( partial_costs && partial_costs[ n_layers-2 ] )
    {
        // Deterministic forward pass
        connections[ n_layers-2 ]->setAsDownInput(
            layers[ n_layers-2 ]->expectation );
        layers[ n_layers-1 ]->getAllActivations( connections[ n_layers-2 ] );
        layers[ n_layers-1 ]->computeExpectation();

        // put appropriate learning rate
        connections[ n_layers-2 ]->setLearningRate( grad_learning_rate );
        layers[ n_layers-1 ]->setLearningRate( grad_learning_rate );

        // Backward pass
        real cost;
        partial_costs[ n_layers-2 ]->fprop( layers[ n_layers-1 ]->expectation,
                                            target, cost );

        partial_costs[ n_layers-2 ]->bpropUpdate(
            layers[ n_layers-1 ]->expectation, target, cost,
            expectation_gradients[ n_layers-1 ] );

        layers[ n_layers-1 ]->bpropUpdate( layers[ n_layers-1 ]->activation,
                                           layers[ n_layers-1 ]->expectation,
                                           activation_gradients[ n_layers-1 ],
                                           expectation_gradients[ n_layers-1 ]
                                         );

        connections[ n_layers-2 ]->bpropUpdate(
            layers[ n_layers-2 ]->expectation,
            layers[ n_layers-1 ]->activation,
            expectation_gradients[ n_layers-2 ],
            activation_gradients[ n_layers-1 ] );

        // put back old learning rate
        connections[ n_layers-2 ]->setLearningRate( cd_learning_rate );
        layers[ n_layers-1 ]->setLearningRate( cd_learning_rate );
    }

    Vec target_exp = classification_module->target_layer->expectation;
    fill_one_hot( target_exp, (int) round(target[0]), real(0.), real(1.) );

    contrastiveDivergenceStep(
        get_pointer( joint_layer ),
        get_pointer( classification_module->joint_connection ),
        layers[ n_layers-1 ], n_layers-2);
}

////////////////////
// fineTuningStep //
////////////////////
void DeepBeliefNet::fineTuningStep( const Vec& input, const Vec& target,
                                    Vec& train_costs )
{
    final_cost_value.resize(0);
    // fprop
    layers[0]->expectation << input;
    for( int i=0 ; i<n_layers-2 ; i++ )
    {
        connections[i]->setAsDownInput( layers[i]->expectation );
        layers[i+1]->getAllActivations( connections[i] );
        layers[i+1]->computeExpectation();
    }

    if( final_cost )
    {
        connections[ n_layers-2 ]->setAsDownInput(
            layers[ n_layers-2 ]->expectation );
        layers[ n_layers-1 ]->getAllActivations( connections[ n_layers-2 ] );
        layers[ n_layers-1 ]->computeExpectation();

        if( final_module )
        {
            final_module->fprop( layers[ n_layers-1 ]->expectation,
                                 final_cost_input );
            final_cost->fprop( final_cost_input, target, final_cost_value );

            final_cost->bpropUpdate( final_cost_input, target,
                                     final_cost_value[0],
                                     final_cost_gradient );
            final_module->bpropUpdate( layers[ n_layers-1 ]->expectation,
                                       final_cost_input,
                                       expectation_gradients[ n_layers-1 ],
                                       final_cost_gradient );
        }
        else
        {
            final_cost->fprop( layers[ n_layers-1 ]->expectation, target,
                               final_cost_value );

            final_cost->bpropUpdate( layers[ n_layers-1 ]->expectation,
                                     target, final_cost_value[0],
                                     expectation_gradients[ n_layers-1 ] );
        }

        train_costs.subVec(final_cost_index, final_cost_value.length())
            << final_cost_value;

        layers[ n_layers-1 ]->bpropUpdate( layers[ n_layers-1 ]->activation,
                                           layers[ n_layers-1 ]->expectation,
                                           activation_gradients[ n_layers-1 ],
                                           expectation_gradients[ n_layers-1 ]
                                         );

        connections[ n_layers-2 ]->bpropUpdate(
            layers[ n_layers-2 ]->expectation,
            layers[ n_layers-1 ]->activation,
            expectation_gradients[ n_layers-2 ],
            activation_gradients[ n_layers-1 ] );
    }
    else  {
        expectation_gradients[ n_layers-2 ].clear();
    }

    if( use_classification_cost )
    {
        classification_module->fprop( layers[ n_layers-2 ]->expectation,
                                      class_output );
        real nll_cost;

        // This doesn't work. gcc bug?
        // classification_cost->fprop( class_output, target, cost );
        classification_cost->CostModule::fprop( class_output, target,
                                                nll_cost );

        real class_error =
            ( argmax(class_output) == (int) round(target[0]) ) ? 0
                                                               : 1;

        train_costs[nll_cost_index] = nll_cost;
        train_costs[class_cost_index] = class_error;

        classification_cost->bpropUpdate( class_output, target, nll_cost,
                                          class_gradient );

        classification_module->bpropUpdate( layers[ n_layers-2 ]->expectation,
                                            class_output,
                                            class_input_gradient,
                                            class_gradient );

        expectation_gradients[n_layers-2] += class_input_gradient;
    }

    for( int i=n_layers-2 ; i>0 ; i-- )
    {
        layers[i]->bpropUpdate( layers[i]->activation,
                                layers[i]->expectation,
                                activation_gradients[i],
                                expectation_gradients[i] );

        connections[i-1]->bpropUpdate( layers[i-1]->expectation,
                                       layers[i]->activation,
                                       expectation_gradients[i-1],
                                       activation_gradients[i] );
    }
}

void DeepBeliefNet::fineTuningStep(const Mat& inputs, const Mat& targets,
                                   Mat& train_costs )
{
    final_cost_values.resize(0, 0);
    // fprop
    layers[0]->getExpectations() << inputs;
    for( int i=0 ; i<n_layers-2 ; i++ )
    {
        connections[i]->setAsDownInputs( layers[i]->getExpectations() );
        layers[i+1]->getAllActivations( connections[i], 0, true );
        layers[i+1]->computeExpectations();
    }

    if( final_cost )
    {
        connections[ n_layers-2 ]->setAsDownInputs(
            layers[ n_layers-2 ]->getExpectations() );
        // TODO Also ensure getAllActivations fills everything.
        layers[ n_layers-1 ]->getAllActivations(connections[n_layers-2],
                                                0, true);
        layers[ n_layers-1 ]->computeExpectations();

        if( final_module )
        {
            final_cost_inputs.resize(minibatch_size,
                                     final_module->output_size);
            final_module->fprop( layers[ n_layers-1 ]->getExpectations(),
                                 final_cost_inputs );
            final_cost->fprop( final_cost_inputs, targets, final_cost_values );

            // TODO This extra memory copy is annoying: how can we avoid it?
            optimized_costs << final_cost_values.column(0);
            final_cost->bpropUpdate( final_cost_inputs, targets,
                                     optimized_costs,
                                     final_cost_gradients );
            final_module->bpropUpdate( layers[ n_layers-1 ]->getExpectations(),
                                       final_cost_inputs,
                                       expectations_gradients[ n_layers-1 ],
                                       final_cost_gradients );
        }
        else
        {
            final_cost->fprop( layers[ n_layers-1 ]->getExpectations(), targets,
                               final_cost_values );

            optimized_costs << final_cost_values.column(0);
            final_cost->bpropUpdate( layers[ n_layers-1 ]->getExpectations(),
                                     targets, optimized_costs,
                                     expectations_gradients[ n_layers-1 ] );
        }

        train_costs.subMatColumns(final_cost_index, final_cost_values.width())
            << final_cost_values;

        layers[ n_layers-1 ]->bpropUpdate( layers[ n_layers-1 ]->activations,
                                           layers[ n_layers-1 ]->getExpectations(),
                                           activations_gradients[ n_layers-1 ],
                                           expectations_gradients[ n_layers-1 ]
                                         );

        connections[ n_layers-2 ]->bpropUpdate(
            layers[ n_layers-2 ]->getExpectations(),
            layers[ n_layers-1 ]->activations,
            expectations_gradients[ n_layers-2 ],
            activations_gradients[ n_layers-1 ] );
    }
    else  {
        expectations_gradients[ n_layers-2 ].clear();
    }

    if( use_classification_cost )
    {
        PLERROR("DeepBeliefNet::fineTuningStep - Not implemented for "
                "mini-batches");
        /*
        classification_module->fprop( layers[ n_layers-2 ]->expectation,
                                      class_output );
        real nll_cost;

        // This doesn't work. gcc bug?
        // classification_cost->fprop( class_output, target, cost );
        classification_cost->CostModule::fprop( class_output, target,
                                                nll_cost );

        real class_error =
            ( argmax(class_output) == (int) round(target[0]) ) ? 0
                                                               : 1;

        train_costs[nll_cost_index] = nll_cost;
        train_costs[class_cost_index] = class_error;

        classification_cost->bpropUpdate( class_output, target, nll_cost,
                                          class_gradient );

        classification_module->bpropUpdate( layers[ n_layers-2 ]->expectation,
                                            class_output,
                                            class_input_gradient,
                                            class_gradient );

        expectation_gradients[n_layers-2] += class_input_gradient;
        */
    }

    for( int i=n_layers-2 ; i>0 ; i-- )
    {
        layers[i]->bpropUpdate( layers[i]->activations,
                                layers[i]->getExpectations(),
                                activations_gradients[i],
                                expectations_gradients[i] );

        connections[i-1]->bpropUpdate( layers[i-1]->getExpectations(),
                                       layers[i]->activations,
                                       expectations_gradients[i-1],
                                       activations_gradients[i] );
    }

    // do it AFTER the bprop to avoid interfering with activations used in bprop
    // (and do not worry that the weights have changed a bit)
    if (reconstruct_layerwise)
    {
        Mat rc = train_costs.column(reconstruction_cost_index);
        rc.clear();
        for( int index=0 ; index<n_layers-2 ; index++ )
        {
            layer_inputs.resize(minibatch_size,layers[index]->size);
            layer_inputs << layers[index]->getExpectations();
            connections[index]->setAsUpInputs(layers[index+1]->getExpectations());
            layers[index]->getAllActivations(connections[index], 0, true);
            layers[index]->fpropNLL(layer_inputs, train_costs.column(reconstruction_cost_index+index+1));
            rc += train_costs.column(reconstruction_cost_index+index+1);
        }
    }


}

///////////////////////////////
// contrastiveDivergenceStep //
///////////////////////////////
void DeepBeliefNet::contrastiveDivergenceStep(
    const PP<RBMLayer>& down_layer,
    const PP<RBMConnection>& connection,
    const PP<RBMLayer>& up_layer,
    int layer_index, bool nofprop)
{
    bool mbatch = minibatch_size > 1 || minibatch_hack;

    // positive phase
    if (!nofprop)
    {
        if (mbatch) {
            connection->setAsDownInputs( down_layer->getExpectations() );
            up_layer->getAllActivations( connection, 0, true );
            up_layer->computeExpectations();
        } else {
            connection->setAsDownInput( down_layer->expectation );
            up_layer->getAllActivations( connection );
            up_layer->computeExpectation();
        }
    }

    if (mbatch) {
        // accumulate positive stats using the expectation
        // we deep-copy because the value will change during negative phase
        pos_down_vals.resize(minibatch_size, down_layer->size);
        pos_up_vals.resize(minibatch_size, up_layer->size);

        pos_down_vals << down_layer->getExpectations();
        pos_up_vals << up_layer->getExpectations();

        // down propagation, starting from a sample of up_layer
        if (background_gibbs_update_ratio<1) 
            // then do some contrastive divergence, o/w only background Gibbs
        {
            up_layer->generateSamples();
            connection->setAsUpInputs( up_layer->samples );
            down_layer->getAllActivations( connection, 0, true );
            down_layer->generateSamples();
            // negative phase
            connection->setAsDownInputs( down_layer->samples );
            up_layer->getAllActivations( connection, 0, mbatch );
            up_layer->computeExpectations();

            // accumulate negative stats
            // no need to deep-copy because the values won't change before update
            Mat neg_down_vals = down_layer->samples;
            Mat neg_up_vals = up_layer->getExpectations();

            if (background_gibbs_update_ratio==0)
            // update here only if there is ONLY contrastive divergence
            {
                down_layer->update( pos_down_vals, neg_down_vals );
                connection->update( pos_down_vals, pos_up_vals,
                                    neg_down_vals, neg_up_vals );
                up_layer->update( pos_up_vals, neg_up_vals );
            }
            else
            {
                connection->accumulatePosStats(pos_down_vals,pos_up_vals);
                cd_neg_down_vals.resize(minibatch_size, down_layer->size);
                cd_neg_up_vals.resize(minibatch_size, up_layer->size);
                cd_neg_down_vals << neg_down_vals;
                cd_neg_up_vals << neg_up_vals;
            }
        }
        // 
        if (background_gibbs_update_ratio>0) 
        {
            Mat down_state = gibbs_down_state[layer_index];

            if (initialize_gibbs_chain) // initializing or re-initializing the chain
            {
                if (background_gibbs_update_ratio==1) // if <1 just use the CD state
                {
                    up_layer->generateSamples();
                    connection->setAsUpInputs(up_layer->samples);
                    down_layer->getAllActivations(connection, 0, true);
                    down_layer->generateSamples();
                }
                initialize_gibbs_chain=false;
            }
            else 
            {
                // sample up state given down state
                connection->setAsDownInputs(down_state);
                up_layer->getAllActivations(connection, 0, true);
                up_layer->computeExpectations();
            }
            up_layer->generateSamples();

            // update using the down_state and up_layer->expectations for moving average in negative phase
            // (and optionally 
            if (background_gibbs_update_ratio<1)
            {
                down_layer->updateCDandGibbs(pos_down_vals,cd_neg_down_vals,
                                             down_layer->samples,
                                             background_gibbs_update_ratio,
                                             gibbs_chain_statistics_forgetting_factor);
                connection->updateCDandGibbs(pos_down_vals,pos_up_vals,
                                             cd_neg_down_vals, cd_neg_up_vals,
                                             down_layer->samples,
                                             up_layer->getExpectations(),
                                             background_gibbs_update_ratio,
                                             gibbs_chain_statistics_forgetting_factor);
                up_layer->updateCDandGibbs(pos_up_vals,cd_neg_up_vals,
                                           up_layer->getExpectations(),
                                           background_gibbs_update_ratio,
                                           gibbs_chain_statistics_forgetting_factor);
            }
            else
            {
                down_layer->updateGibbs(pos_down_vals,down_layer->samples,
                                        gibbs_chain_statistics_forgetting_factor);
                connection->updateGibbs(pos_down_vals,pos_up_vals,down_layer->samples,
                                        up_layer->getExpectations(),
                                        gibbs_chain_statistics_forgetting_factor);
                up_layer->updateGibbs(pos_up_vals,up_layer->getExpectations(),
                                        gibbs_chain_statistics_forgetting_factor);
            }
            // sample down state given up state, to prepare for next time
            connection->setAsUpInputs(up_layer->samples);
            down_layer->getAllActivations(connection, 0, true);
            down_layer->generateSamples();
            down_state << down_layer->samples;
        }
    } else {
        up_layer->generateSample();

        // accumulate positive stats using the expectation
        // we deep-copy because the value will change during negative phase
        pos_down_val.resize( down_layer->size );
        pos_up_val.resize( up_layer->size );

        pos_down_val << down_layer->expectation;
        pos_up_val << up_layer->expectation;

        // down propagation, starting from a sample of up_layer
        connection->setAsUpInput( up_layer->sample );

        down_layer->getAllActivations( connection );

        down_layer->generateSample();
        // negative phase
        connection->setAsDownInput( down_layer->sample );
        up_layer->getAllActivations( connection, 0, mbatch );
        up_layer->computeExpectation();
        // accumulate negative stats
        // no need to deep-copy because the values won't change before update
        Vec neg_down_val = down_layer->sample;
        Vec neg_up_val = up_layer->expectation;

        // update
        down_layer->update( pos_down_val, neg_down_val );
        connection->update( pos_down_val, pos_up_val,
                neg_down_val, neg_up_val );
        up_layer->update( pos_up_val, neg_up_val );
    }
}


///////////////////
// computeOutput //
///////////////////
void DeepBeliefNet::computeOutput(const Vec& input, Vec& output) const
{
    // Compute the output from the input.
    output.resize(0);

    // fprop
    layers[0]->expectation << input;

    if(reconstruct_layerwise)
        reconstruction_costs[0]=0;

    for( int i=0 ; i<n_layers-2 ; i++ )
    {
        connections[i]->setAsDownInput( layers[i]->expectation );
        layers[i+1]->getAllActivations( connections[i] );
        layers[i+1]->computeExpectation();

        if (reconstruct_layerwise)
        {
            layer_input.resize(layers[i]->size);
            layer_input << layers[i]->expectation;
            connections[i]->setAsUpInput(layers[i+1]->expectation);
            layers[i]->getAllActivations(connections[i]);
            real rc = reconstruction_costs[i+1] = layers[i]->fpropNLL( layer_input ); 
            reconstruction_costs[0] += rc;
        }
    }


    if( use_classification_cost )
        classification_module->fprop( layers[ n_layers-2 ]->expectation,
                                      output );

    if( final_cost || ( partial_costs && partial_costs[n_layers-2] ))
    {
        connections[ n_layers-2 ]->setAsDownInput(
            layers[ n_layers-2 ]->expectation );
        layers[ n_layers-1 ]->getAllActivations( connections[ n_layers-2 ] );
        layers[ n_layers-1 ]->computeExpectation();

        if( final_module )
        {
            final_module->fprop( layers[ n_layers-1 ]->expectation,
                                 final_cost_input );
            output.append( final_cost_input );
        }
        else
        {
            output.append( layers[ n_layers-1 ]->expectation );
        }

        if (reconstruct_layerwise)
        {
            layer_input.resize(layers[n_layers-2]->size);
            layer_input << layers[n_layers-2]->expectation;
            connections[n_layers-2]->setAsUpInput(layers[n_layers-1]->expectation);
            layers[n_layers-2]->getAllActivations(connections[n_layers-2]);
            real rc = reconstruction_costs[n_layers-1] = layers[n_layers-2]->fpropNLL( layer_input ); 
            reconstruction_costs[0] += rc;
        }
    }
}

void DeepBeliefNet::computeCostsFromOutputs(const Vec& input, const Vec& output,
                                           const Vec& target, Vec& costs) const
{
    // Compute the costs from *already* computed output.
    costs.resize( cost_names.length() );
    costs.fill( MISSING_VALUE );

    // TO MAKE FOR CLEANER CODE INDEPENDENT OF ORDER OF CALLING THIS
    // METHOD AND computeOutput, THIS SHOULD BE IN A REDEFINITION OF computeOutputAndCosts
    if( use_classification_cost )
    {
        classification_cost->CostModule::fprop( output.subVec(0, n_classes),
                target, costs[nll_cost_index] );

        costs[class_cost_index] =
            (argmax(output.subVec(0, n_classes)) == (int) round(target[0]))? 0
            : 1;
    }

    if( final_cost )
    {
        int init = use_classification_cost ? n_classes : 0;
        final_cost->fprop( output.subVec( init, output.size() - init ),
                           target, final_cost_value );

        costs.subVec(final_cost_index, final_cost_value.length())
            << final_cost_value;
    }

    if( partial_costs )
    {
        Vec pcosts;
        for( int i=0 ; i<n_layers-1 ; i++ )
            // propagate into local cost associated to output of layer i+1
            if( partial_costs[ i ] )
            {
                partial_costs[ i ]->fprop( layers[ i+1 ]->expectation,
                                           target, pcosts);

                costs.subVec(partial_costs_indices[i], pcosts.length())
                    << pcosts;
            }
    }

    if (reconstruct_layerwise)
        costs.subVec(reconstruction_cost_index, reconstruction_costs.length())
            << reconstruction_costs;

}

TVec<string> DeepBeliefNet::getTestCostNames() const
{
    // Return the names of the costs computed by computeCostsFromOutputs
    // (these may or may not be exactly the same as what's returned by
    // getTrainCostNames).

    return cost_names;
}

TVec<string> DeepBeliefNet::getTrainCostNames() const
{
    return cost_names;
}


//#####  Helper functions  ##################################################

void DeepBeliefNet::setLearningRate( real the_learning_rate )
{
    for( int i=0 ; i<n_layers-1 ; i++ )
    {
        layers[i]->setLearningRate( the_learning_rate );
        connections[i]->setLearningRate( the_learning_rate );
    }
    layers[n_layers-1]->setLearningRate( the_learning_rate );

    if( use_classification_cost )
    {
        classification_module->joint_connection->setLearningRate(
            the_learning_rate );
        joint_layer->setLearningRate( the_learning_rate );
    }

    if( final_module )
        final_module->setLearningRate( the_learning_rate );

    if( final_cost )
        final_cost->setLearningRate( the_learning_rate );
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
