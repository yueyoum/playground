-module(json_vs_proto).

%% json_vs_proto: json_vs_proto library's entry point.

-export([setup/0,
         setup/1]).

-export([pack_pb/1,
         pack_json/1,
         pack_json_gzip/1,
         unpack_pb/0,
         unpack_json/0,
         unpack_json_gzip/0]).


-define(TAGS, lists:seq(0, 19)).
-define(LOGS(Amount), lists:map(
                    fun(Id) ->
                        C = "Log Contents..." ++ integer_to_list(Id),
                        [Id, list_to_binary(C), Id rem 2, 10000000 + Id]
                    end,
                    lists:seq(0, Amount-1)
                    )
        ).


-include("../include/gpb.hrl").
-include("../include/protocol.hrl").



%% API

pack_pb(LogAmount) ->
    Person = #'Person'{
        id = 1,
        name = "My Playground!!!",
        tags = get_tags(),
        logs = create_logs_record_list(LogAmount)
    },

    protocol:encode_msg(Person, [{verify, false}]).


pack_json(LogAmount) ->
    Person = #{
        id => 1,
        name => <<"My Playground!!!">>,
        tags => get_tags(),
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

    [Fun(Item) || Item <- get_logs()].
    

create_logs_maps_list(Amount) when Amount >= 0 ->
    Fun = fun([Id, Content, Status, Times]) ->
        #{
            id => Id,
            content => Content,
            status => Status,
            times => Times
        }
    end,

    [Fun(Item) || Item <- get_logs()].


get_data(Type) ->
    case get(Type) of
        undefined ->
            {Path, _Options} = filename:find_src(json_vs_proto),
            DataPath = get_parent_path(3, Path),
            File = get_file(Type, DataPath),
            {ok, Data} = file:read_file(File),
            put(Type, Data),
            Data;
        Data ->
            Data
    end.

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


get_tags() ->
    case get(tags) of
        undefined ->
            Tags = ?TAGS,
            put(tags, Tags),
            Tags;
        Tags ->
            Tags
    end.

get_logs() ->
    get(logs).

get_logs(Amount) ->
    case get(logs) of
        undefined ->
            Logs = ?LOGS(Amount),
            put(logs, Logs),
            Logs;
        Logs ->
            Logs
    end.


setup() ->
    get_tags(),
    get_data(pb),
    get_data(json),
    get_data(json_gzip),
    ok.

setup(Amount) ->
    get_logs(Amount),
    setup().



%% End of Module.
