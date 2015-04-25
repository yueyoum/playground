unpack.exe: UnpackBenchmark.cs
	gmcs -out:unpack.exe UnpackBenchmark.cs \
	-recurse:LitJson/*.cs \
	-recurse:SharpZipLib/*.cs \
	-r:protobuf-net.dll \
	-r:ProtocolDefine.dll \
	-r:ProtocolSerializer.dll

.PHONY: clean
clean:
	-rm -f *.exe
