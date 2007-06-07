#! /usr/bin/env python
import sys
import os
import getopt
import re
import string
import time
import traceback
import shutil
from subprocess import Popen,PIPE
from utils import *
from configobj import ConfigObj
from textwrap import dedent
import pdb
from time import sleep
#from plearn.pymake import pymake

STATUS_FINISHED = 0
STATUS_RUNNING = 1
STATUS_WAITING = 2
STATUS_ERROR = 3


class DBIBase:

    def __init__(self, commands, **args ):
        #generate a new unique id
        self.unique_id = get_new_sid('')

        # option is not used yet
        self.has_short_duration = 0

        # if all machines are full, run the jobs one by one on the localhost
        self_use_localhost_if_full = 1

        # the( human readable) time format used in log file
        self.time_format = "%Y-%m-%d/%H:%M:%S"

        # Commands to be executed once before the entire batch
        self.pre_batch = []
        # Commands to be executed before every task in tasks
        self.pre_tasks = []
        # The main tasks to be dispatched
        self.tasks = []
        # Commands to be executed after each task in tasks
        self.post_tasks = []
        # Commands to be executed once after the entire batch
        self.post_batch = []

        # the default directory where to keep all the log files
        self.log_dir = 'LOGS'
        self.log_file = os.path.join( self.log_dir, self.unique_id )

        # the default directory where file generated by dbi will be stored
        # It should not take the "" or " " value. Use "." instead.
        self.tmp_dir = 'TMP_DBI'

        #
        self.file_redirect_stdout = 0
        self.file_redirect_stderr = 0

        # Initialize the namespace
        self.requirements = ''
        self.test = False
        self.dolog = False
        for key in args.keys():
            self.__dict__[key] = args[key]

        # If some arguments aren't lists, put them in a list
        if not isinstance(commands, list):
            commands = [commands]
        if not isinstance(self.pre_batch, list):
            self.pre_batch = [self.pre_batch]
        if not isinstance(self.pre_tasks, list):
            self.pre_tasks = [self.pre_tasks]
        if not isinstance(self.post_tasks, list):
            self.post_tasks = [self.post_tasks]
        if not isinstance(self.post_batch, list):
            self.post_batch = [self.post_batch]

    def n_avail_machines(self): raise NotImplementedError, "DBIBase.n_avail_machines()"

    def clean(self):
        pass

    def run(self):
        pass

class Task:

    def __init__(self, command, tmp_dir, log_dir, time_format, pre_tasks=[], post_tasks=[], dolog = True, args = {}):
        self.unique_id = get_new_sid('')
        self.add_unique_id = 0
        formatted_command = re.sub( '[^a-zA-Z0-9]', '_', command );
        self.log_file = truncate( os.path.join(log_dir, self.unique_id +'_'+ formatted_command), 200) + ".log"
        # The "python utils.py..." command is not exactly the same for every
        # task in a batch, so it cannot be considered a "pre-command", and
        # has to be actually part of the command.  Since the user-provided
        # pre-command has to be executed afterwards, it also has to be part of
        # the command itself. Therefore, no need for pre- and post-commands in
        # the Task class

        utils_file = os.path.join(tmp_dir, 'utils.py')
        utils_file = os.path.abspath(utils_file)

        for key in args.keys():
            self.__dict__[key] = args[key]

        if self.add_unique_id:
                command = command + ' unique_id=' + self.unique_id
        #self.before_commands = []
        #self.user_defined_before_commands = []
        #self.user_defined_after_commands = []
        #self.after_commands = []

        self.commands = []
        if len(pre_tasks) > 0:
            self.commands.extend( pre_tasks )

        if dolog == True:
            self.commands.append(utils_file + ' set_config_value '+
                string.join([self.log_file,'STATUS',str(STATUS_RUNNING)],' '))
            # set the current date in the field LAUNCH_TIME
            self.commands.append(utils_file +  ' set_current_date '+
                string.join([self.log_file,'LAUNCH_TIME',time_format],' '))


        self.commands.append( command )
        self.commands.extend( post_tasks )
        if dolog == True:
            self.commands.append(utils_file + ' set_config_value '+
                string.join([self.log_file,'STATUS',str(STATUS_FINISHED)],' '))
            # set the current date in the field FINISHED_TIME
            self.commands.append(utils_file + ' set_current_date ' +
                string.join([self.log_file,'FINISHED_TIME',time_format],' '))

        #print "self.commands =", self.commands

    def get_status(self):
        #TODO: catch exception if value not available
        status = get_config_value(self.log_file,'STATUS')
        return int(status)

    def get_stdout(self):
        try:
            if isinstance(self.p.stdout, file):
                return self.p.stdout
            else:
                return open(self.log_file + '.out','r')
        except:
            pass
        return None
        
    def get_stderr(self):
        try:
            if isinstance(self.p.stderr, file):
                return self.p.stderr
            else:
                return open(self.log_file + '.err','r')
        except:
            pass
        return None

    def get_waiting_time(self):
        # get the string representation
        str_sched = get_config_value(self.log_file,'SCHEDULED_TIME')
        # transform in seconds from the start of epoch
        sched_time = time.mktime(time.strptime(str_sched,ClusterLauncher.time_format))

        # get the string representation
        str_launch = get_config_value(self.log_file,'LAUNCH_TIME')
        # transform in seconds from the start of epoch
        launch_time = time.mktime(time.strptime(str_launch,ClusterLauncher.time_format))

        return launch_time - sched_time

    def get_running_time(self):
        #TODO: handle if job did not finish
        # get the string representation
        str_launch = get_config_value(self.log_file,'LAUNCH_TIME')
        # transform in seconds from the start of epoch
        launch_time = time.mktime(time.strptime(str_launch,ClusterLauncher.time_format))

        # get the string representation
        str_finished = get_config_value(self.log_file,'FINISHED_TIME')
        # transform in seconds from the start of epoch
        finished_time = time.mktime(time.strptime(str_finished,ClusterLauncher.time_format))

        return finished_time - launch_time

