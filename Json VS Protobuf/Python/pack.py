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
        with gzip.GzipFile(fileobj=buf, mode='w') as f:
            f.write(data)

        return buf.getvalue()


if __name__ == '__main__':
    import sys
    import timeit

    if len(sys.argv) < 2:
        print "python pack.py [log amount] [beachmark times]"
        sys.exit(1)


    log_amount = int(sys.argv[1])
    times = int(sys.argv[2])


    p = Pack(log_amount)
    pb_data = p.create_pb()
    json_data = p.create_json()

    pb_data_gzip = p.compress_with_gzip(pb_data)
    json_data_gzip = p.compress_with_gzip(json_data)

    print "Protobuf Length     :", len(pb_data)
    print "Json Length         :", len(json_data)
    print "Gzip Protobuf Length:", len(pb_data_gzip)
    print "Gzip Json Length    :", len(json_data_gzip)


    p = Pack(log_amount)

    pb_t = timeit.Timer("p.create_pb()", setup="from __main__ import p")
    json_t = timeit.Timer("p.create_json()", setup="from __main__ import p")
    pb_gzip_t = timeit.Timer("p.compress_with_gzip(p.create_pb())", setup="from __main__ import p")
    json_gzip_t = timeit.Timer("p.compress_with_gzip(p.create_json())", setup="from __main__ import p")

    print
    print "Protobuf            :", pb_t.repeat(number=times)
    print "Json                :", json_t.repeat(number=times)
    print "Gzip Protobuf       :", pb_gzip_t.repeat(number=times)
    print "Gzip Json           :", json_gzip_t.repeat(number=times)
