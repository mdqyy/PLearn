#
# pyext
# Copyright (C) 2007 Xavier Saint-Mleux, ApSTAT Technologies inc.
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions are met:
#
#   1. Redistributions of source code must retain the above copyright
#      notice, this list of conditions and the following disclaimer.
#
#   2. Redistributions in binary form must reproduce the above copyright
#      notice, this list of conditions and the following disclaimer in the
#      documentation and/or other materials provided with the distribution.
#
#   3. The name of the authors may not be used to endorse or promote
#      products derived from this software without specific prior written
#      permission.
#
#  THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS OR
#  IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
#  OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN
#  NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
#  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
#  TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
#  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
#  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
#  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
#  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
#  This file is part of the PLearn library. For more information on the PLearn
#  library, go to the PLearn Web site at www.plearn.org

# Import correct PLearn python extension module as set by plearn.getLib()
from plearn import getLib
pl_lib_dir, pl_lib_name = getLib()
exec 'from %s.%s import *' % (pl_lib_dir, pl_lib_name)
exec 'from %s import %s as pl' % (pl_lib_dir, pl_lib_name)


from plearn.pybridge import wrapped_plearn_object
wrapped_plearn_object.plearn_module= pl

import gc, atexit
from plearn.pyplearn.plargs import *
from plearn.utilities.options_dialog import *

def cleanupWrappedObjects():
    gc.collect()
    if pl: #if plext still loaded
        ramassePoubelles()
atexit.register(cleanupWrappedObjects)

__VERSION__= versionString()

# Redefines function TMat to emulate pyplearn behaviour
def TMat( *args ):
    """Returns a list of lists, each inner list being a row of the TMat"""
    nargs = len(args)
    assert nargs in (0, 1, 3)

    # Empty TMat
    if nargs == 0:
        return []

    # Argument is already a list of lists
    elif nargs == 1:
        return args[0]

    # Argument is a (length, width, content) tuple, content being a list
    elif nargs == 3:
        nrows = args[0]
        ncols = args[1]
        content = args[2]
        assert nrows*ncols == len(content)

        return [ content[i*ncols:(i+1)*ncols] for i in range(nrows) ]


verb, logs, namespaces, use_gui = getGuiInfo(sys.argv)

# Enact the use of plargs: the current behavior is to consider as a plargs
# any command-line argument that contains a '=' char and to neglect all
# others
plargs.parse([ arg for arg in sys.argv if arg.find('=') != -1 ])

if use_gui:
    runit, verb, logs= optionsDialog(sys.argv[0], plargs.expdir,
                                     verb, logs, namespaces)
    if not runit:
        sys.exit()
    loggingControl(verb, logs)

#pl.AutoVMatrix()
#AutoVMatrix.__len__ = lambda self: self.length

if __name__ == "__main__":
    class A(plargs):
        T = plopt(0)

    class B(plnamespace):
        T = plopt(1)

    print sys.argv[1:]
    print A.T
    print B.T

    # python __init__.py T=10 B.T=25
    # ['T=10', 'B.T=25']
    # 10
    # 25
