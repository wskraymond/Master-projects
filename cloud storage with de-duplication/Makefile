JFLAGS =
AZUREJAR = -cp .:$$AZURE/*
JC = javac
.SUFFIXES: .java .class
.java.class:
	$(JC) $(JFLAGS) $(AZUREJAR) $*.java

CLASSES = \
	MyDedup.java 

default: classes

classes: $(CLASSES:.java=.class)

clean:
	$(RM) *.class