class DBICluster(DBIBase):

    def __init__(self, commands, **args ):
        DBIBase.__init__(self, commands, **args)

        # check if log directory exists, if not create it
        if self.dolog and not os.path.exists(self.log_dir):
            os.mkdir(self.log_dir)

        # create the information about the tasks
        for command in commands:
            self.tasks.append(Task(command, self.tmp_dir, self.log_dir,
                                   self.time_format,self.pre_tasks,
                                   self.post_tasks,self.dolog))


    def run_one_job(self, task):
        DBIBase.run(self)

        command = "cluster --execute '" + string.join(task.commands,';') + "'"
        print command

        task.launch_time = time.time()
        set_config_value(task.log_file, 'SCHEDULED_TIME',
                time.strftime(self.time_format, time.localtime(time.time())))
        output = PIPE
        error = PIPE
        if int(self.file_redirect_stdout):
            output = file(task.log_file + '.out','w')
        if int(self.file_redirect_stderr):
            error = file(task.log_file + '.err','w')
        if self.test == False:
            task.p = Popen(command, shell=True,stdout=output,stderr=error)

    def run(self):
        if self.test:
            print "Test mode, we only print the command to be executed, we don't execute them"
        # Execute pre-batch
        pre_batch_command = ';'.join( self.pre_batch )
        output = PIPE
        error = PIPE
        if int(self.file_redirect_stdout):
            output = file(self.log_file + '.pre_batch.out', 'w')
        if int(self.file_redirect_stderr):
            error = file(self.log_file + '.pre_batch.err', 'w')
        if self.test:
            print pre_batch_command
        else:
            self.pre = Popen(pre_batch_command, shell=True, stdout=output, stderr=error)

        # Execute all Tasks (including pre_tasks and post_tasks if any)
        for task in self.tasks:
            self.run_one_job(task)

        # Execute post-batchs
        post_batch_command = ";".join( self.post_batch );
        if int(self.file_redirect_stdout):
            output = file(self.log_file + '.post_batch.out', 'w')
        if int(self.file_redirect_stderr):
            error = file(self.log_file + '.post_batch.err', 'w')
        if self.test:
            print post_batch_command
        else:
            self.post = Popen(post_batch_command, shell=True, stdout=output, stderr=error)
            
    def clean(self):
        #TODO: delete all log files for the current batch
        pass

