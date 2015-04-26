using System;
using System.IO;
using System.Text;
using System.Diagnostics;

using LitJson;
using ICSharpCode.SharpZipLib.GZip;

using MyProject.Protocol;
using MyProject.Protocol.Define;

static class Program
{
    public static string basePath;
    public static int benchmarkTimes;

    public static void Main(string[] args)
    {
        var location = System.Reflection.Assembly.GetEntryAssembly().Location;
        basePath = Path.GetDirectoryName(Path.GetDirectoryName(location));

        try
        {
            benchmarkTimes = Convert.ToInt32(args[0]);
            if (benchmarkTimes <= 0)
            {
                throw new Exception("times must greate than zero!");
            }
        }
        catch
        {
            Console.WriteLine("usage: ./unpack.exe [BENCHMARK TIMES]");
            return;
        }

        Console.WriteLine("Benchmark Times = {0}", benchmarkTimes);
        Console.WriteLine("Protobuf Seconds  : {0}", RunPb());
        Console.WriteLine("Json Seconds      : {0}", RunJson());
        Console.WriteLine("Json GZip Seconds : {0}", RunJsonGZip());
    }

    public static double RunPb()
    {
        var sw = new Stopwatch();
        sw.Start();
        for(var i=0; i<benchmarkTimes; i++)
        {
            UnPackPb();
        }
        sw.Stop();

        var ts = sw.Elapsed;
        return ts.Milliseconds / 1000.0;
    }

    public static double RunJson()
    {
        var sw = new Stopwatch();
        sw.Start();
        for(var i=0; i<benchmarkTimes; i++)
        {
            UnPackJson();
        }
        sw.Stop();

        var ts = sw.Elapsed;
        return ts.Milliseconds / 1000.0;
    }

    public static double RunJsonGZip()
    {
        var sw = new Stopwatch();
        sw.Start();
        for(var i=0; i<benchmarkTimes; i++)
        {
            UnPackJsonGzip();
        }
        sw.Stop();

        var ts = sw.Elapsed;
        return ts.Milliseconds / 1000.0;
    }


    public static byte[] GetData(string fileName)
    {
        var file = Path.Combine(basePath, fileName);
        var count = 0;
        var buffer = new byte[2048];

        using (var ms = new MemoryStream())
        {
            using (var fs = File.OpenRead(file))
            {
                while((count = fs.Read(buffer, 0, buffer.Length)) > 0)
                {
                    ms.Write(buffer, 0, count);
                }
            }

            return ms.ToArray();
        }
    }


    public static void UnPackPb()
    {
        var data = GetData("data.pb");
        var stream = new MemoryStream(data);

        var ser = new ProtocolSerializer();
        var person = ser.Deserialize(stream, null, typeof(Person)) as Person;
        stream.Close();

        // Console.WriteLine("{0}, {1}, {2}, {3}", person.id, person.name, person.tags.Count, person.logs.Count);
    }

    public static void UnPackJson()
    {
        var data = GetData("data.json");
        var json = Encoding.UTF8.GetString(data);

        ConvertJsonType(json);
    }

    public static void UnPackJsonGzip()
    {
        var data = GetData("data.json.gz");
        var inputStream = new MemoryStream(data);
        var outStream = new MemoryStream();

        var zs = new GZipInputStream(inputStream);

        int count = 0;
        var buffer = new byte[2048];
        while((count = zs.Read(buffer, 0, buffer.Length)) > 0)
        {
            outStream.Write(buffer, 0, count);
        }

        var decodedData = outStream.ToArray();
        var json = Encoding.UTF8.GetString(decodedData);

        outStream.Close();
        inputStream.Close();

        ConvertJsonType(json);
    }



    public static void ConvertJsonType(string json)
    {
        JsonData obj = JsonMapper.ToObject(json);
        int id = (int)obj["id"];
        string name = (string)obj["name"];

        // Console.WriteLine("Id: {0}, Name: {1}", id, name);

        // Console.Write("Tags: ");
        foreach(JsonData _tag in obj["tags"])
        {
            int tag = (int)_tag;
            // Console.Write("{0}, ", tag);
        }

        // Console.WriteLine();

        foreach(JsonData _log in obj["logs"])
        {
            int logId = (int)_log["id"];
            string content = (string)_log["content"];
            int status = (int)_log["status"];
            int times = (int)_log["times"];
            // Console.WriteLine("{0}, {1}, {2}, {3}", logId, content, status, times);
        }
    }
}
