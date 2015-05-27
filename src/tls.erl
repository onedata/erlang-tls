%%%--------------------------------------------------------------------
%%% @author Konrad Zemek
%%% @copyright (C) 2015 ACK CYFRONET AGH
%%% This software is released under the MIT license
%%% cited in 'LICENSE.txt'.
%%% @end
%%%--------------------------------------------------------------------
%%% @doc Main API module for TLS.
%%%--------------------------------------------------------------------
-module(tls).
-author("Konrad Zemek").

%% API
-export([start/0, start/1, connect/3, connect/4]).

-type ipaddress() :: {_, _, _, _} | {_, _, _, _, _, _, _, _}.
-type hostname() :: string().
-type host() :: hostname() | ipaddress().
-opaque tlssocket() :: {}.

-on_load(init/0).

%%%===================================================================
%%% API
%%%===================================================================

%%--------------------------------------------------------------------
%% @doc
%% Utility function that starts the tls application. Default type is temporary.
%% @see application(3)
%% @end
%%--------------------------------------------------------------------
-spec start() -> ok | {error, Reason :: term()}.
start() ->
    start(temporary).

%%--------------------------------------------------------------------
%% @doc
%% Utility function that starts the tls application.
%% @see application(3)
%% @end
%%--------------------------------------------------------------------
-spec start(permanent | transient | temporary) ->
    ok | {error, Reason :: term()}.
start(Type) ->
    application:start(tls, Type).


-spec connect(host(), port(), Options :: proplists:proplist()) ->
    {ok, tlssocket()} | {error, Reason :: any()}.
connect(Host, Port, Options) ->
    connect(Host, Port, Options, infinity).

-spec connect(host(), port(), Options :: proplists:proplist(),
    Timeout :: non_neg_integer()) ->
    {ok, tlssocket()} | {error, Reason :: any()}.
connect(Host, Port, Options, Timeout) ->
    Ref = make_ref(),
    ok = connect_nif(Ref, Host, Port),
    receive
        {Ref, Result} ->
            Result
    after Timeout ->
        {error, timeout}
    end.


connect_nif(Ref, Host, Port) ->
    error(nif_not_loaded).


init() ->
    erlang:load_nif("/home/konrad/plgrid/erlang_tls/liberlang_tls", 0).