class DBIbqtools(DBIBase):

    def __init__( self, commands, **args ):
        DBIBase.__init__(self, commands, **args)

        # create directory in which all the temp files will be created
        self.temp_dir = 'batch_' + self.unique_id + '_tmp'
        os.mkdir(self.temp_dir)
        os.chdir(self.temp_dir)

        # create the right symlink for parent in self.temp_dir_name
        self.parent_dir = 'parent'
        os.symlink( '..', self.parent_dir )

        # check if log directory exists, if not create it
        if self.dolog and not os.path.exists(self.log_dir):
            os.mkdir(self.log_dir)

        self.log_file = os.path.join( self.parent_dir, self.log_file )

        # create the information about the tasks
        args['temp_dir'] = self.temp_dir
        for command in commands:
            self.tasks.append(Task(command, self.tmp_dir, self.log_dir,
                                   self.time_format,
                                   [self.pre_tasks, 'cd parent;'],
                                   self.post_tasks,self.dolog,args))


    def run(self):
        pre_batch_command = ';'.join( self.pre_batch );
        if int(self.file_redirect_stdout):
            pre_batch_command += ' >> ' + self.log_file + '.pre_batch.out'
        if int(self.file_redirect_stderr):
            pre_batch_command += ' 2>> ' + self.log_file + '.pre_batch.err'

        post_batch_command = ';'.join( self.post_batch );
        if int(self.file_redirect_stdout):
            post_batch_command += ' >> ' + self.log_file + '.post_batch.out'
        if int(self.file_redirect_stderr):
            post_batch_command += ' 2>> ' + self.log_file + '.post_batch.err'

        # create one (sh) script that will launch the appropriate ~~command~~
        # in the right environment


        launcher = open( 'launcher', 'w' )
        bq_cluster_home = os.getenv( 'BQ_CLUSTER_HOME', '$HOME' )
        bq_shell_cmd = os.getenv( 'BQ_SHELL_CMD', '/bin/sh -c' )
        launcher.write( dedent('''\
                #!/bin/sh

                HOME=%s
                export HOME

                (%s '~~task~~')'''
                % (bq_cluster_home, bq_shell_cmd)
                ) )

        if int(self.file_redirect_stdout):
            launcher.write( ' >> ~~logfile~~.out' )
        if int(self.file_redirect_stderr):
            launcher.write( ' 2>> ~~logfile~~.err' )
        launcher.close()

        # create a file containing the list of commands, one per line
        # and another one containing the log_file name associated
        tasks_file = open( 'tasks', 'w' )
        logfiles_file = open( 'logfiles', 'w' )
        for task in self.tasks:
            tasks_file.write( ';'.join(task.commands) + '\n' )
            logfiles_file.write( task.log_file + '\n' )
        tasks_file.close()
        logfiles_file.close()

        # create the bqsubmit.dat, with
        bqsubmit_dat = open( 'bqsubmit.dat', 'w' )
        bqsubmit_dat.write( dedent('''\
                batchName = dbi_batch
                command = sh launcher
                templateFiles = launcher
                linkFiles = parent;parent/utils.py
                remoteHost = ss3
                param1 = (task, logfile) = load tasks, logfiles
                concurrentJobs = 200

                ''') )
#                preBatch = ''' + pre_batch_command + '''
#                postBatch = ''' + post_batch_command +'''
        bqsubmit_dat.close()

        # Launch bqsubmit
        output = PIPE
        error = PIPE
        if int(self.file_redirect_stdout):
            output = file(self.log_file + '.out', 'w')
        if int(self.file_redirect_stderr):
            error = file(self.log_file + '.err', 'w')
        self.p = Popen( 'bqsubmit', shell=True, stdout=output, stderr=error)

        os.chdir('parent')

class DBICondor(DBIBase):

    def __init__( self, commands, **args ):
        DBIBase.__init__(self, commands, **args)

        # check if log directory exists, if not create it
        if self.dolog and not os.path.exists(self.log_dir):
            os.mkdir(self.log_dir)

        if not os.path.exists(self.tmp_dir):
            os.mkdir(self.tmp_dir)
            
#        self.log_file = os.path.join( self.parent_dir, self.log_file )

        # create the information about the tasks
