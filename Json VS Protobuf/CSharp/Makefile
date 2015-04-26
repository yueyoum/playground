unpack.exe: UnpackBenchmark.cs
	gmcs -out:unpack.exe UnpackBenchmark.cs \
	-r:LitJson.dll \
	-r:ICSharpCode.SharpZipLib.dll \
	-r:protobuf-net.dll \
	-r:ProtocolDefine.dll \
	-r:ProtocolSerializer.dll

.PHONY: clean
clean:
	-rm -f *.exe
