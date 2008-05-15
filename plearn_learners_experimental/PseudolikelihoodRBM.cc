// -*- C++ -*-

// PseudolikelihoodRBM.cc
//
// Copyright (C) 2008 Hugo Larochelle
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

// Authors: Hugo Larochelle

/*! \file PseudolikelihoodRBM.cc */


#define PL_LOG_MODULE_NAME "PseudolikelihoodRBM"
#include "PseudolikelihoodRBM.h"
#include <plearn_learners/online/RBMLayer.h>
#include <plearn/io/pl_log.h>
#include <plearn/math/TMat_sort.h>

#define minibatch_hack 0 // Do we force the minibatch setting? (debug hack)

namespace PLearn {
using namespace std;

PLEARN_IMPLEMENT_OBJECT(
    PseudolikelihoodRBM,
    "Restricted Boltzmann Machine trained by (generalized) pseudolikelihood.",
    "");

///////////////////
// PseudolikelihoodRBM //
///////////////////
PseudolikelihoodRBM::PseudolikelihoodRBM() :
    learning_rate( 0. ),
    decrease_ct( 0. ),
    cd_learning_rate( 0. ),
    cd_decrease_ct( 0. ),
    cd_n_gibbs( 1 ),
    persistent_cd_weight( 0. ),
    n_gibbs_chains( 1 ),
    use_mean_field_cd( false ),
    denoising_learning_rate( 0. ),
    denoising_decrease_ct( 0. ),
    n_classes( -1 ),
    compute_input_space_nll( false ),
    pseudolikelihood_context_size ( 0 ),
    pseudolikelihood_context_type( "uniform_random" ),
    k_most_correlated( -1 ),
    nll_cost_index( -1 ),
    class_cost_index( -1 ),
    training_cpu_time_cost_index ( -1 ),
    cumulative_training_time_cost_index ( -1 ),
    //cumulative_testing_time_cost_index ( -1 ),
    cumulative_training_time( 0 ),
    //cumulative_testing_time( 0 ),
    log_Z( MISSING_VALUE ),
    Z_is_up_to_date( false )
{
    random_gen = new PRandom();
}

////////////////////
// declareOptions //
////////////////////
void PseudolikelihoodRBM::declareOptions(OptionList& ol)
{
    declareOption(ol, "learning_rate", &PseudolikelihoodRBM::learning_rate,
                  OptionBase::buildoption,
                  "The learning rate used for pseudolikelihood training.\n");

    declareOption(ol, "decrease_ct", &PseudolikelihoodRBM::decrease_ct,
                  OptionBase::buildoption,
                  "The decrease constant of the learning rate.\n");

    declareOption(ol, "cd_learning_rate", &PseudolikelihoodRBM::cd_learning_rate,
                  OptionBase::buildoption,
                  "The learning rate used for contrastive divergence learning.\n");

    declareOption(ol, "cd_decrease_ct", &PseudolikelihoodRBM::cd_decrease_ct,
                  OptionBase::buildoption,
                  "The decrease constant of the contrastive divergence "
                  "learning rate.\n");

    declareOption(ol, "cd_n_gibbs", &PseudolikelihoodRBM::cd_n_gibbs,
                  OptionBase::buildoption,
                  "Number of negative phase gibbs sampling steps.\n");

    declareOption(ol, "persistent_cd_weight", 
                  &PseudolikelihoodRBM::persistent_cd_weight,
                  OptionBase::buildoption,
                  "Weight of Persistent Contrastive Divergence, i.e. "
                  "weight of the prolonged gibbs chain.\n");

    declareOption(ol, "n_gibbs_chains", 
                  &PseudolikelihoodRBM::n_gibbs_chains,
                  OptionBase::buildoption,
                  "Number of gibbs chains maintained in parallel for "
                  "Persistent Contrastive Divergence.\n");

    declareOption(ol, "use_mean_field_cd", &PseudolikelihoodRBM::use_mean_field_cd,
                  OptionBase::buildoption,
                  "Indication that a mean-field version of Contrastive "
                  "Divergence (MF-CD) should be used.\n");

    declareOption(ol, "denoising_learning_rate", 
                  &PseudolikelihoodRBM::denoising_learning_rate,
                  OptionBase::buildoption,
                  "The learning rate used for denoising autoencoder learning.\n");

    declareOption(ol, "denoising_decrease_ct", 
                  &PseudolikelihoodRBM::denoising_decrease_ct,
                  OptionBase::buildoption,
                  "The decrease constant of the denoising autoencoder "
                  "learning rate.\n");

    declareOption(ol, "fraction_of_masked_inputs", 
                  &PseudolikelihoodRBM::fraction_of_masked_inputs,
                  OptionBase::buildoption,
                  "Fraction of input components set to 0 for denoising "
                  "autoencoder learning.\n");

    declareOption(ol, "n_classes", &PseudolikelihoodRBM::n_classes,
                  OptionBase::buildoption,
                  "Number of classes in the training set (for supervised learning).\n"
                  "If < 2, unsupervised learning will be performed.\n"
                  );

    declareOption(ol, "compute_input_space_nll", 
                  &PseudolikelihoodRBM::compute_input_space_nll,
                  OptionBase::buildoption,
                  "Indication that the input space NLL should be "
                  "computed during test.\n"
                  );

    declareOption(ol, "pseudolikelihood_context_size", 
                  &PseudolikelihoodRBM::pseudolikelihood_context_size,
                  OptionBase::buildoption,
                  "Number of additional input variables chosen to form the joint\n"
                  "condition likelihoods in generalized pseudolikelihood\n"
                  "(default = 0, which corresponds to standard pseudolikelihood).\n"
                  );

    declareOption(ol, "pseudolikelihood_context_type", 
                  &PseudolikelihoodRBM::pseudolikelihood_context_type,
                  OptionBase::buildoption,
                  "Type of context for generalized pseudolikelihood:\n"
                  "\"uniform_random\": context elements are picked uniformly randomly\n"
                  "\n"
                  "- \"most_correlated\": the most correlated (positively or negatively\n"
                  "                     elemenst with the current input element are picked\n"
                  "\n"
                  "- \"most_correlated_uniform_random\": context elements are picked uniformly\n"
                  "                                    among the k_most_correlated other input\n"
                  "                                    elements, for each current input\n"
                  );

    declareOption(ol, "k_most_correlated", 
                  &PseudolikelihoodRBM::k_most_correlated,
                  OptionBase::buildoption,
                  "Number of most correlated input elements over which to sample.\n"
                  );

    declareOption(ol, "input_layer", &PseudolikelihoodRBM::input_layer,
                  OptionBase::buildoption,
                  "The binomial input layer of the RBM.\n");

    declareOption(ol, "hidden_layer", &PseudolikelihoodRBM::hidden_layer,
                  OptionBase::buildoption,
                  "The hidden layer of the RBM.\n");

    declareOption(ol, "connection", &PseudolikelihoodRBM::connection,
                  OptionBase::buildoption,
                  "The connection weights between the input and hidden layer.\n");

    declareOption(ol, "cumulative_training_time", 
                  &PseudolikelihoodRBM::cumulative_training_time,
                  //OptionBase::learntoption | OptionBase::nosave,
                  OptionBase::learntoption,
                  "Cumulative training time since age=0, in seconds.\n");

//    declareOption(ol, "cumulative_testing_time", 
//                  &PseudolikelihoodRBM::cumulative_testing_time,
//                  //OptionBase::learntoption | OptionBase::nosave,
//                  OptionBase::learntoption,
//                  "Cumulative testing time since age=0, in seconds.\n");

    declareOption(ol, "log_Z", &PseudolikelihoodRBM::log_Z,
                  OptionBase::learntoption,
                  "Normalisation constant (on log scale).\n");

    declareOption(ol, "Z_is_up_to_date", &PseudolikelihoodRBM::Z_is_up_to_date,
                  OptionBase::learntoption,
                  "Indication that the normalisation constant Z is up to date.\n");

    declareOption(ol, "persistent_gibbs_chain_is_started", 
                  &PseudolikelihoodRBM::persistent_gibbs_chain_is_started,
                  OptionBase::learntoption,
                  "Indication that the prolonged gibbs chain for "
                  "Persistent Consistent Divergence is started, for each chain.\n");

//    declareOption(ol, "target_weights_L1_penalty_factor", 
//                  &PseudolikelihoodRBM::target_weights_L1_penalty_factor,
//                  OptionBase::buildoption,
//                  "Target weights' L1_penalty_factor.\n");
//
//    declareOption(ol, "target_weights_L2_penalty_factor", 
//                  &PseudolikelihoodRBM::target_weights_L2_penalty_factor,
//                  OptionBase::buildoption,
//                  "Target weights' L2_penalty_factor.\n");

    // Now call the parent class' declareOptions
    inherited::declareOptions(ol);
}

////////////
// build_ //
////////////
void PseudolikelihoodRBM::build_()
{
    MODULE_LOG << "build_() called" << endl;

    if( inputsize_ > 0 && targetsize_ >= 0)
    {
        if( n_classes > 1 && targetsize_ != 1 )
            PLERROR("In PseudolikelihoodRBM::build_(): can't use supervised "
                "learning (n_classes > 1) if there is no target field "
                "(targetsize() != 1)");
        
        if( compute_input_space_nll && n_classes > 1 )
            PLERROR("In PseudolikelihoodRBM::build_(): compute_input_space_nll "
                    "is not compatible with n_classes > 1");

        if( pseudolikelihood_context_size < 0 )
            PLERROR("In PseudolikelihoodRBM::build_(): "
                    "pseudolikelihood_context_size should be >= 0.");

        if( pseudolikelihood_context_type != "uniform_random" &&
            pseudolikelihood_context_type != "most_correlated" &&
            pseudolikelihood_context_type != "most_correlated_uniform_random" )
            PLERROR("In PseudolikelihoodRBM::build_(): "
                    "pseudolikelihood_context_type is not valid.");

        if( pseudolikelihood_context_type == "most_correlated"
            && pseudolikelihood_context_size <= 0 )
            PLERROR("In PseudolikelihoodRBM::train(): "
                    "pseudolikelihood_context_size should be > 0 "
                    "for \"most_correlated\" context type");        

        build_layers_and_connections();
        build_costs();

        // Activate the profiler
        Profiler::activate();
    }
}

/////////////////
// build_costs //
/////////////////
void PseudolikelihoodRBM::build_costs()
{
    cost_names.resize(0);
    
    int current_index = 0;
    if( compute_input_space_nll || n_classes > 1 )
    {
        cost_names.append("NLL");
        nll_cost_index = current_index;
        current_index++;
    }
    
    if( n_classes > 1 )
    {
        cost_names.append("class_error");
        class_cost_index = current_index;
        current_index++;
    }

    cost_names.append("cpu_time");
    cost_names.append("cumulative_train_time");
    //cost_names.append("cumulative_test_time");

    training_cpu_time_cost_index = current_index;
    current_index++;
    cumulative_training_time_cost_index = current_index;
    current_index++;
    //cumulative_testing_time_cost_index = current_index;
    //current_index++;


    PLASSERT( current_index == cost_names.length() );
}

//////////////////////////////////
// build_layers_and_connections //
//////////////////////////////////
void PseudolikelihoodRBM::build_layers_and_connections()
{
    MODULE_LOG << "build_layers_and_connections() called" << endl;

    if( !input_layer )
        PLERROR("In PseudolikelihoodRBM::build_layers_and_connections(): "
                "input_layer must be provided");
    if( !hidden_layer )
        PLERROR("In PseudolikelihoodRBM::build_layers_and_connections(): "
                "hidden_layer must be provided");

    if( !connection )
        PLERROR("PseudolikelihoodRBM::build_layers_and_connections(): \n"
                "connection must be provided");

    if( connection->up_size != hidden_layer->size ||
        connection->down_size != input_layer->size )
        PLERROR("PseudolikelihoodRBM::build_layers_and_connections(): \n"
                "connection's size (%d x %d) should be %d x %d",
                connection->up_size, connection->down_size,
                hidden_layer->size, input_layer->size);

    input_gradient.resize( input_layer->size );
    hidden_activation_pos_i.resize( hidden_layer->size );
    hidden_activation_neg_i.resize( hidden_layer->size );
    hidden_activation_gradient.resize( hidden_layer->size );
    hidden_activation_pos_i_gradient.resize( hidden_layer->size );
    hidden_activation_neg_i_gradient.resize( hidden_layer->size );
    connection_gradient.resize( connection->up_size, connection->down_size );

    // Generalized pseudolikelihood option
    context_indices.resize( input_layer->size - 1);
    if( pseudolikelihood_context_size > 0 )
    {
        context_indices_per_i.resize( input_layer->size, 
                                      pseudolikelihood_context_size );

        int n_conf = ipow(2, pseudolikelihood_context_size);
        nums_act.resize( 2 * n_conf );
        gnums_act.resize( 2 * n_conf );
        context_probs.resize( 2 * n_conf );
        hidden_activations_context.resize( 2*n_conf, hidden_layer->size );
        hidden_activations_context_k_gradient.resize( hidden_layer->size );
    }

    // CD option
    pos_hidden.resize( hidden_layer->size );
    pers_cd_input.resize( n_gibbs_chains );
    pers_cd_hidden.resize( n_gibbs_chains );
    for( int i=0; i<n_gibbs_chains; i++ )
    {
        pers_cd_input[i].resize( input_layer->size );
        pers_cd_hidden[i].resize( hidden_layer->size );
    }
    if( persistent_gibbs_chain_is_started.length() != n_gibbs_chains )
    {
        persistent_gibbs_chain_is_started.resize( n_gibbs_chains );
        persistent_gibbs_chain_is_started.fill( false );
    }

    // Denoising autoencoder options
    transpose_connection = new RBMMatrixTransposeConnection;
    transpose_connection->rbm_matrix_connection = connection;
    transpose_connection->build();
    reconstruction_activation_gradient.resize(input_layer->size);
    hidden_layer_expectation_gradient.resize(hidden_layer->size);
    hidden_layer_activation_gradient.resize(hidden_layer->size);
    masked_autoencoder_input.resize(input_layer->size);
    autoencoder_input_indices.resize(input_layer->size);
    for(int i=0; i<input_layer->size; i++)
        autoencoder_input_indices[i] = i;

    if( inputsize_ >= 0 )
        PLASSERT( input_layer->size == inputsize() );

    if( n_classes > 1 )
    {
        class_output.resize( n_classes );
        before_class_output.resize( n_classes );
        class_gradient.resize( n_classes );
        target_one_hot.resize( n_classes );
    }

    if( !input_layer->random_gen )
    {
        input_layer->random_gen = random_gen;
        input_layer->forget();
    }

    if( !hidden_layer->random_gen )
    {
        hidden_layer->random_gen = random_gen;
        hidden_layer->forget();
    }

    if( !connection->random_gen )
    {
        connection->random_gen = random_gen;
        connection->forget();
    }
}

///////////
// build //
///////////
void PseudolikelihoodRBM::build()
{
    inherited::build();
    build_();
}

/////////////////////////////////
// makeDeepCopyFromShallowCopy //
/////////////////////////////////
void PseudolikelihoodRBM::makeDeepCopyFromShallowCopy(CopiesMap& copies)
{
    inherited::makeDeepCopyFromShallowCopy(copies);

    deepCopyField(input_layer, copies);
    deepCopyField(hidden_layer, copies);
    deepCopyField(connection, copies);
    deepCopyField(cost_names, copies);
    deepCopyField(transpose_connection, copies);

    deepCopyField(target_one_hot, copies);
    deepCopyField(input_gradient, copies);
    deepCopyField(class_output, copies);
    deepCopyField(before_class_output, copies);
    deepCopyField(class_gradient, copies);
    deepCopyField(hidden_activation_pos_i, copies);
    deepCopyField(hidden_activation_neg_i, copies);
    deepCopyField(hidden_activation_gradient, copies);
    deepCopyField(hidden_activation_pos_i_gradient, copies);
    deepCopyField(hidden_activation_neg_i_gradient, copies);
    deepCopyField(connection_gradient, copies);
    deepCopyField(context_indices, copies);
    deepCopyField(context_indices_per_i, copies);
    deepCopyField(correlations_per_i, copies);
    deepCopyField(context_most_correlated, copies);
    deepCopyField(hidden_activations_context, copies);
    deepCopyField(hidden_activations_context_k_gradient, copies);
    deepCopyField(nums, copies);
    deepCopyField(nums_act, copies);
    deepCopyField(context_probs, copies);
    deepCopyField(gnums_act, copies);
    deepCopyField(conf, copies);
    deepCopyField(pos_input, copies);
    deepCopyField(pos_hidden, copies);
    deepCopyField(neg_input, copies);
    deepCopyField(neg_hidden, copies);
    deepCopyField(reconstruction_activation_gradient, copies);
    deepCopyField(hidden_layer_expectation_gradient, copies);
    deepCopyField(hidden_layer_activation_gradient, copies);
    deepCopyField(masked_autoencoder_input, copies);
    deepCopyField(autoencoder_input_indices, copies);
    deepCopyField(pers_cd_input, copies);
    deepCopyField(pers_cd_hidden, copies);
    deepCopyField(persistent_gibbs_chain_is_started, copies);
}


////////////////
// outputsize //
////////////////
int PseudolikelihoodRBM::outputsize() const
{
    return n_classes > 1 ? n_classes : hidden_layer->size;
}

////////////
// forget //
////////////
void PseudolikelihoodRBM::forget()
{
    inherited::forget();

    input_layer->forget();
    hidden_layer->forget();
    connection->forget();

    cumulative_training_time = 0;
    //cumulative_testing_time = 0;
    Z_is_up_to_date = false;

    persistent_gibbs_chain_is_started.fill( false );
    correlations_per_i.resize(0,0);
}

///////////
// train //
///////////
void PseudolikelihoodRBM::train()
{
    MODULE_LOG << "train() called " << endl;

    MODULE_LOG << "stage = " << stage
        << ", target nstages = " << nstages << endl;

    PLASSERT( train_set );

    Vec input( inputsize() );
    Vec target( targetsize() );
    int target_index;
    real weight; // unused
    real lr;

    TVec<string> train_cost_names = getTrainCostNames() ;
    Vec train_costs( train_cost_names.length() );
    train_costs.fill(MISSING_VALUE) ;

    int nsamples = train_set->length();
    int init_stage = stage;
    if( !initTrain() )
    {
        MODULE_LOG << "train() aborted" << endl;
        return;
    }

    PP<ProgressBar> pb;

    // clear stats of previous epoch
    train_stats->forget();

    if( report_progress )
        pb = new ProgressBar( "Training "
                              + classname(),
                              nstages - stage );

    // Start the actual time counting
    Profiler::reset("training");
    Profiler::start("training");

    for( ; stage<nstages ; stage++ )
    {
        Z_is_up_to_date = false;
        train_set->getExample(stage%nsamples, input, target, weight);

        if( pb )
            pb->update( stage - init_stage + 1 );

        if( n_classes > 1 )
        {
            target_one_hot.clear();
            if( !is_missing(target[0]) )
            {
                target_index = (int)round( target[0] );
                target_one_hot[ target_index ] = 1;
            }
            PLERROR("In PseudolikelihoodRBM::train(): supervised learning "
                    "not implemented yet.");

            if( decrease_ct != 0 )
                lr = learning_rate / (1.0 + stage * decrease_ct );
            else
                lr = learning_rate;

            setLearningRate(lr);
        }
        else
        {
            if( !fast_is_equal(learning_rate, 0.) )
            {
                if( decrease_ct != 0 )
                    lr = learning_rate / (1.0 + stage * decrease_ct );
                else
                    lr = learning_rate;

                setLearningRate(lr);

                if( pseudolikelihood_context_size == 0 )
                {
                    // Compute input_probs
                    //
                    //a = W x + c
                    //  for i in 1...d
                    //      num_pos = b_i
                    //      num_neg = 0
                    //      for j in 1...h
                    //          num_pos += softplus( a_j - W_ji x_i + W_ji)
                    //          num_neg += softplus( a_j - W_ji x_i)
                    //      p_i = exp(num_pos) / (exp(num_pos) + exp(num_neg))

                    connection->setAsDownInput( input );
                    hidden_layer->getAllActivations( 
                        (RBMMatrixConnection*) connection );

                    real num_pos_act;
                    real num_neg_act;
                    real num_pos;
                    real num_neg;
                    real* a = hidden_layer->activation.data();
                    real* a_pos_i = hidden_activation_pos_i.data();
                    real* a_neg_i = hidden_activation_neg_i.data();
                    real* w, *gw;
                    int m = connection->weights.mod();
                    real input_i, input_probs_i;
                    real pseudolikelihood = 0;
                    real* ga_pos_i = hidden_activation_pos_i_gradient.data();
                    real* ga_neg_i = hidden_activation_neg_i_gradient.data();
                    hidden_activation_gradient.clear();
                    connection_gradient.clear();
                    for( int i=0; i<input_layer->size ; i++ )
                    {
                        num_pos_act = input_layer->bias[i];
                        num_neg_act = 0;
                        w = &(connection->weights(0,i));
                        input_i = input[i];
                        for( int j=0; j<hidden_layer->size; j++,w+=m )
                        {
                            a_pos_i[j] = a[j] - *w * ( input_i - 1 );
                            a_neg_i[j] = a[j] - *w * input_i;
                        }
                        num_pos_act -= hidden_layer->freeEnergyContribution(
                            hidden_activation_pos_i);
                        num_neg_act -= hidden_layer->freeEnergyContribution(
                            hidden_activation_neg_i);
                        //num_pos = safeexp(num_pos_act);
                        //num_neg = safeexp(num_neg_act);
                        //input_probs_i = num_pos / (num_pos + num_neg);
                        if( input_layer->use_fast_approximations )
                            input_probs_i = fastsigmoid(
                                num_pos_act - num_neg_act);
                        else
                        {
                            num_pos = safeexp(num_pos_act);
                            num_neg = safeexp(num_neg_act);
                            input_probs_i = num_pos / (num_pos + num_neg);
                        }

                        // Compute input_prob gradient
                        if( input_layer->use_fast_approximations )
                            pseudolikelihood += tabulated_softplus( 
                                num_pos_act - num_neg_act ) 
                                - input_i * (num_pos_act - num_neg_act);
                        else
                            pseudolikelihood += softplus( 
                                num_pos_act - num_neg_act ) 
                                - input_i * (num_pos_act - num_neg_act);
                        input_gradient[i] = input_probs_i - input_i;

                        hidden_layer->freeEnergyContributionGradient(
                            hidden_activation_pos_i,
                            hidden_activation_pos_i_gradient,
                            -input_gradient[i],
                            false);
                        hidden_activation_gradient += hidden_activation_pos_i_gradient;

                        hidden_layer->freeEnergyContributionGradient(
                            hidden_activation_neg_i,
                            hidden_activation_neg_i_gradient,
                            input_gradient[i],
                            false);
                        hidden_activation_gradient += hidden_activation_neg_i_gradient;

                        gw = &(connection_gradient(0,i));
                        for( int j=0; j<hidden_layer->size; j++,gw+=m )
                        {
                            *gw -= ga_pos_i[j] * ( input_i - 1 );
                            *gw -= ga_neg_i[j] * input_i;
                        }
                    }

                    externalProductAcc( connection_gradient, hidden_activation_gradient,
                                        input );

                    // Hidden bias update
                    multiplyScaledAdd(hidden_activation_gradient, 1.0, -lr,
                                      hidden_layer->bias);
                    // Connection weights update
                    multiplyScaledAdd( connection_gradient, 1.0, -lr,
                                       connection->weights );
                    // Input bias update
                    multiplyScaledAdd(input_gradient, 1.0, -lr,
                                      input_layer->bias);

                    // N.B.: train costs contains pseudolikelihood
                    //       or pseudoNLL, not NLL
                    train_costs[nll_cost_index] = pseudolikelihood;

//                    cout << "input_gradient: " << input_gradient << endl;
//                    cout << "hidden_activation_gradient" << hidden_activation_gradient << endl;

                }
                else
                {
                    if( ( pseudolikelihood_context_type == "most_correlated" ||
                          pseudolikelihood_context_type == "most_correlated_uniform_random" )
                        && correlations_per_i.length() == 0 )
                    {
                        Vec corr_input(inputsize());
                        Vec corr_target(targetsize());
                        real corr_weight;
                        Vec mean(inputsize());
                        mean.clear();
                        for(int t=0; t<train_set->length(); t++)
                        {
                            train_set->getExample(t,corr_input,corr_target,
                                                  corr_weight);
                            mean += corr_input;
                        }
                        mean /= train_set->length();
                        
                        correlations_per_i.resize(inputsize(),inputsize());
                        correlations_per_i.clear();
                        Mat cov(inputsize(), inputsize());
                        cov.clear();
                        for(int t=0; t<train_set->length(); t++)
                        {
                            train_set->getExample(t,corr_input,corr_target,
                                                  corr_weight);
                            corr_input -= mean;
                            externalProductAcc(cov,
                                               corr_input,corr_input);
                        }
                        //correlations_per_i /= train_set->length();

                        for( int i=0; i<inputsize(); i++ )
                            for( int j=0; j<inputsize(); j++)
                            {
                                correlations_per_i(i,j) = 
                                    abs(cov(i,j)) 
                                    / sqrt(cov(i,i)*cov(j,j));
                            }

                        if( pseudolikelihood_context_type == "most_correlated")
                        {
                            if( pseudolikelihood_context_size <= 0 )
                                PLERROR("In PseudolikelihoodRBM::train(): "
                                    "pseudolikelihood_context_size should be > 0 "
                                    "for \"most_correlated\" context type");
                            real current_min;
                            int current_min_position;
                            real* corr;
                            int* context;
                            Vec context_corr(pseudolikelihood_context_size);
                            context_indices_per_i.resize(
                                inputsize(),
                                pseudolikelihood_context_size);

                            // HUGO: this is quite inefficient for big 
                            // pseudolikelihood_context_sizes, should use a heap
                            for( int i=0; i<inputsize(); i++ )
                            {
                                current_min = REAL_MAX;
                                current_min_position = -1;
                                corr = correlations_per_i[i];
                                context = context_indices_per_i[i];
                                for( int j=0; j<inputsize(); j++ )
                                {
                                    if( i == j )
                                        continue;

                                    // Filling first pseudolikelihood_context_size elements
                                    if( j - (j>i?1:0) < pseudolikelihood_context_size )
                                    {
                                        context[j - (j>i?1:0)] = j;
                                        context_corr[j - (j>i?1:0)] = corr[j];
                                        if( current_min > corr[j] )
                                        {
                                            current_min = corr[j];
                                            current_min_position = j - (j>i?1:0);
                                        }
                                        continue;
                                    }

                                    if( corr[j] > current_min )
                                    {
                                        context[current_min_position] = j;
                                        context_corr[current_min_position] = corr[j];
                                        current_min = 
                                            min( context_corr, 
                                                 current_min_position );
                                    }
                                }
                            }
                        }
                        
                        if( pseudolikelihood_context_type == 
                            "most_correlated_uniform_random" )
                        {
                            if( k_most_correlated < 
                                pseudolikelihood_context_size )
                                PLERROR("In PseudolikelihoodRBM::train(): "
                                        "k_most_correlated should be "
                                        ">= pseudolikelihood_context_size");

                            if( k_most_correlated > inputsize() - 1 )
                                PLERROR("In PseudolikelihoodRBM::train(): "
                                        "k_most_correlated should be "
                                        "< inputsize()");

                            real current_min;
                            int current_min_position;
                            real* corr;
                            int* context;
                            Vec context_corr( k_most_correlated );
                            context_most_correlated.resize( inputsize() );

                            // HUGO: this is quite inefficient for big 
                            // pseudolikelihood_context_sizes, should use a heap
                            for( int i=0; i<inputsize(); i++ )
                            {
                                context_most_correlated[i].resize( 
                                    k_most_correlated );
                                current_min = REAL_MAX;
                                current_min_position = -1;
                                corr = correlations_per_i[i];
                                context = context_most_correlated[i].data();
                                for( int j=0; j<inputsize(); j++ )
                                {
                                    if( i == j )
                                        continue;

                                    // Filling first k_most_correlated elements
                                    if( j - (j>i?1:0) <  k_most_correlated )
                                    {
                                        context[j - (j>i?1:0)] = j;
                                        context_corr[j - (j>i?1:0)] = corr[j];
                                        if( current_min > corr[j] )
                                        {
                                            current_min = corr[j];
                                            current_min_position = j - (j>i?1:0);
                                        }
                                        continue;
                                    }

                                    if( corr[j] > current_min )
                                    {
                                        context[current_min_position] = j;
                                        context_corr[current_min_position] = corr[j];
                                        current_min = 
                                            min( context_corr, 
                                                 current_min_position );
                                    }
                                }
                            }
                        }                        
                    }

                    if( pseudolikelihood_context_type == "uniform_random" ||
                        pseudolikelihood_context_type == "most_correlated_uniform_random" )
                    {
                        // Generate contexts
                        if( pseudolikelihood_context_type == "uniform_random" )
                            for( int i=0; i<context_indices.length(); i++)
                                context_indices[i] = i;
                        int tmp,k;
                        int* c;
                        int n;
                        if( pseudolikelihood_context_type == "uniform_random" )
                        {
                            c = context_indices.data();
                            n = input_layer->size-1;
                        }
                        int* ci;
                        for( int i=0; i<context_indices_per_i.length(); i++)
                        {
                            if( pseudolikelihood_context_type == 
                                "most_correlated_uniform_random" )
                            {
                                c = context_most_correlated[i].data();
                                n = context_most_correlated[i].length();
                            }

                            ci = context_indices_per_i[i];
                            for (int j = 0; j < context_indices_per_i.width(); j++) 
                            {
                                k = j + 
                                    random_gen->uniform_multinomial_sample(n - j);
                                
                                tmp = c[j];
                                c[j] = c[k];
                                c[k] = tmp;

                                if( pseudolikelihood_context_type 
                                    == "uniform_random" )
                                {
                                    if( c[j] >= i )
                                        ci[j] = c[j]+1;
                                    else
                                        ci[j] = c[j];
                                }

                                if( pseudolikelihood_context_type == 
                                    "most_correlated_uniform_random" )
                                    ci[j] = c[j];
                            }
                        }
                    }

                    connection->setAsDownInput( input );
                    hidden_layer->getAllActivations( 
                        (RBMMatrixConnection *) connection );

                    int n_conf = ipow(2, pseudolikelihood_context_size);
                    //nums_act.resize( 2 * n_conf );
                    //gnums_act.resize( 2 * n_conf );
                    //context_probs.resize( 2 * n_conf );
                    //hidden_activations_context.resize( 2*n_conf, hidden_layer->size );
                    //hidden_activations_context_k_gradient.resize( hidden_layer->size );
                    real* nums_data;
                    real* gnums_data;
                    real* cp_data;
                    real* a = hidden_layer->activation.data();
                    real* w, *gw, *gi, *ac, *bi, *gac;
                    int* context_i;
                    int m;
                    int conf_index;
                    real input_i, input_j,  log_Zi;
                    real pseudolikelihood = 0;

                    input_gradient.clear();
                    hidden_activation_gradient.clear();
                    connection_gradient.clear();
                    gi = input_gradient.data();
                    bi = input_layer->bias.data();
                    for( int i=0; i<input_layer->size ; i++ )
                    {
                        nums_data = nums_act.data();
                        cp_data = context_probs.data();
                        input_i = input[i];

                        m = connection->weights.mod();
                        // input_i = 1
                        for( int k=0; k<n_conf; k++)
                        {
                            *nums_data = bi[i];
                            *cp_data = input_i;
                            conf_index = k;
                            ac = hidden_activations_context[k];

                            w = &(connection->weights(0,i));
                            for( int j=0; j<hidden_layer->size; j++,w+=m )
                                ac[j] = a[j] - *w * ( input_i - 1 );

                            context_i = context_indices_per_i[i];
                            for( int l=0; l<pseudolikelihood_context_size; l++ )
                            {
                                input_j = input[*context_i];
                                w = &(connection->weights(0,*context_i));
                                if( conf_index & 1)
                                {
                                    *cp_data *= input_j;
                                    *nums_data += bi[*context_i];
                                    for( int j=0; j<hidden_layer->size; j++,w+=m )
                                        ac[j] -=  *w * ( input_j - 1 );
                                }
                                else
                                {
                                    *cp_data *= (1-input_j);
                                    for( int j=0; j<hidden_layer->size; j++,w+=m )
                                        ac[j] -=  *w * input_j;
                                }

                                conf_index >>= 1;
                                context_i++;
                            }
                            *nums_data -= hidden_layer->freeEnergyContribution(
                                hidden_activations_context(k));
                            nums_data++;
                            cp_data++;
                        }

                        // input_i = 0
                        for( int k=0; k<n_conf; k++)
                        {
                            *nums_data = 0;
                            *cp_data = (1-input_i);
                            conf_index = k;
                            ac = hidden_activations_context[n_conf + k];
                        
                            w = &(connection->weights(0,i));
                            for( int j=0; j<hidden_layer->size; j++,w+=m )
                                ac[j] = a[j] - *w * input_i;

                            context_i = context_indices_per_i[i];
                            for( int l=0; l<pseudolikelihood_context_size; l++ )
                            {
                                w = &(connection->weights(0,*context_i));
                                input_j = input[*context_i];
                                if( conf_index & 1)
                                {
                                    *cp_data *= input_j;
                                    *nums_data += bi[*context_i];
                                    for( int j=0; j<hidden_layer->size; j++,w+=m )
                                        ac[j] -=  *w * ( input_j - 1 );
                                }
                                else
                                {
                                    *cp_data *= (1-input_j);
                                    for( int j=0; j<hidden_layer->size; j++,w+=m )
                                        ac[j] -=  *w * input_j;
                                }

                                conf_index >>= 1;
                                context_i++;
                            }
                            *nums_data -= hidden_layer->freeEnergyContribution(
                                hidden_activations_context(n_conf + k));
                            nums_data++;
                            cp_data++;
                        }
                    

                        // Gradient computation
                        //exp( nums_act, nums);
                        //Zi = sum(nums);
                        //log_Zi = pl_log(Zi);
                        log_Zi = logadd(nums_act);

                        nums_data = nums_act.data();
                        gnums_data = gnums_act.data();
                        cp_data = context_probs.data();

                        // Compute input_prob gradient

                        m = connection_gradient.mod();
                        // input_i = 1                    
                        for( int k=0; k<n_conf; k++)
                        {
                            pseudolikelihood -= *cp_data * (*nums_data - log_Zi);
                            *gnums_data = (safeexp(*nums_data - log_Zi) - *cp_data);
                            gi[i] += *gnums_data;
                        
                            hidden_layer->freeEnergyContributionGradient(
                                hidden_activations_context(k),
                                hidden_activations_context_k_gradient,
                                -*gnums_data,
                                false);
                            hidden_activation_gradient += 
                                hidden_activations_context_k_gradient;
                        
                            gac = hidden_activations_context_k_gradient.data();
                            gw = &(connection_gradient(0,i));
                            for( int j=0; j<hidden_layer->size; j++,gw+=m )
                                *gw -= gac[j] * ( input_i - 1 );

                            context_i = context_indices_per_i[i];
                            for( int l=0; l<pseudolikelihood_context_size; l++ )
                            {
                                gw = &(connection_gradient(0,*context_i));
                                input_j = input[*context_i];
                                if( conf_index & 1)
                                {
                                    gi[*context_i] += *gnums_data;
                                    for( int j=0; j<hidden_layer->size; j++,gw+=m )
                                        *gw -= gac[j] * ( input_j - 1 );
                                }
                                else
                                {
                                    for( int j=0; j<hidden_layer->size; j++,gw+=m )
                                        *gw -= gac[j] * input_j;
                                }
                                conf_index >>= 1;
                                context_i++;
                            }

                            nums_data++;
                            gnums_data++;
                            cp_data++;
                        }

                        // input_i = 0
                        for( int k=0; k<n_conf; k++)
                        {
                            pseudolikelihood -= *cp_data * (*nums_data - log_Zi);
                            *gnums_data = (safeexp(*nums_data - log_Zi) - *cp_data);
                        
                            hidden_layer->freeEnergyContributionGradient(
                                hidden_activations_context(n_conf + k),
                                hidden_activations_context_k_gradient,
                                -*gnums_data,
                                false);
                            hidden_activation_gradient += 
                                hidden_activations_context_k_gradient;
                        
                            gac = hidden_activations_context_k_gradient.data();
                            gw = &(connection_gradient(0,i));
                            for( int j=0; j<hidden_layer->size; j++,gw+=m )
                                *gw -= gac[j] *input_i;

                            context_i = context_indices_per_i[i];
                            for( int l=0; l<pseudolikelihood_context_size; l++ )
                            {
                                gw = &(connection_gradient(0,*context_i));
                                input_j = input[*context_i];
                                if( conf_index & 1)
                                {
                                    gi[*context_i] += *gnums_data;
                                    for( int j=0; j<hidden_layer->size; j++,gw+=m )
                                        *gw -= gac[j] * ( input_j - 1 );
                                }
                                else
                                {
                                    for( int j=0; j<hidden_layer->size; j++,gw+=m )
                                        *gw -= gac[j] * input_j;
                                }

                                conf_index >>= 1;
                                context_i++;
                            }

                            nums_data++;
                            gnums_data++;
                            cp_data++;
                        }
                    }

//                    cout << "input_gradient: " << input_gradient << endl;
//                    cout << "hidden_activation_gradient" << hidden_activation_gradient << endl;

                    externalProductAcc( connection_gradient, hidden_activation_gradient,
                                        input );

                    // Hidden bias update
                    multiplyScaledAdd(hidden_activation_gradient, 1.0, -lr,
                                      hidden_layer->bias);
                    // Connection weights update
                    multiplyScaledAdd( connection_gradient, 1.0, -lr,
                                       connection->weights );
                    // Input bias update
                    multiplyScaledAdd(input_gradient, 1.0, -lr,
                                      input_layer->bias);

                    // N.B.: train costs contains pseudolikelihood
                    //       or pseudoNLL, not NLL
                    train_costs[nll_cost_index] = pseudolikelihood;
                }
            }
            
            // CD learning
            if( !fast_is_equal(cd_learning_rate, 0.) )
            {

                if( !fast_is_equal(persistent_cd_weight, 1.) )
                {
                    if( cd_decrease_ct != 0 )
                        lr = cd_learning_rate / (1.0 + stage * cd_decrease_ct );
                    else
                        lr = cd_learning_rate;
                    
                    lr *= (1-persistent_cd_weight);

                    setLearningRate(lr);

                    // Positive phase
                    pos_input = input;
                    connection->setAsDownInput( input );
                    hidden_layer->getAllActivations( 
                        (RBMMatrixConnection*) connection );
                    hidden_layer->computeExpectation();
                    //pos_hidden.resize( hidden_layer->size );
                    pos_hidden << hidden_layer->expectation;
                    
                    // Negative phase
                    for(int i=0; i<cd_n_gibbs; i++)
                    {
                        if( use_mean_field_cd )
                        {
                            connection->setAsUpInput( hidden_layer->expectation );
                        }
                        else
                        {
                            hidden_layer->generateSample();
                            connection->setAsUpInput( hidden_layer->sample );
                        }
                        input_layer->getAllActivations( 
                            (RBMMatrixConnection*) connection );
                        input_layer->computeExpectation();
                        if( use_mean_field_cd )
                        {
                            connection->setAsDownInput( input_layer->expectation );
                        }
                        else
                        {
                            input_layer->generateSample();
                            connection->setAsDownInput( input_layer->sample );
                        }
                        hidden_layer->getAllActivations( 
                            (RBMMatrixConnection*) connection );
                        hidden_layer->computeExpectation();
                    }
                    
                    if( use_mean_field_cd )
                        neg_input = input_layer->expectation;
                    else
                        neg_input = input_layer->sample;
                    neg_hidden = hidden_layer->expectation;
                    
                    input_layer->update(pos_input,neg_input);
                    hidden_layer->update(pos_hidden,neg_hidden);
                    connection->update(pos_input,pos_hidden,
                                       neg_input,neg_hidden);
                }

                if( !fast_is_equal(persistent_cd_weight, 0.) )
                {
                    if( use_mean_field_cd )
                        PLERROR("In PseudolikelihoodRBM::train(): Persistent "
                            "Contrastive Divergence was not implemented for "
                            "MF-CD");

                    if( cd_decrease_ct != 0 )
                        lr = cd_learning_rate / (1.0 + stage * cd_decrease_ct );
                    else
                        lr = cd_learning_rate;
                    
                    lr *= persistent_cd_weight;

                    setLearningRate(lr);

                    int chain_i = stage % n_gibbs_chains;

                    if( !persistent_gibbs_chain_is_started[chain_i] )
                    {  
                        // Start gibbs chain
                        connection->setAsDownInput( input );
                        hidden_layer->getAllActivations( 
                            (RBMMatrixConnection*) connection );
                        hidden_layer->computeExpectation();
                        hidden_layer->generateSample();
                        pers_cd_hidden[chain_i] << hidden_layer->sample;
                        persistent_gibbs_chain_is_started[chain_i] = true;
                    }

                    if( fast_is_equal(persistent_cd_weight, 1.) )
                    {
                        // Hidden positive sample was not computed previously
                        connection->setAsDownInput( input );
                        hidden_layer->getAllActivations( 
                            (RBMMatrixConnection*) connection );
                        hidden_layer->computeExpectation();
                        pos_hidden << hidden_layer->expectation;
                    }

                    hidden_layer->sample << pers_cd_hidden[chain_i];
                    // Prolonged Gibbs chain
                    for(int i=0; i<cd_n_gibbs; i++)
                    {
                        connection->setAsUpInput( hidden_layer->sample );
                        input_layer->getAllActivations( 
                            (RBMMatrixConnection*) connection );
                        input_layer->computeExpectation();
                        input_layer->generateSample();
                        connection->setAsDownInput( input_layer->sample );
                        hidden_layer->getAllActivations( 
                            (RBMMatrixConnection*) connection );
                        hidden_layer->computeExpectation();
                        hidden_layer->generateSample();
                    }

                    pers_cd_input[chain_i] << input_layer->sample;
                    pers_cd_hidden[chain_i] << hidden_layer->sample;

                    input_layer->update(input, pers_cd_input[chain_i]);
                    hidden_layer->update(pos_hidden,hidden_layer->expectation);
                    connection->update(input,pos_hidden,
                                       pers_cd_input[chain_i],
                                       hidden_layer->expectation);
                }
            }
        
            if( !fast_is_equal(denoising_learning_rate, 0.) )
            {
                if( denoising_decrease_ct != 0 )
                    lr = denoising_learning_rate / 
                        (1.0 + stage * denoising_decrease_ct );
                else
                    lr = denoising_learning_rate;

                setLearningRate(lr);
                // I'm here
                if( fraction_of_masked_inputs > 0 )
                    random_gen->shuffleElements(autoencoder_input_indices);
                
                masked_autoencoder_input << input;
                if( fraction_of_masked_inputs > 0 )
                {
                    for( int j=0 ; 
                         j < round(fraction_of_masked_inputs*input_layer->size) ; 
                         j++)
                        masked_autoencoder_input[ autoencoder_input_indices[j] ] = 0; 
                }

                // Somehow, doesn't compile without the fancy casts...
                ((RBMMatrixConnection *)connection)->RBMConnection::fprop( masked_autoencoder_input, 
                                   hidden_layer->activation );

                hidden_layer->fprop( hidden_layer->activation,
                                     hidden_layer->expectation );
                
                transpose_connection->fprop( hidden_layer->expectation,
                                             input_layer->activation );
                input_layer->fprop( input_layer->activation,
                                    input_layer->expectation );
                input_layer->setExpectation( input_layer->expectation );

                real cost = input_layer->fpropNLL(input);
                
                input_layer->bpropNLL(input, cost, 
                                      reconstruction_activation_gradient);
                input_layer->update( reconstruction_activation_gradient );

                transpose_connection->bpropUpdate( 
                    hidden_layer->expectation,
                    input_layer->activation,
                    hidden_layer_expectation_gradient,
                    reconstruction_activation_gradient );

                hidden_layer->bpropUpdate( hidden_layer->activation,
                                           hidden_layer->expectation,
                                           hidden_layer_activation_gradient,
                                           hidden_layer_expectation_gradient );
                
                connection->bpropUpdate( masked_autoencoder_input, 
                                         hidden_layer->activation,
                                         reconstruction_activation_gradient, // is not used afterwards...
                                         hidden_layer_activation_gradient );
            }

        }
        train_stats->update( train_costs );
        
    }
    
    Profiler::end("training");
    const Profiler::Stats& stats = Profiler::getStats("training");
    real ticksPerSec = Profiler::ticksPerSecond();
    real cpu_time = (stats.user_duration+stats.system_duration)/ticksPerSec;
    cumulative_training_time += cpu_time;

    train_costs.fill(MISSING_VALUE);
    train_costs[training_cpu_time_cost_index] = cpu_time;
    train_costs[cumulative_training_time_cost_index] = cumulative_training_time;
    train_stats->update( train_costs );

    // Sums to 1 test
    //compute_Z();
    //conf.resize( input_layer->size );
    //Vec output,costs;
    //output.resize(outputsize());
    //costs.resize(getTestCostNames().length());
    //target.resize( targetsize() );
    //real sums = 0;
    //int input_n_conf = input_layer->getConfigurationCount();
    //for(int i=0; i<input_n_conf; i++)
    //{
    //    input_layer->getConfiguration(i,conf);
    //    computeOutput(conf,output);
    //    computeCostsFromOutputs( conf, output, target, costs );
    //    if( i==0 )
    //        sums = -costs[nll_cost_index];
    //    else
    //        sums = logadd( sums, -costs[nll_cost_index] );
    //    //sums += safeexp( -costs[nll_cost_index] );
    //}        
    //cout << "sums: " << safeexp(sums) << endl;
    //    //sums << endl;
    train_stats->finalize();
}


///////////////////
// computeOutput //
///////////////////
void PseudolikelihoodRBM::computeOutput(const Vec& input, Vec& output) const
{
    // Compute the output from the input.
    if( n_classes > 1 )
    {
        // Get output probabilities
        PLERROR("n_classes > 1 not implemented yet");
    }
    else
    {
        // Get hidden layer representation
        connection->setAsDownInput( input );
        hidden_layer->getAllActivations( (RBMMatrixConnection *) connection );
        hidden_layer->computeExpectation();
        output << hidden_layer->expectation;
    }
}


void PseudolikelihoodRBM::computeCostsFromOutputs(const Vec& input, 
                                                  const Vec& output,
                                                  const Vec& target, 
                                                  Vec& costs) const
{

    // Compute the costs from *already* computed output.
    costs.resize( cost_names.length() );
    costs.fill( MISSING_VALUE );

    if( n_classes > 1 )
    {
        costs[class_cost_index] =
            (argmax(output) == (int) round(target[0]))? 0 : 1;
        costs[nll_cost_index] = -pl_log(output[(int) round(target[0])]);
    }
    else
    {        
        if( compute_input_space_nll )
        {
            compute_Z();
            connection->setAsDownInput( input );
            hidden_layer->getAllActivations( (RBMMatrixConnection *) connection );
            costs[nll_cost_index] = hidden_layer->freeEnergyContribution(
                hidden_layer->activation) - dot(input,input_layer->bias) + log_Z;
        }
    }
}

TVec<string> PseudolikelihoodRBM::getTestCostNames() const
{
    // Return the names of the costs computed by computeCostsFromOutputs
    // (these may or may not be exactly the same as what's returned by
    // getTrainCostNames).

    return cost_names;
}

TVec<string> PseudolikelihoodRBM::getTrainCostNames() const
{
    return cost_names;
}


//#####  Helper functions  ##################################################

void PseudolikelihoodRBM::setLearningRate( real the_learning_rate )
{
    input_layer->setLearningRate( the_learning_rate );
    hidden_layer->setLearningRate( the_learning_rate );
    connection->setLearningRate( the_learning_rate );
    //target_layer->setLearningRate( the_learning_rate );
    //last_to_target->setLearningRate( the_learning_rate );
}

void PseudolikelihoodRBM::compute_Z() const
{
    if( Z_is_up_to_date ) return;

    int input_n_conf = input_layer->getConfigurationCount(); 
    int hidden_n_conf = hidden_layer->getConfigurationCount();
    if( input_n_conf == RBMLayer::INFINITE_CONFIGURATIONS && 
        hidden_n_conf == RBMLayer::INFINITE_CONFIGURATIONS )
        PLERROR("In PseudolikelihoodRBM::computeCostsFromOutputs: "
                "RBM's input and hidden layers are too big "
                "for NLL computations.");

    log_Z = 0;
    if( input_n_conf < hidden_n_conf )
    {
        conf.resize( input_layer->size );
        for(int i=0; i<input_n_conf; i++)
        {
            input_layer->getConfiguration(i,conf);
            connection->setAsDownInput( conf );
            hidden_layer->getAllActivations( (RBMMatrixConnection *) connection );
            if( i == 0 )
                log_Z = -hidden_layer->freeEnergyContribution(
                    hidden_layer->activation) + dot(conf,input_layer->bias);
            else
                log_Z = logadd(-hidden_layer->freeEnergyContribution(
                                   hidden_layer->activation) 
                               + dot(conf,input_layer->bias),
                               log_Z);
        }
    }
    else
    {
        conf.resize( hidden_layer->size );
        for(int i=0; i<hidden_n_conf; i++)
        {
            hidden_layer->getConfiguration(i,conf);
            connection->setAsUpInput( conf );
            input_layer->getAllActivations( (RBMMatrixConnection *) connection );
            if( i == 0 )
                log_Z = -input_layer->freeEnergyContribution(
                    input_layer->activation) + dot(conf,hidden_layer->bias);
            else
                log_Z = logadd(-input_layer->freeEnergyContribution(
                                   input_layer->activation)
                               + dot(conf,hidden_layer->bias),
                               log_Z);
        }        
    }
    
    Z_is_up_to_date = true;
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