#        args['temp_dir'] = self.temp_dir
        for command in commands:
            pos = string.find(command,' ')
            if pos>=0:
                c = command[0:pos]
                c2 = command[pos:]
            else:
                c=command
                c2=""

            # We use the absolute path so that we don't have corner case as with ./ 
            c = os.path.normpath(os.path.join(os.getcwd(), c))
            command = c + c2
            
                # We will execute the command on the specified architecture
                # if it is specified. If the executable exist for both
                # architecture we execute on both. Otherwise we execute on the
                # same architecture as the architecture of the launch computer
            self.cplat = get_condor_platform()
            if c.endswith('.32'):
                self.targetcondorplatform='INTEL'
                self.targetplatform='linux-i386'
                newcommand=command
            elif c.endswith('.64'):
                self.targetcondorplatform='X86_64'
                self.targetplatform='linux-x86_64'
                newcommand=command
            elif os.path.exists(c+".32") and os.path.exists(c+".64"):
                self.targetcondorplatform='BOTH'
                self.targetplatform='linux-i386'
                #newcommand=c+".32"+c2
                newcommand='if [ $1 == "INTEL" ]; then\n'
                newcommand+='  '+c+'.32'+c2+'\n'
                newcommand+='else\n'
                newcommand+='  '+c+".64"+c2+'\nfi'
                if not os.access(c+".64", os.X_OK):
                    raise Exception("The command '"+c+".64' do not have execution permission!")
                c+=".32"
            elif self.cplat=="INTEL" and os.path.exists(c+".32"):
                self.targetcondorplatform='INTEL'
                self.targetplatform='linux-i386'
                c+=".32"
                newcommand=c+c2
            elif self.cplat=="X86_64" and os.path.exists(c+".64"):
                self.targetcondorplatform='X86_64'
                self.targetplatform='linux-x86_64'
                c+=".64"
                newcommand=c+c2
            else:
                self.targetcondorplatform=self.cplat
                if self.cplat=='INTEL':
                    self.targetplatform='linux-i386'
                else:
                    self.targetplatform='linux-x86_64'
                newcommand=command
            
            if not os.path.exists(c):
                raise Exception("The command '"+c+"' do not exist!")
            elif not os.access(c, os.X_OK):
                raise Exception("The command '"+c+"' do not have execution permission!")

            self.tasks.append(Task(newcommand, self.tmp_dir, self.log_dir,
                                   self.time_format, self.pre_tasks,
                                   self.post_tasks,self.dolog,args))

            #keeps a list of the temporary files created, so that they can be deleted at will            
        self.temp_files = []

    def run_all_job(self):
        if len(self.tasks)==0:
            return #no task to run
        # create the bqsubmit.dat, with
        condor_datas = []
        for task in self.tasks:
            condor_data = os.path.join(self.tmp_dir, task.unique_id + '.data')
            condor_datas.append(condor_data)
            self.temp_files.append(condor_data)
            param_dat = open(condor_data, 'w')
            
            param_dat.write( dedent('''\
            #!/bin/bash
            %s''' %('\n'.join(task.commands))))
            param_dat.close()
        

        condor_file = os.path.join(self.tmp_dir, task.unique_id + ".condor")
        self.temp_files.append(condor_file)
        condor_dat = open( condor_file, 'w' )
        
        req=""
        u=get_username()
        if self.targetcondorplatform == 'BOTH':
            req="((Arch == \"INTEL\")||(Arch == \"X86_64\"))"
        elif self.targetcondorplatform == 'INTEL':
            req="(Arch == \"INTEL\")"
        elif self.targetcondorplatform == 'X86_64':
            req="(Arch == \"X86_64\")"
            

        tplat=self.targetplatform

        if self.requirements != "":
            req = req+'&&('+self.requirements+')'

        condor_dat.write( dedent('''\
                executable     = %s/launch.sh
                universe       = vanilla
                requirements   = %s
                output         = main.%s.%s.$(Process).out
                error          = main.%s.%s.$(Process).error
                log            = main.%s.log
                ''' % (self.tmp_dir,req,self.targetcondorplatform,task.unique_id,self.targetcondorplatform,task.unique_id,self.targetcondorplatform)))
