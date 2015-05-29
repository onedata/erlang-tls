/**
 * @file wrapper.cpp
 * @author Konrad Zemek
 * @copyright (C) 2015 ACK CYFRONET AGH
 * @copyright This software is released under the MIT license cited in
 * 'LICENSE.txt'
 */

#include "commonDefs.hpp"
#include "nifpp.h"
#include "tlsApplication.hpp"
#include "tlsSocket.hpp"

#include <memory>
#include <tuple>

class Env {
public:
    Env()
        : env{enif_alloc_env(), enif_free_env}
    {
    }

    operator ErlNifEnv *() { return env.get(); }

private:
    std::shared_ptr<ErlNifEnv> env;
};

namespace {

nifpp::str_atom ok{"ok"};
nifpp::str_atom error{"error"};

one::etls::TLSApplication app;

one::etls::ErrorFun onError(Env localEnv, ErlNifPid pid, nifpp::TERM ref)
{
    return [=](std::string reason) mutable {
        auto message = nifpp::make(
            localEnv, std::make_tuple(ref, std::make_tuple(error, reason)));

        enif_send(nullptr, &pid, localEnv, message);
    };
}

} // namespace

extern "C" {

static int load(ErlNifEnv *env, void ** /*priv*/, ERL_NIF_TERM /*load_info*/)
{
    nifpp::register_resource<one::etls::TLSSocket::Ptr>(
        env, nullptr, "TLSSocket");

    return 0;
}

static ERL_NIF_TERM connect_nif(
    ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[])
{
    try {
        Env localEnv;

        nifpp::TERM ref{enif_make_copy(localEnv, argv[0])};
        auto host = nifpp::get<std::string>(env, argv[1]);
        unsigned short port = nifpp::get<int>(env, argv[2]);

        ErlNifPid pid;
        enif_self(env, &pid);

        auto onSuccess = [=](one::etls::TLSSocket::Ptr socket) mutable {
            auto resource =
                nifpp::construct_resource<one::etls::TLSSocket::Ptr>(socket);

            auto message = nifpp::make(localEnv,
                std::make_tuple(ref, std::make_tuple(
                                         ok, nifpp::make(localEnv, resource))));

            enif_send(nullptr, &pid, localEnv, message);
        };

        auto sock = std::make_shared<one::etls::TLSSocket>(app.ioService());
        sock->connectAsync(sock, host, port, std::move(onSuccess),
            onError(localEnv, pid, ref));

        return nifpp::make(env, ok);
    }
    catch (const nifpp::badarg &) {
        return enif_make_badarg(env);
    }
    catch (const std::exception &e) {
        return nifpp::make(env, std::make_tuple(error, std::string{e.what()}));
    }
}

static ERL_NIF_TERM send_nif(
    ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[])
{
    try {
        Env localEnv;

        nifpp::TERM ref{enif_make_copy(localEnv, argv[0])};
        auto sock = *nifpp::get<one::etls::TLSSocket::Ptr *>(env, argv[1]);
        nifpp::TERM data{enif_make_copy(localEnv, argv[2])};

        ErlNifBinary bin;
        enif_inspect_iolist_as_binary(localEnv, data, &bin);

        boost::asio::const_buffer buffer{bin.data, bin.size};

        ErlNifPid pid;
        enif_self(env, &pid);

        auto onSuccess = [=]() mutable {
            auto message = nifpp::make(localEnv, std::make_tuple(ref, ok));
            enif_send(nullptr, &pid, localEnv, message);
        };

        sock->sendAsync(
            sock, buffer, std::move(onSuccess), onError(localEnv, pid, ref));

        return nifpp::make(env, ok);
    }
    catch (const nifpp::badarg &) {
        return enif_make_badarg(env);
    }
    catch (const std::exception &e) {
        return nifpp::make(env, std::make_tuple(error, std::string{e.what()}));
    }
}

static ERL_NIF_TERM recv_nif(
    ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[])
{
    try {
        Env localEnv;

        nifpp::TERM ref{enif_make_copy(localEnv, argv[0])};
        auto sock = *nifpp::get<one::etls::TLSSocket::Ptr *>(env, argv[1]);
        auto size = nifpp::get<std::size_t>(env, argv[2]);

        ErlNifPid pid;
        enif_self(env, &pid);

        auto bin = std::make_shared<nifpp::binary>(size == 0 ? 1024 : size);

        auto onSuccess = [=](boost::asio::mutable_buffer buffer) mutable {
            if (bin->size != boost::asio::buffer_size(buffer))
                enif_realloc_binary(
                    bin.get(), boost::asio::buffer_size(buffer));

            auto message = nifpp::make(localEnv,
                std::make_tuple(ref, std::make_tuple(
                                         ok, nifpp::make(localEnv, *bin))));

            enif_send(nullptr, &pid, localEnv, message);
        };

        boost::asio::mutable_buffer buffer{bin->data, bin->size};
        if (size == 0) {
            sock->recvAnyAsync(sock, buffer, std::move(onSuccess),
                onError(localEnv, pid, ref));
        }
        else {
            sock->recvAsync(sock, buffer, std::move(onSuccess),
                onError(localEnv, pid, ref));
        }

        return nifpp::make(env, ok);
    }
    catch (const nifpp::badarg &) {
        return enif_make_badarg(env);
    }
    catch (const std::exception &e) {
        return nifpp::make(env, std::make_tuple(error, std::string{e.what()}));
    }
}

static ErlNifFunc nif_funcs[] = {{"connect_nif", 3, connect_nif},
    {"send_nif", 3, send_nif}, {"recv_nif", 3, recv_nif}};

ERL_NIF_INIT(tls, nif_funcs, load, NULL, NULL, NULL)

} // extern C
