
# *****************************************************************************************************************************
#											Manages core operations data
# *****************************************************************************************************************************

class CoreOperations:
	def __init__(self):
		core = """		
				@ ! c@ c! +! + - * / and or xor not 0= 0> 0< 0- 1+ 1- 2* 2/ dup drop swap rot over ; 
				r> >r rdrop if then for next dsp! dsp@ rsp! rsp@
			"""																				# core words.
		self.coreList = core.lower().split()												# make a list of words
		self.coreList.sort()																# alphabetical order
		self.coreDictionary = {} 															# convert to dictionary
		for n in range(0,len(self.coreList)):
			self.coreDictionary[self.coreList[n]] = n

	def getName(self,id):																	# id -> name
		return self.coreList[id]
	def getID(self,word):																	# name -> id
		return self.coreDictionary(word.lower().strip())
	def getIDList(self):																	# list of ids
		return range(0,len(self.coreList))

	def createFiles(self):
		h = open("__primitives.h","w")														# create any files which depend on IDs
		h.write("/* Automatically generated */\n\n")
		h.write("#ifndef __PRIMITIVES\n#define __PRIMITIVES\n\n")
		h.write("#define COP_COUNT ({0})\n\n".format(len(self.coreList)))
		for id in self.getIDList():
			name = self.toIdentifier(self.getName(id).upper())
			h.write("#define COP_{0} ({1})\n".format(name,id))
		h.write("\n#ifdef STATIC_WORD_NAMES\n\n")
		s = ",".join(['"'+x+'"' for x in self.coreList])
		h.write("static const char *__primitives[] = {"+s+"};\n")
		h.write("#endif\n\n")
		h.write("#endif\n\n")
		h.close()

	def toIdentifier(self,s):
		s = s.replace("@","_READ_").replace("!","_STORE_").replace("+","_ADD_")				# convert word to valid C identifier
		s = s.replace("-","_SUB_").replace("/","_DIV_").replace(">","_GREATER_")
		s = s.replace("*","_MUL_").replace("<","_LESS_").replace("=","_EQUAL_")
		s = s.replace(";","_RETURN_")
		s = s.replace("__","_")
		s = s[1:] if s[0] == "_" else s
		s = s[:-1] if s[-1] == "_" else s
		return s

c = CoreOperations()
c.createFiles()
