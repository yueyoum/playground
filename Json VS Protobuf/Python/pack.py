#!/usr/bin/env python
import json
import gzip
from cStringIO import StringIO

from protocol_pb2 import Person



class Pack(object):
    def __init__(self, log_amount=0):
        self.TAGS = self.get_tags()
        self.LOGS = self.get_logs(log_amount)

    def get_tags(self):
        return range(20)

    def get_logs(self, amount):
        return [(i, "Log Contents...%d" %i, i % 2, 10000000 + i) for i in range(amount)]


    def create_pb(self):
        msg = Person()
        msg.id = 1
        msg.name = "My Playground!!!"
        msg.tags.extend(self.TAGS)
        for _id, content, status, times in self.LOGS:
            log = msg.logs.add()
            log.id = _id
            log.content = content
            log.status = status
            log.times = times

        return msg.SerializeToString()


    def create_json(self):
        data = {
            'id': 1,
            'name': "My Playground!!!",
            'tags': self.TAGS,
            'logs': []
        }

        for _id, content, status, times in self.LOGS:
            data['logs'].append({
                    'id': _id,
                    'content': content,
                    'status': status,
                    'times': times
                })

        return json.dumps(data)


    def compress_with_gzip(self, data):
        buf = StringIO()
        with gzip.GzipFile(fileobj=buf, mode='w', compresslevel=6) as f:
            f.write(data)

        return buf.getvalue()


if __name__ == '__main__':
    import sys
    import timeit

    try:
        log_amount = int(sys.argv[1])
        times = int(sys.argv[2])
    except:
        print "./pack.py [LOG AMOUNT] [BENCHMARK TIMES]"
        sys.exit(1)


    p = Pack(log_amount)
    pb_data = p.create_pb()
    json_data = p.create_json()
    json_data_gzip = p.compress_with_gzip(json_data)

    print "LogAmount = ", log_amount
    print "Protobuf Size     :", len(pb_data)
    print "Json Size         :", len(json_data)
    print "Json GZip Size    :", len(json_data_gzip)


    pb_t = timeit.Timer("p.create_pb()", setup="from __main__ import p")
    json_t = timeit.Timer("p.create_json()", setup="from __main__ import p")
    json_gzip_t = timeit.Timer("p.compress_with_gzip(p.create_json())", setup="from __main__ import p")

    print
    print "Benchmark Times =", times
    print "Protobuf Seconds  :", pb_t.timeit(number=times)
    print "Json Seconds      :", json_t.timeit(number=times)
    print "Json GZip Seconds :", json_gzip_t.timeit(number=times)
