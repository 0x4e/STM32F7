The drivers under AV1.11 are from an older version of STCUBE.
The reason it is called AV1.11 is due to the way the Makefile was finding the various folders for the code.
Having it as V1.11 caused the Makefile to take the c file from the first path it found, even when explicitly telling make where the file is...