#                preBatch = ''' + pre_batch_command + '''
#                postBatch = ''' + post_batch_command +'''

        for i in condor_datas:
            condor_dat.write("arguments      = sh "+i+" $$(Arch) \nqueue\n")
        condor_dat.close()

        launch_file = os.path.join(self.tmp_dir, 'launch.sh')
        dbi_file=get_plearndir()+'/python_modules/plearn/parallel/dbi.py'
        overwrite_launch_file=False
        if not os.path.exists(dbi_file):
            print 'WARNING: Can\' locate dbi.py file. Meaby the file "'+launch_file+'" is not up to date!'
        else:
            if os.path.exists(launch_file):
                mtimed=os.stat(dbi_file)[8]
                mtimel=os.stat(launch_file)[8]
                if mtimed>mtimel:
                    print 'WARNING: We overwrite the file "'+launch_file+'" with a new version. Update it to your need!'
                    overwrite_launch_file=True
        
        if not os.path.exists(launch_file) or overwrite_launch_file:
            self.temp_files.append(launch_file)
            launch_dat = open(launch_file,'w')
            launch_dat.write(dedent('''\
                #!/bin/sh
                PROGRAM=$1
                shift\n'''))
            if None != os.getenv("CONDOR_LOCAL_SOURCE"):
                launch_dat.write('source ' + os.getenv("CONDOR_LOCAL_SOURCE") + '\n')
            launch_dat.write(dedent('''\
                    echo "Executing on ${HOSTNAME}" 1>&2
                    echo "PATH: $PATH" 1>&2
                    echo "PYTHONPATH: $PYTHONPATH" 1>&2
                    echo "LD_LIBRARY_PATH: $LD_LIBRARY_PATH" 1>&2
                    which python 1>&2
                    echo -n python version: 1>&2
                    python -V 1>&2
                    echo -n /usr/bin/python version: 1>&2
                    /usr/bin/python -V 1>&2
                    echo ${PROGRAM} $@ 1>&2
                    $PROGRAM $@'''))
            launch_dat.close()
            os.chmod(launch_file, 0755)

        utils_file = os.path.join(self.tmp_dir, 'utils.py')
        if not os.path.exists(utils_file):
            shutil.copy( get_plearndir()+
                         '/python_modules/plearn/parallel/utils.py', utils_file)
            self.temp_files.append(utils_file)
            os.chmod(utils_file, 0755)

        configobj_file = os.path.join(self.tmp_dir, 'configobj.py')
        if not os.path.exists('configobj.py'):
            shutil.copy( get_plearndir()+
                         '/python_modules/plearn/parallel/configobj.py',  configobj_file)
            self.temp_files.append(configobj_file)            
            os.chmod(configobj_file, 0755)
        # Launch condor
        output = PIPE
        error = PIPE
        if int(self.file_redirect_stdout):
            output = file(self.log_file + '.out', 'w')
        if int(self.file_redirect_stderr):
            error = file(self.log_file + '.err', 'w')

        if self.test == False:
            print "Executing: condor_submit " + condor_file
            self.p = Popen( 'condor_submit '+ condor_file, shell=True , stdout=output, stderr=error)
        else:
            print "Created condor file: " + condor_file
            
    def clean(self):
                
        sleep(20)
        for file_name in self.temp_files:
            try:
                os.remove(file_name)
            except os.error:
                pass
            pass    


    def run(self):
        self.run_all_job()




    def clean(self):
        pass



class SshHost:
    def __init__(self, hostname):
        self.hostname= hostname
        self.lastupd= -16
        self.getAvailability()
        
    def getAvailability(self):
        # simple heuristic: mips / load
        t= time.time()
        if t - self.lastupd > 15: # min. 15 sec. before update
            self.bogomips= self.getBogomips()
            self.loadavg= self.getLoadavg()
            self.lastupd= t
            #print  self.hostname, self.bogomips, self.loadavg, (self.bogomips / (self.loadavg + 0.5))
        return self.bogomips / (self.loadavg + 0.5)
        
    def getBogomips(self):
        cmd= ["ssh", self.hostname ,"cat /proc/cpuinfo"]
        p= Popen(cmd, stdout=PIPE)
        bogomips= 0.0
        for l in p.stdout:
            if l.startswith('bogomips'):
                s= l.split(' ')
                bogomips+= float(s[-1])
        return bogomips

    def getLoadavg(self):
        cmd= ["ssh", self.hostname,"cat /proc/loadavg"]
        p= Popen(cmd, stdout=PIPE)
        l= p.stdout.readline().split(' ')
        return float(l[0])
        
    def addToLoadavg(self,n):
        self.loadavg+= n
        self.lastupd= time.time()

    def __str__(self):
        return "SshHost("+self.hostname+" <"+str(self.bogomips) \
               +','+str(self.loadavg) +','+str(self.getAvailability()) \
               +','+str(self.lastupd) + '>)'

    def __repr__(self):
        return str(self)
        
