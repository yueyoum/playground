pack.exe: PackBenchmark.cs
	gmcs -out:pack.exe PackBenchmark.cs \
	-r:LitJson.dll \
	-r:ICSharpCode.SharpZipLib.dll \
	-r:protobuf-net.dll \
	-r:ProtocolDefine.dll \
	-r:ProtocolSerializer.dll

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
