-module(json_vs_proto).

%% json_vs_proto: json_vs_proto library's entry point.

-export([pack_pb/1,
         pack_json/1,
         pack_json_gzip/1,
         unpack_pb/0,
         unpack_json/0,
         unpack_json_gzip/0]).


-define(TAGS, lists:seq(1, 20)).
-define(LOGS(Amount), lists:map(
                    fun(Id) -> [Id, "Log Contents..." ++ integer_to_list(Id), Id rem 2, 10000000 + Id] end,
                    lists:seq(1, Amount)
                    )
        ).


-include("../include/gpb.hrl").
-include("../include/protocol.hrl").



%% API

pack_pb(LogAmount) ->
    Person = #'Person'{
        id = 1,
        name = "My Playground!!!",
        tags = ?TAGS,
        logs = create_logs_record_list(LogAmount)
    },

    protocol:encode_msg(Person, [{verify, true}]).


pack_json(LogAmount) ->
    Person = #{
        id => 1,
        name => "My Playground!!!",
        tags => ?TAGS,
        logs => create_logs_maps_list(LogAmount)
    },

    jiffy:encode(Person).


pack_json_gzip(LogAmount) ->
    Json = pack_json(LogAmount),
    zlib:gzip(Json).


unpack_pb() ->
    Data = get_data(pb),
    protocol:decode_msg(Data, 'Person').

unpack_json() ->
    Data = get_data(json),
    jiffy:decode(Data, [return_maps]).

unpack_json_gzip() ->
    Data = zlib:gunzip(get_data(json_gzip)),
    jiffy:decode(Data, [return_maps]).



%% Internals


create_logs_record_list(Amount) when Amount >= 0 ->
    Fun = fun([Id, Content, Status, Times]) ->
        #'Log'{
            id=Id,
            content=Content,
            status=Status,
            times=Times
        }
    end,

    [Fun(Item) || Item <- ?LOGS(Amount)].
    

create_logs_maps_list(Amount) when Amount >= 0 ->
    Fun = fun([Id, Content, Status, Times]) ->
        #{
            id => Id,
            content => Content,
            status => Status,
            times => Times
        }
    end,

    [Fun(Item) || Item <- ?LOGS(Amount)].


get_data(Type) ->
    {Path, _Options} = filename:find_src(json_vs_proto),
    DataPath = get_parent_path(2, Path),
    File = get_file(Type, DataPath),
    {ok, Data} = file:read_file(File),
    Data.

get_file(pb, DataPath) ->
    filename:join(DataPath, "data.pb");

get_file(json, DataPath) ->
    filename:join(DataPath, "data.json");

get_file(json_gzip, DataPath) ->
    filename:join(DataPath, "data.json.gz").


get_parent_path(0, Path) ->
    Path;

get_parent_path(N, Path) when N > 0 ->
    get_parent_path(N-1, filename:dirname(Path)).



%% End of Module.
