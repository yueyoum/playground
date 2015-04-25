using System;
using System.IO;
using System.Text;
using System.Collections.Generic;
using System.Diagnostics;

using LitJson;
using ICSharpCode.SharpZipLib.GZip;

using MyProject.Protocol;
using MyProject.Protocol.Define;

static class Program
{
    public static string basePath;
    public static void Main(string[] args)
    {
        var location = System.Reflection.Assembly.GetEntryAssembly().Location;
        basePath = Path.GetDirectoryName(Path.GetDirectoryName(location));

        int times = 0;
        try
        {
            times = Convert.ToInt32(args[0]);
        }
        catch
        {
            Console.WriteLine("usage: ./unpack.exe [TIMES]");
            return;
        }

        if (times > 0)
        {
            Dictionary<string, Action> actions = new Dictionary<string, Action> {
                {"Protobuf  :", UnPackPb},
                {"Json      :", UnPackJson},
                {"Json Gzip :", UnPackJsonGzip},
            };

            foreach(KeyValuePair<string, Action> kv in actions)
            {
                Console.Write(kv.Key + "[");
                for (var i=0; i<3; i++)
                {
                    Console.Write("{0}, ", Run(kv.Value, times));
                }
                Console.WriteLine("]");
            }
        }
    }

    public static double Run(Action action, int times)
    {
        Stopwatch sw = new Stopwatch();
        sw.Start();
        for(var i=0; i<times; i++)
        {
            action();
        }
        sw.Stop();

        TimeSpan ts = sw.Elapsed;
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
