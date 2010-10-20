import time

import pyphp
pyphp.shutdown()

start = time.time()

init = start
pyphp.init()
init = time.time() - start
print "Init duration: %s" % init

i = 0;
while i < 1000:

	script = time.time()
	result = pyphp.runScript('test.php')
	script = time.time() - script
	#print "Script duration: %s" % script

	if result == False:
		print "Script failed!"
		
	i += 1





shutdown = time.time()
pyphp.shutdown()
shutdown = time.time() - shutdown
print "Shutdown duration: %s" % shutdown

print "Total duration: %s" % (time.time() - start)
