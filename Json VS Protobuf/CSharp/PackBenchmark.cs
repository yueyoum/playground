using System;
using System.IO;
using System.Text;
using System.Collections.Generic;
using System.Diagnostics;

using LitJson;
using ICSharpCode.SharpZipLib.Core;
using ICSharpCode.SharpZipLib.GZip;

using MyProject.Protocol;
using MyProject.Protocol.Define;


class LogEntry
{
    public int Id {get; set;}
    public string Content {get; set;}
    public int Status {get; set;}
    public int Times {get; set;}
}



static class Program
{
    public static int[] tags = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19};
    public static List<LogEntry> logs;

    public static int logAmount;
    public static int benchmarkTimes;


    public static void Main(string[] args)
    {
        try
        {
            logAmount = Convert.ToInt32(args[0]);
            benchmarkTimes = Convert.ToInt32(args[1]);
            if (logAmount < 0 || benchmarkTimes <= 0)
            {
                throw new Exception("wrong arguments");
            }
        }
        catch
        {
            Console.WriteLine("usage: ./pack.exe [LOG AMOUNT] [BENCHMARK TIMES]");
            return;
        }

        logs = new List<LogEntry>();
        for(var i=1; i<=logAmount; i++)
        {
            var log = new LogEntry
            {
                Id = i,
                Content = "Log Contents..." + i,
                Status = i % 2,
                Times = 10000000 + i,
            };

            logs.Add(log);
        }

        Console.WriteLine("LogAmount = {0}", logAmount);
        Console.WriteLine("Protobuf Size         : {0}", PackPb().Length);
        // Console.WriteLine("Protobuf GZip Size    : {0}", PackPbGZip().Length);
        Console.WriteLine("Json Size             : {0}", PackJson().Length);
        Console.WriteLine("Json GZip Size        : {0}", PackJsonGZip().Length);

        Console.WriteLine();
        Console.WriteLine("Benchmark Times = {0}", benchmarkTimes);
        Console.WriteLine("Protobuf Seconds      : {0}", RunPb());
        // Console.WriteLine("Protobuf GZip Seconds : {0}", RunPbGZip());
        Console.WriteLine("Json Seconds          : {0}", RunJson());
        Console.WriteLine("Json GZip Seconds     : {0}", RunJsonGZip());

    }


    public static double RunPb()
    {
        var sw = new Stopwatch();
        sw.Start();
        for (var i = 0; i < benchmarkTimes; i++)
        {
            PackPb();
        }

        sw.Stop();

        var ts = sw.Elapsed;
        return ts.Milliseconds / 1000.0;
    }


    public static double RunPbGZip()
    {
        var sw = new Stopwatch();
        sw.Start();
        for (var i = 0; i < benchmarkTimes; i++)
        {
            PackPbGZip();
        }

        sw.Stop();

        var ts = sw.Elapsed;
        return ts.Milliseconds / 1000.0;
    }

    public static double RunJson()
    {
        var sw = new Stopwatch();
        sw.Start();
        for (var i = 0; i < benchmarkTimes; i++)
        {
            PackJson();
        }

        sw.Stop();

        var ts = sw.Elapsed;
        return ts.Milliseconds / 1000.0;
    }

    public static double RunJsonGZip()
    {
        var sw = new Stopwatch();
        sw.Start();
        for (var i = 0; i < benchmarkTimes; i++)
        {
            PackJsonGZip();
        }

        sw.Stop();

        var ts = sw.Elapsed;
        return ts.Milliseconds / 1000.0;
    }


    public static byte[] PackPb()
    {
        var person = new Person();
        person.id = 1;
        person.name = "My Playground!!!";
        person.tags.AddRange(tags);

        foreach (var log in logs)
        {
            person.logs.Add(new Log
                {
                    id = log.Id,
                    content = log.Content,
                    status = log.Status,
                    times = log.Times
                }
            );
        }

        using (var ms = new MemoryStream())
        {
            var ser = new ProtocolSerializer();
            ser.Serialize(ms, person);
            return ms.ToArray();
        }
    }

    public static byte[] PackJson()
    {
        var json = new JsonData();
        json["id"] = 1;
        json["name"] = "My Playground!!!";
        json["tags"] = new JsonData();
        json["logs"] = new JsonData();

        foreach (var i in tags)
        {
            json["tags"].Add(i);
        }

        foreach(var i in logs)
        {
            var log = new JsonData();
            log["id"] = i.Id;
            log["content"] = i.Content;
            log["status"] = i.Status;
            log["times"] = i.Times;

            json["logs"].Add(log);
        }

        var text = json.ToJson();
//        Console.WriteLine();
//        Console.WriteLine("{0}", text);
//        Console.WriteLine();

        var x = Encoding.UTF8.GetBytes(text);

        return x;
    }


    public static byte[] PackPbGZip()
    {
        return CompressWithGZip(PackPb());
    }


    public static byte[] PackJsonGZip()
    {
        return CompressWithGZip(PackJson());
    }


    public static byte[] CompressWithGZip(byte[] input)
    {

        var outStream = new MemoryStream();
        var s = new GZipOutputStream(outStream);
        s.Write(input, 0, input.Length);
        s.Close();
        outStream.Flush();
        var data = outStream.ToArray();
        outStream.Close();
        return data;
    }
}
