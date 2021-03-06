from plearn.learners.modulelearners import *

zoom_factor = 5
from plearn.learners.modulelearners.sampler import *

import random


def view_reconstruct( learner, Nim , dataSet):

  print "analyzing learner..."
  #
  # Getting the RBMmodule which sees the image (looking at size of the down layer)
  #
  modules=getModules(learner)
  for i in range(len(modules)):
     module = modules[i]
     if isModule(module,'RBM') and module.connection.down_size == Nim:
        image_RBM=learner.module.modules[i]
        break
  image_RBM_name=image_RBM.name
  #
  # Getting the top RBMmodule
  #

  top_RBM = getTopRBMModule( learner )
  top_RBM_name = top_RBM.name
  
  NH=top_RBM.connection.up_size


  init_ports = [ ('input',  image_RBM_name+'.visible'),
                 ('output', top_RBM_name+'.hidden.state')
               ]
  ports = [ ('input', top_RBM_name+'.hidden.state' ),
            ('output', image_RBM_name+'.visible_reconstruction.state')
            ]

  #
  # Removing useless connections for sampling
  #
  old_connections_list = copy.copy(learner.module.connections)
  conn_toremove=[]
  connections_list_down=[]
  connections_list_up=[]
  for connection in old_connections_list:
      source_module = getModule( learner, port2moduleName( connection.source ))
      dest_module   = getModule( learner, port2moduleName( connection.destination ))
      if isModule( source_module, 'RBM') and isModule( dest_module,'RBM'):
         connections_list_up.append ( pl.NetworkConnection(source = port2moduleName( connection.source )+'.hidden.state',
                                                           destination = port2moduleName( connection.destination )+'.visible',
                                                           propagate_gradient = 0) )
         connections_list_down.append ( pl.NetworkConnection(source = port2moduleName( connection.destination )+'.visible_reconstruction.state',
                                                    destination = port2moduleName( connection.source )+'.hidden.state',
                                                    propagate_gradient = 0) )
  
  #
  # Removing useless modules for sampling
  #
  modules_list = getModules(learner)
  mod_toremove=[]
  for module in modules_list:
      if isModule( module, 'RBM') == False:
         mod_toremove.append(module)
  for module in mod_toremove:
      modules_list.remove(module)
  
  RBMnetworkInit = pl.NetworkModule(
                          modules = modules_list,
                          connections = connections_list_up,
                          ports = init_ports,
                          # to avoid calling the forget() method in ModuleLearner                          
                          random_gen = pl.PRandom( seed = 1827 ),
                          # Hack from Olivier
                          save_states = 0
                         )
  RBMnetwork = pl.NetworkModule(
                          modules = modules_list,
                          connections = connections_list_down,
                          ports = ports,
                          # to avoid calling the forget() method in ModuleLearner                          
                          random_gen = pl.PRandom( seed = 1827 ),
                          # Hack from Olivier
                          save_states = 0
                         )

  
  RBMmodel = pl.ModuleLearner(
                              cost_ports = [],
                              target_ports = [],
                              module = RBMnetwork
                           )
  RBMmodelInit = pl.ModuleLearner(
                              cost_ports = [],
                              target_ports = [],
                              module = RBMnetworkInit
                           )



  RBMmodelInit.setTrainingSet(pl.AutoVMatrix(inputsize=Nim, targetsize=0, weightsize=0), False)
  RBMmodel.setTrainingSet(pl.AutoVMatrix(inputsize=NH, targetsize=0, weightsize=0), False)


  screen=init_screen(Nim,zoom_factor)
  random.seed(1969)

  for i in range(len(RBMmodel.module.modules)):
    module = RBMmodel.module.modules[i]
    if isModule( module, 'RBM'):
       RBMmodel.module.modules[i].compute_contrastive_divergence = False
       if module.name == top_RBM_name:
          top_RBM = RBMmodel.module.modules[i]

  RBMmodelInit.build()
  RBMmodel.build()

  reconstruct_man()

  while True:
 
   random_index=random.randint(0,dataSet.length)
   init_image=[dataSet.getRow(random_index)[i] for i in range(Nim)]
   
   c = draw_image( init_image, screen, zoom_factor )
   if c==EXITCODE:
      return
   
   init_hidden = RBMmodelInit.computeOutput(init_image)
   c = draw_image( RBMmodel.computeOutput(init_hidden), screen, zoom_factor )   
   if c==EXITCODE:
      return

def reconstruct_man():
     print "\nPlease type:"
     print ":    <ENTER>   : to continue"
     print ":      q       : (quit) when you are fed up\n"


if __name__ == "__main__":
  import sys, os.path

  if len(sys.argv) < 2:
     print "Usage:\n\t" + sys.argv[0] + " <ModuleLearner_filename> <Image_size> <dataSet_filename>\n"
     print "Purpose:\n\tSee the reconstruction of an image by an RBM (autoassiocator mode)\n"
     reconstruct_man()
     sys.exit()

  learner_filename = sys.argv[1]
  Nim = int(sys.argv[2])
  data_filename = sys.argv[3]
     

     
  if os.path.isfile(learner_filename) == False:
     raise TypeError, "Cannot find file "+learner_filename
  print " loading... "+learner_filename
  learner = loadObject(learner_filename)
  if 'HyperLearner' in str(type(learner)):
     learner=learner.learner
  
  if os.path.isfile(data_filename) == False and os.path.isdir(data_filename) == False:
     raise TypeError, "Cannot find file "+data_filename
  print " loading... "+data_filename
  dataSet = pl.AutoVMatrix( specification = data_filename )

  view_reconstruct( learner, Nim , dataSet)