def find_all_ssh_hosts():
    return [SshHost(h) for h in set(pymake.get_distcc_hosts())]

def cmp_ssh_hosts(h1, h2):
    return cmp(h2.getAvailability(), h1.getAvailability())

class DBISsh(DBIBase):

    def __init__(self, commands, **args ):
        print "WARNING: The SSH DBI is not fully implemented!"
        print "Use at your own risk!"
        DBIBase.__init__(self, commands, **args)

        # check if log directory exists, if not create it
        if self.dolog and not os.path.exists(self.log_dir):
            os.mkdir(self.log_dir)

        # create the information about the tasks
        for command in commands:
            self.tasks.append(Task(command, self.tmp_dir, self.log_dir,
                                   self.time_format, self.pre_tasks,
                                   self.post_tasks,self.dolog))
        self.hosts= find_all_ssh_hosts()
        

    def getHost(self):
        self.hosts.sort(cmp= cmp_ssh_hosts)
        #print "hosts= "
        #for h in self.hosts: print h
        self.hosts[0].addToLoadavg(1.0)
        return self.hosts[0]
    
    def run_one_job(self, task):
        DBIBase.run(self)

        host= self.getHost()


        cwd= os.getcwd()
        command = "ssh " + host.hostname + " 'cd " + cwd + "; " + string.join(task.commands,';') + "'"
        print command

        task.launch_time = time.time()
        set_config_value(task.log_file, 'SCHEDULED_TIME',
                time.strftime(self.time_format, time.localtime(time.time())))
        output = PIPE
        error = PIPE
        if int(self.file_redirect_stdout):
            output = file(task.log_file + '.out','w')
        if int(self.file_redirect_stderr):
            error = file(task.log_file + '.err','w')        
        task.p = Popen(command, shell=True,stdout=output,stderr=error)

    def run(self):
        # Execute pre-batch
        pre_batch_command = ';'.join( self.pre_batch )
        output = PIPE
        error = PIPE
        if int(self.file_redirect_stdout):
            output = file(self.log_file + '.pre_batch.out', 'w')
        if int(self.file_redirect_stderr):
            error = file(self.log_file + '.pre_batch.err', 'w')
        self.pre = Popen(pre_batch_command, shell=True, stdout=output, stderr=error)
        print 'pre_batch_command =', pre_batch_command

        # Execute all Tasks (including pre_tasks and post_tasks if any)
        print "tasks= ", self.tasks
        for task in self.tasks:
            self.run_one_job(task)

        # Execute post-batchs
        post_batch_command = ";".join( self.post_batch );
        if int(self.file_redirect_stdout):
            output = file(self.log_file + '.post_batch.out', 'w')
        if int(self.file_redirect_stderr):
            error = file(self.log_file + '.post_batch.err', 'w')
        self.post = Popen(post_batch_command, shell=True, stdout=output, stderr=error)
        print 'post_batch_command =', post_batch_command

    def clean(self):
        #TODO: delete all log files for the current batch
        pass



# creates an object of type ('DBI' + launch_system) if it exists
def DBI(commands, launch_system, **args):
    try:
        jobs = eval('DBI'+launch_system+'(commands,**args)')
    except NameError:
        print 'The launch system ',launch_system, ' does not exists. Available systems are: Cluster, Ssh, bqtools and Condor'
        traceback.print_exc()
        sys.exit(1)
    return jobs

def main():
    if len(sys.argv)!=2:
        print "Usage:dbi.py {Condor|Cluster|Ssh|Local|bqtools} < joblist"
        print "Where joblist is a file containing one exeperiement by line"
        sys.exit(0)
    DBI([ s[0:-1] for s in sys.stdin.readlines() ], sys.argv[1]).run()
#    jobs.clean()

#    config['LOG_DIRECTORY'] = 'LOGS/'
if __name__ == "__main__":
    main()
