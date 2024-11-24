# BFMReader
While I was creating readers for the bloodrayne 2 game formats, 
I found more and more problems, 
which led to my own implementation of the serialization library (sern) 
and static reflection(tiny_refl). 
To prove the generality of the ideas underlying this library, 
I also had to write a json exporter. 

These very much distracted me from the main project, 
as a result it is laid out in an 
ABSOLUTELY UNFINISHED, DO NOT USE IT.

However, with very small changes (see BFMReader.cpp), 
you will be able to convert bfm to dae (with skeleton) and so on for other formats.